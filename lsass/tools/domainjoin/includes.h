#include <lw/rtlstring.h>
#include <lsa/ad.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>

#define BAIL_ON_DJ_ERROR(err) \
    do { \
        if ((err) != STATUS_SUCCESS) { \
            goto error; \
        } \
    } while (0);

#define LWDJ_SAFE_FREE_STRING(str) \
        do {                         \
           if (str) {                \
              LwRtlCStringFree(&str); \
              (str) = NULL;          \
           }                         \
        } while(0);
