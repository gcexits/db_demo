#include "HttpClient.h"

#include <mutex>

namespace duobei {

std::string HttpClient::UserAgent = "sdk_defult/1.0";

int HttpClient::EasyCURL::__count = 0;

int OnDebug(CURL *, curl_infotype type, char *data, size_t size, void *) {
    if (type == CURLINFO_TEXT) {
    } else if (type == CURLINFO_HEADER_IN) {
    } else if (type == CURLINFO_HEADER_OUT) {
    } else if (type == CURLINFO_DATA_IN) {
    } else if (type == CURLINFO_DATA_OUT) {
    }
    return 0;
}

static std::mutex OnWriteData_mutex_;
//	size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
//	https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c
static size_t OnWriteData(void *buffer, size_t size, size_t nmemb, void *userdata) {
    std::lock_guard<std::mutex> lck(OnWriteData_mutex_);
    if (!userdata || !buffer) {
        return -1;
    }
    auto *str = reinterpret_cast<std::string *>(userdata);
    str->append((char *)buffer, size * nmemb);
    return nmemb;
}
static size_t defualtCallback(void *buffer, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

int HttpClient::Post(const std::string &url, const std::string &forms, std::string &response) {
    EasyCURL c(debug_);
    if (!c.curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.curl, CURLOPT_USERAGENT, UserAgent.c_str());

    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //不验证证书  SKIP_PEER_VERIFICATION
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host  SKIP_HOSTNAME_VERIFICATION

    curl_easy_setopt(c.curl, CURLOPT_POST, 1);
    curl_easy_setopt(c.curl, CURLOPT_POSTFIELDS, forms.c_str());

    curl_easy_setopt(c.curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(c.curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(c.curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 3);

    c.setContentType("Content-Type: application/x-www-form-urlencoded");

    return c.Perform();  // 频繁出现 crash
}

int HttpClient::Get(const std::string &url, std::string &response) {
    EasyCURL c(debug_);
    if (!c.curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.curl, CURLOPT_USERAGENT, UserAgent.c_str());
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c.curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, &response);
    /**
	* 当多个线程都使用超时处理的时候，同时主线程中有sleep或是wait等操作。
	* 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
	*/
    curl_easy_setopt(c.curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(c.curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 3);
    return c.Perform();
}

int HttpClient::Posts(const std::string &url, const std::string &forms, std::string &response, const std::string &certPath) {
    EasyCURL c(debug_);
    if (!c.curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.curl, CURLOPT_USERAGENT, UserAgent.c_str());
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c.curl, CURLOPT_POST, 1);
    curl_easy_setopt(c.curl, CURLOPT_POSTFIELDS, forms.c_str());
    curl_easy_setopt(c.curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(c.curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 3);
    if (certPath.empty()) {
        curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, false);
    } else {
        //缺省情况就是PEM，所以无需设置，另外支持DER
        //curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE,"PEM");
        curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, true);
        curl_easy_setopt(c.curl, CURLOPT_CAINFO, certPath.c_str());
    }
    curl_easy_setopt(c.curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 3);
    return c.Perform();
}

int HttpClient::Gets(const std::string &url, std::string &response, const std::string &certPath) {
    EasyCURL c(debug_);
    if (!c.curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.curl, CURLOPT_USERAGENT, UserAgent.c_str());
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c.curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(c.curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 3);
    if (certPath.empty()) {
        curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, false);
    } else {
        curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, true);
        curl_easy_setopt(c.curl, CURLOPT_CAINFO, certPath.c_str());
    }
    curl_easy_setopt(c.curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 3);
    return c.Perform();
}

double HttpClient::getHttpFileSize(const std::string &url) {
#if defined(ENABLE_TERMINATE_CURL_EASY_PERFORM_TIMEOUT)
    auto c = std::make_shared<EasyCURL>();
    curl_holder = c;
#else
    EasyCURL c;
#endif
    //auto c = std::make_shard<EasyCURL>();
#if defined(ENABLE_TERMINATE_CURL_EASY_PERFORM_TIMEOUT)
    if (!c->curl) {
        return 0;
    }

    curl_easy_setopt(c->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c->curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c->curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, defualtCallback);
    curl_easy_setopt(c->curl, CURLOPT_HEADER, 1);  //只要求header头
    curl_easy_setopt(c->curl, CURLOPT_NOBODY, 1);  //不需求body
    curl_easy_setopt(c->curl, CURLOPT_TIMEOUT, 3);
    //curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, save_header);

    CURLcode retCode = c->Perform();
    if (retCode != CURLE_OK) {
        WriteErrorLog("getHttpFileSize err[%s]", curl_easy_strerror(retCode));
        return 0;
    }

    double len = 0;
    retCode = curl_easy_getinfo(c->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
#else
    if (!c.curl) {
        return 0;
    }

    curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, defualtCallback);
    curl_easy_setopt(c.curl, CURLOPT_HEADER, 1);  //只要求header头
    curl_easy_setopt(c.curl, CURLOPT_NOBODY, 1);  //不需求body
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 3);
    //curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, save_header);

    CURLcode retCode = c.Perform();
    if (retCode != CURLE_OK) {
        return 0;
    }

    double len = 0;
    retCode = curl_easy_getinfo(c.curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
#endif
    if (retCode != CURLE_OK) {
        return 0;
    }
    return len;
}

