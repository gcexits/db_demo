#pragma once

#include <curl/curl.h>

#include <thread>
#include <iostream>
#include <fstream>

class HttpClient {
    std::string url_;
    double file_size;
public:
    HttpClient() = default;
    ~HttpClient() = default;

    struct EasyCURL {
        static int __count;
        CURL *curl = nullptr;

        int sockopt_data = 1;
        static int sockopt_callback(void *clientp, curl_socket_t curlfd,
                                    curlsocktype purpose) {
            int value = *(int *)clientp;
            setsockopt(curlfd, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
            return CURL_SOCKOPT_OK;
        }


        EasyCURL() {
//            if (++__count == 1) {
//                curl_global_init(CURL_GLOBAL_ALL);
//            }

            curl = curl_easy_init();

            //https://curl.haxx.se/libcurl/c/CURLOPT_SOCKOPTDATA.html
            curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
            curl_easy_setopt(curl, CURLOPT_SOCKOPTDATA, &sockopt_data);
        }
        ~EasyCURL() {
            FreeHeaders();
            curl_easy_cleanup(curl);
        }

        struct curl_slist *headers = nullptr;

        void FreeHeaders() {
            if (headers) {
                curl_slist_free_all(headers);
            }
        }

        CURLcode Perform() {
            char errbuf[CURL_ERROR_SIZE] = {0};
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
            return curl_easy_perform(curl);  // todo: crash 偶现
        }
    };

    struct DownloadBuffer {
        const void *data;
        int size;

        explicit DownloadBuffer(uint8_t *ptr)
            : data(ptr), size(0) {}
        size_t eatData(void *buffer, size_t len) {
            memcpy((char *)data + size, buffer, len);
            size += len;
            return len;
        }
        static size_t callback(void *pBuffer, size_t nSize, size_t nMemByte, void *pParam) {
            //把下载到的数据以追加的方式写入文件(一定要有a，否则前面写入的内容就会被覆盖了)
            auto that = static_cast<DownloadBuffer *>(pParam);
            if (!that) {
                return 0;
            }
            //WriteDebugLog("callback::[%d][%lu]", buffer->size, nMemByte);
            return that->eatData(pBuffer, nSize * nMemByte);
        }
    };

    struct CURLAgent {
        using EasyCURL = HttpClient::EasyCURL;
        using DownloadBuffer = HttpClient::DownloadBuffer;
        EasyCURL c;
        struct curl_slist *list = nullptr;
        char buf_[64] = {0};
        CURLAgent() = default;
        ~CURLAgent() = default;

        int Download(const std::string &url, size_t begin, size_t end, DownloadBuffer &buffer) {
            if (!c.curl) {
                return CURLE_FAILED_INIT;
            }

            snprintf(buf_, 64, "Range: bytes=%zu-%zu", begin, end);
            curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
            list = curl_slist_append(list, buf_);
            curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
            curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
            curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, list);
            //设置接收数据的回调
            curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, buffer.callback);  // CurlOptionWriteFunction
            curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, &buffer);
//            curl_easy_setopt(c.curl, CURLOPT_TIMEOUT_MS, 2000);  //curl最长执行秒数

            //curl_easy_setopt(c.curl, CURLOPT_INFILESIZE, lFileSize);
            //curl_easy_setopt(c.curl, CURLOPT_HEADER, 1);
            //curl_easy_setopt(c.curl, CURLOPT_NOBODY, 1);
            curl_easy_setopt(c.curl, CURLOPT_NOSIGNAL, 1);
            // 设置重定向的最大次数
            curl_easy_setopt(c.curl, CURLOPT_MAXREDIRS, 5);
            // 设置301、302跳转跟随location
            curl_easy_setopt(c.curl, CURLOPT_FOLLOWLOCATION, 1);
            //设置进度回调函数
            //curl_easy_getinfo(c.curl,  CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lFileSize);
            //curl_easy_setopt(c.curl, CURLOPT_PROGRESSDATA, g_hDlgWnd);
            //开始执行请求
            CURLcode retcCode = c.Perform();
            if (retcCode != CURLE_OK) {
                std::cout << "err = " << retcCode << ", msg = " << curl_easy_strerror(retcCode) << std::endl;
            }
            //清理curl，和前面的初始化匹配
            curl_slist_free_all(list);
            list = nullptr;
            return retcCode;
        }
    };

    void DownloadThread() {
        CURLAgent curl_agent;
        uint8_t *data = new uint8_t[file_size + 1];
        std::ofstream fp_out;
        fp_out.open("./curl_down.mp3", std::ios::binary | std::ios::ate | std::ios::out);
        while (1) {
            // todo: bf 的有效期问题
            HttpClient::DownloadBuffer bf(data);
            int ret = curl_agent.Download(url_, 0, file_size, bf);
            if (ret != 0 || bf.size == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                fp_out.write((char *)data, file_size);
                break;
            }
        }
        fp_out.close();
    }
    std::thread download_thread;  //读取文件线程

    double getHttpFileSize(const std::string &url);
};

