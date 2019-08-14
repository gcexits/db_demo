#include "HttpFile.h"

namespace duobei {

void HttpFile::DownloadThread() {
    CURLAgent curl_agent;
    while (worker.running) {
        //TODO：添加检查代码，检查map中是否有过期数据过期后删除
        //如果大于文件大小,则退出
        std::unique_lock<std::mutex> lck(close_Mx_);
        if (!worker.running) {
            return;
        }
        if (worker.Eof()) {
            lck.unlock();
            std::unique_lock<std::mutex> cl(synchronizer.mtx);
            synchronizer.cond.wait(cl);
            continue;
        }
        std::unique_lock<std::mutex> cache_lock(cache_.mtx);
        auto key = worker.cache_index;
        auto left = worker.cache_index * kCacheLength;
        //如果map中小于预约数量则等待
        if (worker.SeekAtLeft() && (left - worker.current_position) >= kCacheBlockSum * kCacheLength && cache_.full()) {
            cache_lock.unlock();
            lck.unlock();
            std::unique_lock<std::mutex> cl(synchronizer.mtx);
            synchronizer.cond.wait(cl);
            continue;
        }

        if (cache_.buffers.find(worker.cache_index) != cache_.buffers.end()) {
            // 存在
            ++worker.cache_index;
            cache_lock.unlock();
            lck.unlock();
            continue;
        }
        cache_lock.unlock();

        auto right_ = left + kCacheLength;
        auto right = right_ > worker.file_size ? worker.file_size - 1 : right_ - 1;
        auto buf_ = Buffer::New(left, right);
        ++worker.cache_index;

        // todo: bf 的有效期问题
        HttpClient::DownloadBuffer bf(buf_->data);
        int ret = curl_agent.Download(url_, buf_->begin, buf_->end, bf);
        if (ret != 0 || bf.size == 0) {
            // 失败
            if (worker.cache_index > 0) {
                --worker.cache_index;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            //lck.unlock();
            continue;
        }
        cache_lock.lock();
        cache_.buffers.emplace(key, std::move(buf_));
        cache_lock.unlock();
        //lck.unlock();
    }
    // WriteDebugLog("DownloadThread quit!");
}

int HttpFile::Open(const std::string &url) {
    url_ = url;
    double size = 0;
    for (int count = 0; count < 5 && size <= 0; ++count) {
        size = http.getHttpFileSize(url);
        if (size <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    if (size <= 0) {
        return FILEERR;
    }
    std::cout << "file_size = " << size << std::endl;
    std::lock_guard<std::mutex> lock(worker.mtx_);
    worker.file_size = static_cast<size_t>(size);
    worker.running = true;
    worker.download_thread = std::thread(&HttpFile::DownloadThread, this);
    return FILEOK;
}

//跳转指定大小 0 success ,-1 error , -2 end OK
int HttpFile::SeekTo(size_t size) {
    if (worker.SeekOverflow(size)) {
        return FILEERR;
    }
    // todo: seek需要释放无用的内存
    std::unique_lock<std::mutex> cache_lock(cache_.mtx);
    worker.Seek(size);
    for (auto iter = cache_.buffers.begin(); iter != cache_.buffers.end();) {
        if (iter->first < worker.cache_index) {
            cache_.buffers.erase(iter++);
        } else {
            iter++;
        }
    }
    synchronizer.cond.notify_all();
    return FILEOK;
}

int HttpFile::Seek(int delta) {
    if (worker.SeekOverflow(delta)) {
        return FILEERR;
    }
    worker.Seek(delta);
    synchronizer.cond.notify_all();
    return FILEOK;
}

//读取指定大小 返回读取大小 -2 end 读取完成后指针会延后 OK
int HttpFile::ReadInternal(uint8_t *buf, size_t bufSize, size_t readSize, int &hasReadSize) {
    hasRead = 0;
    size_t endRead = readSize;
    int outTime = 3000 * 2;  // *5 后超时
    int maxNoticTime = 200;  //多久开始提醒正在读取中
    if (worker.FileEnd()) {
        return FILEEND;
    }
    bool isWait = false;
    while (worker.running && hasRead != readSize) {
        if (worker.FileEnd()) {
            return FILEEND;
        }
        auto key = (worker.current_position) / kCacheLength;
        std::unique_lock<std::mutex> buf_lock(cache_.mtx);
        auto v = cache_.buffers.find(key);
        if (v == cache_.buffers.end()) {
            //TODO: 激活读取线程 ，等待读取,并且阻塞所有读取线程
            buf_lock.unlock();
            synchronizer.cond.notify_all();
            //TODO: 阻塞所有读取函数
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            if (maxNoticTime <= 0) {
                isWait = true;
                maxNoticTime = 400;
            }
            maxNoticTime--;
            if (outTime-- <= 0) {
                return FILEERR;
            }
            continue;
        } else {
            //TODO： 找到并读取
            //如果足够读取,则读取并退出
            if ((v->second->end + 1 - worker.current_position) >= endRead) {
                //memcpy(buf + hasRead, v->second->data + (current_position - v->second->begin), endRead);
                v->second->Read(buf + hasRead, worker.current_position, endRead);
                hasRead += endRead;
                worker.current_position += endRead;
                if (!cache_.enough()) {
                    synchronizer.cond.notify_all();
                }
                buf_lock.unlock();
                hasReadSize = hasRead;
                break;
            } else {  //不够读取
                //memcpy(buf + hasRead, v->second->data + (current_position - v->second->begin), v->second->end + 1 - current_position);
                v->second->Read(buf + hasRead, worker.current_position);
                endRead -= (v->second->end + 1 - worker.current_position);
                hasRead += (v->second->end + 1 - worker.current_position);
                worker.current_position = v->second->end + 1;
                cache_.buffers.erase(v);
                hasReadSize = hasRead;
            }
        }
        if (!cache_.enough()) {
            synchronizer.cond.notify_all();
        }
        buf_lock.unlock();
    }
    return readSize;
}

int HttpFile::Read(uint8_t *buf, size_t bufSize, size_t readSize, int &hasReadSize) {
    return Read(buf, bufSize, readSize, 15, hasReadSize);
}

int HttpFile::Read(uint8_t *buf, size_t bufSize, size_t readSize, size_t head_size, int &hasReadSize) {
    auto ret = ReadInternal(buf, bufSize, readSize, hasReadSize);
    if (ret == FILEEND) {
        return ret;
    }

    if (ret != readSize) {
        auto delta = static_cast<int>(hasRead + head_size);
        Seek(-delta);
    }
    return ret;
}
//读取后指针不会延迟 OK
int HttpFile::ReadDelay(uint8_t *buf, size_t bufSize, size_t readSize) {
    size_t hasRead = 0;
    size_t endRead = readSize;
    size_t seekTmp = worker.current_position;
    if (worker.FileEnd()) {
        return FILEEND;
    }
    bool isWait = false;
    int maxNoticTime = 10;   //多久开始提醒正在读取中
    int outTime = 3000 * 2;  // *5 后超时
#if defined(SEEK_TEST)
    while (worker.running && hasRead != readSize && readOption().RunState.isRunning() && !newseek) {
#else
    while (worker.running && hasRead != readSize) {
#endif
        auto key = (worker.current_position) / kCacheLength;
        std::unique_lock<std::mutex> buf_lock(cache_.mtx);
        auto v = cache_.buffers.find(key);
        if (v == cache_.buffers.end()) {
            //TODO: 激活读取线程 ，等待读取,并且阻塞所有读取线程
            buf_lock.unlock();
            synchronizer.cond.notify_all();
            //TODO: 阻塞所有读取函数
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            if (maxNoticTime <= 0) {
                isWait = true;
                maxNoticTime = 400;
            }
            maxNoticTime--;
            if (outTime-- <= 0) {
                return FILEERR;
            }
            continue;
        } else {
            //TODO： 找到并读取
            //如果足够读取,则读取并退出
            if (v->second->end < worker.current_position) {
                buf_lock.unlock();
                return FILEEND;
            } else if ((v->second->end + 1 - worker.current_position) >= endRead) {
                //memcpy(buf + hasRead, v->second->data + (current_position - v->second->begin), endRead);
                v->second->Read(buf + hasRead, worker.current_position, endRead);
                hasRead += endRead;
                worker.current_position += endRead;
                if (!cache_.enough()) {
                    synchronizer.cond.notify_all();
                }
                buf_lock.unlock();
                break;
            } else {  //不够读取
                //memcpy(buf + hasRead, v->second->data + (current_position - v->second->begin), v->second->end + 1 - current_position);
                v->second->Read(buf + hasRead, worker.current_position);
                endRead -= (v->second->end + 1 - worker.current_position);
                hasRead += (v->second->end + 1 - worker.current_position);
                worker.current_position = v->second->end + 1;
            }
        }
        buf_lock.unlock();
    }
    if (isWait) {
    }
    worker.current_position = seekTmp;
    return readSize;
}
//回跳到文件开始 OK
int HttpFile::SeekToBegin() {
    std::unique_lock<std::mutex> cache_lock(cache_.mtx);
    worker.cache_index = 0;
    worker.current_position = 0;
    return FILEOK;
}
//OK
void HttpFile::Close() {
    std::lock_guard<std::mutex> lock(worker.mtx_);
    if (!worker.running) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(close_Mx_);
        worker.running = false;
        //worker.curl_agent.Terminate();
    }

    do {
        synchronizer.cond.notify_all();
        if (worker.download_thread.joinable()) {
            worker.download_thread.join();
            break;
        }
    } while (true);
    cache_.buffers.clear();
}

}  // namespace duobei
