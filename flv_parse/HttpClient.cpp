#include "HttpClient.h"

static size_t defualtCallback(void *buffer, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

double HttpClient::getHttpFileSize(const std::string& url) {
    EasyCURL c;
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
        std::cout << "getHttpFileSize err = " << curl_easy_strerror(retCode) << std::endl;
        return 0;
    }

    double len = 0;
    retCode = curl_easy_getinfo(c.curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
    if (retCode != CURLE_OK) {
        std::cout << "getHttpFileSize err = " << curl_easy_strerror(retCode) << std::endl;
        return 0;
    }
    std::cout << "url = " << url << " size = " << len << std::endl;
    return len;
}