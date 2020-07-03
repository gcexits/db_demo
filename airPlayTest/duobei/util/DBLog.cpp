#include "DBLog.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>
#include <sstream>
#include "Callback.h"

namespace duobei {

[[maybe_unused]] std::string getCurrentSystemTime() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date(64, 0);
    auto len = strftime(&date[0], date.size(), "%Y-%m-%d %H:%M:%S", localtime(&t));
    date.resize(len);
    return date;
}

[[maybe_unused]] void _log_internal(std::string& msg, const char* format, va_list args) {
    msg.resize(1024, 0);
    auto len = vsnprintf(&msg[0], msg.size(), format, args);
    msg.resize(len);
}

void log(LogLevel level, int line, const char* fn, const char* format, ...) noexcept {
#if defined(ENABLE_WRITE_LOG)
    if (level > 0) {
        return;
    }
    std::string msg;
    va_list args;
    va_start(args, format);
    _log_internal(msg, format, args);
    va_end(args);
    auto timestamp = getCurrentSystemTime();
    Callback::prettyLog(level, timestamp.c_str(), fn, line, msg.c_str());
#endif
}

}
