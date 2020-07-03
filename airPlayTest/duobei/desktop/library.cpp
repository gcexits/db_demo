/**
 * dby unity core library
 */
#include "library.h"

#include <string>
#include <vector>

#include "DBApi.h"

#define DUOBEI_LIVE_SDK

static bool DBYRuning = false;

int startDBY(char* uid_, wchar_t* path_, int width, int height) {
    if (DBYRuning) {
        return -2;
    }
    std::string uid = uid_;
    std::wstring path = path_;
    int statusCode = duobei::DBApi::getApi()->startApi(uid, path, width, height);
    if (statusCode != 0) {
        duobei::DBApi::getApi()->stopApi();
        return 0;
    }
    DBYRuning = true;
    return statusCode;
}

int stopDBY() {
    if (!DBYRuning) {
        return -2;
    }
    int statusCode = duobei::DBApi::getApi()->stopApi();
    DBYRuning = false;
    return statusCode;
}

static onStatusCodeCallbackFunc onStatusCodeCallback = nullptr;

void _onStatusCodeCallbackFunc(int event) {
    onStatusCodeCallback(event);
}

int registerStatusCodeCallback(onStatusCodeCallbackFunc statusCodeCallback) {
    onStatusCodeCallback = statusCodeCallback;
    using namespace std::placeholders;
    duobei::RegisterCallback::setstatusCodeCallCallback(std::bind(_onStatusCodeCallbackFunc, _1));
    return 0;
}

static PrettyLogCallback pretty_log_callback = nullptr;

int PrettyLogCallbackWrapper(int level, const char* timestamp, const char* fn, int line, const char* msg) {
    if (pretty_log_callback == nullptr) {
        return -1;
    }
    return pretty_log_callback(level, timestamp, fn, line, msg);
}

int registerPrettyLogCallback(PrettyLogCallback callback) {
    using namespace std::placeholders;
    pretty_log_callback = std::move(callback);
    duobei::RegisterCallback::setprettyLogCallback(std::bind(PrettyLogCallbackWrapper, _1, _2, _3, _4, _5));
    return 0;
}
