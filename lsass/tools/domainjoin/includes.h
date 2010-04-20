#ifndef DJ_INCLUDES
#define DJ_INCLUDES
#ifdef __LWADAM_MAKEFILE__
#include <lwbase/include/lw/types.h>
#include <lwbase/include/lw/rtlstring.h>
#include <lsass/include/lsa/lsa.h>
#include <lsass/include/lsa/ad.h>
#include <netlogon/include/lwnet.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>
#else
#include <lw/types.h>
#include <lw/rtlstring.h>
#include <lsa/lsa.h>
#include <lsa/ad.h>
#include <lwnet.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>
#endif

#ifndef BAIL_ON_DJ_ERROR
#define BAIL_ON_DJ_ERROR(err) \
    do { \
        if ((err) != STATUS_SUCCESS) { \
            goto error; \
        } \
    } while (0);
#endif
#endif


#ifndef DJ_SAFE_FREE_STRING
#define LWDJ_SAFE_FREE_STRING(str) \
        do {                         \
           if (str) {                \
              LwRtlCStringFree(&str); \
              (str) = NULL;          \
           }                         \
        } while(0);
#endif
