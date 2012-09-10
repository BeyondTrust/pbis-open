#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <lwmem.h>
#include <lwstr.h>

#define LOG(...) { PSTR pszMessage = NULL; if (0 == LwAllocateStringPrintf(&pszMessage, __VA_ARGS__)) { Log(pszMessage); LW_SAFE_FREE_STRING(pszMessage); } }

void
Log(
    const char *msg
    );

#endif
