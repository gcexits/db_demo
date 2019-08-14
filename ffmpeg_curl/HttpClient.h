#pragma once

#include <curl/curl.h>
#include <iostream>
#include <string>

#include "Time.h"

typedef size_t (*CurlOptionWriteFunction)(void *, size_t, size_t, void *);
typedef int (*CurlOptionProgressFunction)(void *, double, double, double, double);
using FunctionPtr = size_t (*)(void *, size_t, size_t, void *);

namespace duobei {

int OnDebug(CURL *, curl_infotype type, char *data, size_t size, void *);
class HttpClient {
public:
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

        EasyCURL(bool debug) {
            if (++__count == 1) {
                /* Must initialize libcurl before any threads are started */
                curl_global_init(CURL_GLOBAL_ALL);
            }

            curl = curl_easy_init();
            if (debug) {
                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
                curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            }

            //https://curl.haxx.se/libcurl/c/CURLOPT_SOCKOPTDATA.html
            curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
            curl_easy_setopt(curl, CURLOPT_SOCKOPTDATA, &sockopt_data);
        }
        explicit EasyCURL()
            : EasyCURL(false) {}
        ~EasyCURL() {
            FreeHeaders();
            curl_easy_cleanup(curl);
        }

        std::string Escape(const std::string &source) {
            char *out = curl_easy_escape(curl, source.c_str(), source.length());
            std::string dest(out);
            if (out) {
                curl_free(out);
            }
            return dest;
        }

        struct curl_slist *headers = nullptr;
        // "Content-Type: application/json"
        void setContentType(const std::string &c) {
            headers = curl_slist_append(headers, "Expect:");
            headers = curl_slist_append(headers, c.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

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

    static std::string UserAgent;
    HttpClient() = default;
    // note：目前移动端同步到pc的curl方式，移动端原来的方式暂时保留

    ~HttpClient() = default;

public:
    /**
	* @brief HTTP POST请求
	* @param url 输入参数,请求的Url地址,如:http://www.baidu.com
	* @param forms 输入参数,使用如下格式para1=val1&para2=val2&…
	* @param response 输出参数,返回的内容
	* @return 返回是否Post成功
	*/
    int Post(const std::string &url, const std::string &forms, std::string &response);

    /**
	* @brief HTTP GET请求
	* @param url 输入参数,请求的Url地址,如:http://www.baidu.com
	* @param response 输出参数,返回的内容
	* @return 返回是否Get成功
	*/
    int Get(const std::string &url, std::string &response);

    /**
	* @brief HTTPS POST请求,无证书版本
	* @param url 输入参数,请求的Url地址,如:https://www.alipay.com
	* @param forms 输入参数,使用如下格式para1=val1&para2=val2&…
	* @param strResponse 输出参数,返回的内容
	* @param certPath 输入参数,为CA证书的路径.如果输入为NULL,则不验证服务器端证书的有效性.
	* @return 返回是否Post成功
	*/
    int Posts(const std::string &url, const std::string &forms, std::string &strResponse,
              const std::string &certPath = NULL);

    /**
	* @brief HTTPS GET请求,无证书版本
	* @param url 输入参数,请求的Url地址,如:https://www.alipay.com
	* @param response 输出参数,返回的内容
	* @param certPath 输入参数,为CA证书的路径.如果输入为NULL,则不验证服务器端证书的有效性.
	* @return 返回是否Post成功
	*/
    int Gets(const std::string &url, std::string &response, const std::string &certPath = NULL);
    /**
	 * 获取文件大小
	 */
    double getHttpFileSize(const std::string &url);  //CurlOptionWriteFunction
    /**
	 * @param url 地址
	 * @param begin 开始位置
	 * @param end  结束位置
	 * @param wfun 下载文件读取函数
	 * @param progress 进度回调函数
	 */
    // todo: PC端回放下载文件
    int Download(const std::string &url, size_t begin, size_t end, DownloadBuffer &buffer, CurlOptionProgressFunction progress);
    int Download(const std::string &url, size_t begin, size_t end, DownloadBuffer &buffer);

    int downFile(const char *url, CurlOptionWriteFunction f, void *v);

    static std::string UrlEncode(const std::string &source) {
        EasyCURL c;
        return c.Escape(source);
    }

private:
    bool debug_ = false;
};

struct CURLAgent {
    using EasyCURL = HttpClient::EasyCURL;
    using DownloadBuffer = HttpClient::DownloadBuffer;
    EasyCURL c;
    struct curl_slist *list = nullptr;
    char buf_[64] = {0};
    CURLAgent() {
    }
    ~CURLAgent() {
    }

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
        curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 5);  //curl最长执行秒数

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
        curl_slist_free_all(list); /* free the list again */
        list = nullptr;
        return retcCode;
    }
};

}  // namespace duobei
