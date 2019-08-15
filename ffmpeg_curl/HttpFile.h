#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include "HttpClient.h"
#include "Demuxer.h"

namespace duobei {

constexpr size_t kCacheLength = 512 * 1024;  //每段缓存大小
constexpr size_t kCacheBlockSum = 20;

enum {
    FILEOK = 0,
    FILEEND = -1,
    FILEERR = -2
};

class HttpFile {
    IOBufferContext *ioBufferContext = nullptr;
    std::string url_;

    struct DownloadWorker {
        size_t file_size = 0;
        size_t cache_index = 0;       //当前读取到的文件位置内部使用,以CACHE_LEN 为分割
        size_t current_position = 0;  //当前外部读取到的文件位置
        int lastseek_position = 0;
        std::thread download_thread;  //读取文件线程
        bool running = false;         //是否打开
        mutable std::mutex mtx_;

        bool Eof() const {
            return cache_index * kCacheLength > file_size;
        }

        bool FileEnd() const {
            return current_position == file_size;
        }

        bool SeekAtLeft() const {
            return current_position <= cache_index * kCacheLength;
        }

        void Seek(size_t size) {
            current_position += size;
            cache_index = current_position / kCacheLength;
        }

        void Seek(int delta) {
            current_position += delta;
            cache_index = current_position / kCacheLength;
        }

        void SeekTo(int delta) {
            current_position = delta;
            cache_index = current_position / kCacheLength;
        }

        bool SeekOverflow(size_t size) const {
            return current_position + size > file_size;
        }

        bool SeekOverflow(int delta) const {
            return current_position + delta > file_size && current_position + delta >= 0;
        }
    };

    struct Buffer {
        uint8_t *data = nullptr;     //buf
        size_t size = kCacheLength;  //总大小
        size_t nowReadSize = 0;      //当前读取到的位置
        size_t begin = 0;            //在整个文件中的开始位置
        size_t end = 0;              //在整个文件中的结束位置

        using Ptr = std::unique_ptr<Buffer>;
        using Map = std::unordered_map<size_t, Ptr>;

        explicit Buffer(size_t b, size_t e) : begin(b), end(e) {
            data = new uint8_t[size];
        }
        ~Buffer() {
            delete[] data;
        }

        static Ptr New(size_t b, size_t e) {
            return std::make_unique<Buffer>(b, e);
        }

        void Read(uint8_t *buf, size_t nowSeekg, size_t length) {
            assert(buf != nullptr);
            assert(nowSeekg >= begin);
            // memcpy(buf + hasRead, v->second->data + (current_position - v->second->begin), endRead);
            memcpy(buf, data + nowSeekg - begin, length);
        }

        void Read(uint8_t *buf, size_t nowSeekg) {
            return Read(buf, nowSeekg, end + 1 - nowSeekg);
        }
    };
    struct BufferCache {
        Buffer::Map buffers;
        std::mutex mtx;
        bool enough() const {
            return buffers.size() > kCacheBlockSum / 2;
        }
        bool full() const {
            return buffers.size() > kCacheBlockSum;
        }
    };
    BufferCache cache_;

    struct Synchronizer {
        mutable std::mutex mtx;
        mutable std::condition_variable cond;
    };
    Synchronizer synchronizer;

    mutable std::mutex close_Mx_;
    HttpClient http;
    void DownloadThread();
    size_t hasRead = 0;
    int ReadInternal(uint8_t *buf, size_t bufSize, size_t readSize, int &hasReadSize);
    int Seek(int delta);

public:
    void setIOBufferContext(IOBufferContext *ioBufferContext_) {
        ioBufferContext = ioBufferContext_;
    }
    DownloadWorker worker;
    HttpFile() {}
    virtual ~HttpFile() {
        Close();
    }

    int Open(const std::string &);
    std::thread read;
    void startRead();
    int Read(uint8_t *buf, size_t bufSize, size_t readSize, int &hasReadSize);
    int Read(uint8_t *buf, size_t bufSize, size_t readSize, size_t head_size, int &hasReadSize);
    int ReadDelay(uint8_t *buf, size_t bufSize, size_t readSize);
    void Close();
    int SeekTo(size_t size);
    int SeekToBegin();
    void fixCurrent(int delta) {
        worker.SeekTo(delta);
    }
};

}  // namespace duobei
