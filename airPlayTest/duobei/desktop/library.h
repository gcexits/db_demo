#ifndef DBY_SDK_UNITY_LIBRARY_H
#define DBY_SDK_UNITY_LIBRARY_H

#if defined(CORE_CLI_TEST)
    #define EXPORT
    #define CALL
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define EXPORT __declspec(dllexport)
    #define CALL __cdecl
#else
    #define EXPORT
    #define CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

EXPORT int startDBY(char* uid_, wchar_t* path_, int width, int height);

EXPORT int stopDBY();

typedef void(CALL*onStatusCodeCallbackFunc)(int event);
EXPORT int registerStatusCodeCallback(onStatusCodeCallbackFunc statusCodeCallback);

typedef int (CALL *PrettyLogCallback)(int level, const char* timestamp, const char* fn, int line, const char* msg);
EXPORT int registerPrettyLogCallback(PrettyLogCallback callback);

#ifdef __cplusplus
}
#endif

#endif
