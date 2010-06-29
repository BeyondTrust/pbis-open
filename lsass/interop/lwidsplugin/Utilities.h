#ifndef __Utilities_h__
#define __Utilities_h__ 1

#ifdef __cplusplus
extern "C" {
#endif

long
LWIAllocateMemory(
    size_t dwSize,
    PVOID * ppMemory
    );

long
LWIReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    size_t dwSize
    );

long
LWIAllocateString(
    const char* pszInputString,
    char** ppszOutputString
    );

void
LWIFreeString(
    char* pszString
    );

#define CT_SAFE_FREE_STRING(str)                                        \
    do { if (str) { CTFreeString(str); (str) = NULL; } } while (0)

void
LWIFreeMemory(
    PVOID pMemory
    );

void LogMessageV(const char *Format, va_list Args);
void LogMessage(const char *Format, ...);
void LogBuffer(void* Buffer, int Length);
const char* StateToString(unsigned long State);
const char* TypeToString(unsigned long Type);
const char* MacErrorToString(long MacError);

#define BOOL_STRING(x) ((x) ? "Y" : "N")

#define SAFE_LOG_STR(s) ((s)?(s):"(null)")

#define _LOG(prefix, format, ...)                                       \
    LogMessage("[LWI] %s: %s(): " format, prefix, __FUNCTION__, ##__VA_ARGS__)

#define LOG(format, ...)                        \
    _LOG("     ", format, ##__VA_ARGS__)

#define TRY_CRASH() \
    ((*(char*)0) = 0)

#define NOP() \
    (0)

#define LOG_FATAL(format, ...) \
    do { \
        _LOG("FATAL", format, ##__VA_ARGS__); \
        TRY_CRASH(); \
    } while (0)

#define LOG_ERROR(format, ...)                  \
    _LOG("ERROR", format, ##__VA_ARGS__)

#define LOG_ENTER(format, ...)                  \
    _LOG("ENTER", format, ##__VA_ARGS__)

#define LOG_PARAM(format, ...)                  \
    _LOG("PARAM", format, ##__VA_ARGS__)

#define LOG_LEAVE(format, ...)                  \
    _LOG("LEAVE", format, ##__VA_ARGS__)

#define LOG_BUFFER(buffer, length) \
    LogBuffer(buffer, length)

#define GOTO_CLEANUP()                          \
    do {                                        \
        goto cleanup;                           \
    } while (0)

#define GOTO_CLEANUP_EE(EE)                     \
    do {                                        \
        (EE) = __LINE__;                        \
        GOTO_CLEANUP();                         \
    } while (0)

#define _LOG_GOTO_CLEANUP_ON_MACERROR(macError)                         \
    LOG_ERROR("Error %d (%s) at %s:%d", macError, SAFE_LOG_STR(MacErrorToString(macError)), __FILE__, __LINE__)

#define GOTO_CLEANUP_ON_MACERROR_EE(macError, EE)       \
    do {                                                \
        if (macError) {                                 \
            _LOG_GOTO_CLEANUP_ON_MACERROR(macError);    \
            GOTO_CLEANUP_EE(EE);                        \
        }                                               \
    } while (0)

#define GOTO_CLEANUP_ON_MACERROR(macError)              \
    do {                                                \
        if (macError) {                                 \
            _LOG_GOTO_CLEANUP_ON_MACERROR(macError);    \
            GOTO_CLEANUP();                             \
        }                                               \
    } while (0)

#define FlagOn(var, flag) \
            ((var) & (flag))

#ifdef __cplusplus
}
#endif

#endif
