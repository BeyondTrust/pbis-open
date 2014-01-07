#ifndef LWTEST_DEFS
#define LWTEST_DEFS

#define LWT_DATA_CSV 1
#define LWT_DATA_LDIF 2


#define IsNullOrEmpty(str) ( !(str) || !(*(str)) )


#define BAIL(func) { dwError = func; if ( dwError ) goto error; }

#define BAIL_ON_LWT_ERROR(dwError) \
    if (dwError) {\
        char msg[256];\
        LwGetErrorString(dwError, msg, sizeof(msg));\
        fprintf(stderr, "Error at %s:%d [code: %d] %s\n", __FILE__, __LINE__, dwError, msg);\
        goto error;                 \
    }

#define BAIL_ON_TEST_BROKE(dwErrorCode) \
    if ( dwErrorCode != LW_ERROR_SUCCESS ) { \
        dwError = dwErrorCode; \
        if ( dwErrorCode != LW_ERROR_TEST_FAILED ) {\
            goto error; \
        }\
    }

#define LWT_LOG_TEST(msg) { \
    LwtLogTest( \
           __FUNCTION__,\
           __FILE__, \
           pszTestDescription, \
           pszTestAPIs, \
           dwError, \
           msg); \
}

#endif
