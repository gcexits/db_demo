#pragma once

namespace duobei {
enum LogLevel {
    Fatal = -1,
    Error = 0,
    Warning = 1,
    Info = 2,
    Verbose = 3,
    Debug = 4,
    Trace = 5,
    Dev = 6
};
void log(LogLevel level, int line, const char* fn, const char* format, ...) noexcept;
}

#define ENABLE_WRITE_LOG

#if defined(ENABLE_WRITE_LOG)
#define WriteErrorLog(msg, ...) duobei::log(duobei::Error, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#define WriteWarnLog(msg, ...) duobei::log(duobei::Warning, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#define WriteInfoLog(msg, ...) duobei::log(duobei::Info, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#define WriteDebugLog(msg, ...) duobei::log(duobei::Debug, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#else
#    define WriteErrorLog(msg, ...) (void)0
#    define WriteWarnLog(msg, ...) (void)0
#    define WriteInfoLog(msg, ...) (void)0
#    define WriteDebugLog(msg, ...) (void)0
#endif