int HttpClient::downFile(const char *url, CurlOptionWriteFunction f, void *v) {
    EasyCURL c;
    if (!c.curl) {
        return CURLE_FAILED_INIT;
    }
    curl_easy_setopt(c.curl, CURLOPT_URL, url);
    //设置接收数据的回调
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, f);    // CurlOptionWriteFunction
    curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, v);
    //curl_easy_setopt(c.curl, CURLOPT_INFILESIZE, lFileSize);
    //curl_easy_setopt(c.curl, CURLOPT_HEADER, 1);
    //curl_easy_setopt(c.curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(c.curl, CURLOPT_NOSIGNAL, 1);
    // 设置重定向的最大次数
    curl_easy_setopt(c.curl, CURLOPT_MAXREDIRS, 5);
    // 设置301、302跳转跟随location
    curl_easy_setopt(c.curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(c.curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 15);
    //curl_easy_getinfo(c.curl,  CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lFileSize);
    //curl_easy_setopt(c.curl, CURLOPT_PROGRESSDATA, g_hDlgWnd);
    //开始执行请求
    CURLcode retCode = c.Perform();
    if (CURLE_OK != retCode) {
    }
    return retCode;
}
// todo: PC端回放下载文件
int HttpClient::Download(const std::string &url,
                         size_t begin, size_t end, DownloadBuffer &buffer, CurlOptionProgressFunction progress) {
    EasyCURL c;
    if (!c.curl) {
        return CURLE_FAILED_INIT;
    }

    struct curl_slist *list = NULL;
    char buf_[64] = {0};
    snprintf(buf_, 64, "Range: bytes=%lu-%lu", begin, end);
    curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
    list = curl_slist_append(list, buf_);
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, list);
    //设置接收数据的回调
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, buffer.callback);  // CurlOptionWriteFunction
    curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, &buffer);
    //curl_easy_setopt(c.curl, CURLOPT_TIMEOUT_MS , 10000);  //curl最长执行秒数

    //curl_easy_setopt(c.curl, CURLOPT_INFILESIZE, lFileSize);
    //curl_easy_setopt(c.curl, CURLOPT_HEADER, 1);
    //curl_easy_setopt(c.curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(c.curl, CURLOPT_NOSIGNAL, 1);
    // 设置重定向的最大次数
    curl_easy_setopt(c.curl, CURLOPT_MAXREDIRS, 5);
    // 设置301、302跳转跟随location
    curl_easy_setopt(c.curl, CURLOPT_FOLLOWLOCATION, 1);
    //设置进度回调函数
    if (progress) {
        curl_easy_setopt(c.curl, CURLOPT_NOPROGRESS, false);           //是否有进度相应函数
        curl_easy_setopt(c.curl, CURLOPT_PROGRESSFUNCTION, progress);  // CurlOptionProgressFunction
    }
    //curl_easy_getinfo(c.curl,  CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lFileSize);
    //curl_easy_setopt(c.curl, CURLOPT_PROGRESSDATA, g_hDlgWnd);
    //开始执行请求
    CURLcode retcCode = c.Perform();
    if (retcCode != CURLE_OK) {
    }
    //清理curl，和前面的初始化匹配
    curl_slist_free_all(list); /* free the list again */
    return retcCode;
}

int HttpClient::Download(const std::string &url, size_t begin, size_t end, DownloadBuffer &buffer) {
    EasyCURL c;
    if (!c.curl) {
        return CURLE_FAILED_INIT;
    }

    struct curl_slist *list = NULL;
    char buf_[64] = {0};
    snprintf(buf_, 64, "Range: bytes=%lu-%lu", begin, end);
    curl_easy_setopt(c.curl, CURLOPT_URL, url.c_str());
    list = curl_slist_append(list, buf_);
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYPEER, 0L);  //设置不验证证书
    curl_easy_setopt(c.curl, CURLOPT_SSL_VERIFYHOST, 0L);  //不验证host
    curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, list);
    //设置接收数据的回调
    curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION, buffer.callback);  // CurlOptionWriteFunction
    curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, &buffer);
    //curl_easy_setopt(c.curl, CURLOPT_TIMEOUT_MS , 10000);  //curl最长执行秒数

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
    }
    //清理curl，和前面的初始化匹配
    curl_slist_free_all(list); /* free the list again */
    return retcCode;
}
}  // namespace duobei
