#ifndef __GPODEFINES_H__
#define __GPODEFINES_H__

#define GPO_FLAG_DISABLE	0x01
#define GPO_FLAG_FORCE		0x02

#define GPO_TRUE  1
#define GPO_FALSE 0

#ifndef WIN32

#define PATH_SEPARATOR_STR "/"

#else

#define PATH_SEPARATOR_STR "\\"

#endif

#if defined(HAVE_SOCKLEN_T) && defined(GETSOCKNAME_TAKES_SOCKLEN_T)
#    define SOCKLEN_T socklen_t
#else
#    define SOCKLEN_T int
#endif

#ifndef u_int32_t
#define u_int32_t uint32_t
#endif
#ifndef u_int16_t
#define u_int16_t uint16_t
#endif
#ifndef u_int8_t
#define u_int8_t uint8_t
#endif

#define MACHINE_GROUP_POLICY 1
#define USER_GROUP_POLICY    2

typedef struct __GPADBLBYTE {
    BYTE b1;
    BYTE b2;
} GPADBLBYTE, *PGPADBLBYTE;

#ifndef WIN32

#ifdef BAIL_ON_CENTERIS_ERROR
#undef BAIL_ON_CENTERIS_ERROR
#endif

/* Define one with logging involved */
#define BAIL_ON_CENTERIS_ERROR(ceError)                                 \
    if (ceError) {                                                      \
        GPA_LOG_ERROR("Error at %s:%d. Error code [0x%8x] (%s)",        \
                      __FILE__, __LINE__, ceError,                      \
                      CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown"); \
        goto error;                                                     \
    }

#ifdef BAIL_ON_NULL_POINTER
#undef BAIL_ON_NULL_POINTER
#endif

#define BAIL_ON_NULL_POINTER(p)                    \
        if (NULL == p) {                          \
           ceError = CENTERROR_INVALID_PARAMETER; \
           goto error;                            \
        }
#endif

#ifndef WIN32
#ifdef BAIL_ON_GPA_ERROR
#undef BAIL_ON_GPA_ERROR
#endif

/* Define one with logging involved */
#define BAIL_ON_GPA_ERROR(ceError)                                 \
    if (ceError) {                                                      \
        goto error;                                                     \
    }

#endif

#ifndef GOTO_CLEANUP

#define GOTO_CLEANUP() \
    do { goto cleanup; } while (0)

#endif

#ifndef GOTO_CLEANUP_EE

#define GOTO_CLEANUP_EE(EE) \
    do { (EE) = __LINE__; goto cleanup; } while (0)

#endif

#ifndef GOTO_CLEANUP_ON_CENTERROR

#define GOTO_CLEANUP_ON_CENTERROR(ceError) \
    do { if (ceError) goto cleanup; } while (0)

#endif

#ifndef GOTO_CLEANUP_ON_CENTERROR_EE

#define GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE) \
    do { if (ceError) { (EE) = __LINE__; goto cleanup; } } while (0)

#endif

/* Deprecated -- please use GOTO_CLEANUP versions */
#ifndef CLEANUP_ON_CENTERROR

#define CLEANUP_ON_CENTERROR(ceError) GOTO_CLEANUP_ON_CENTERROR(ceError)

#endif

#ifndef CLEANUP_ON_CENTERROR_EE

#define CLEANUP_ON_CENTERROR_EE(ceError, EE) GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE)

#endif

#ifndef IsNullOrEmptyString

#define IsNullOrEmptyString(pszStr)             \
    (pszStr == NULL || *pszStr == '\0')

#endif

#if defined(WORDS_BIGENDIAN)
#define GPA_CONVERT_ENDIAN_DWORD(ui32val)           \
    ((ui32val & 0x000000FF) << 24 |             \
     (ui32val & 0x0000FF00) << 8  |             \
     (ui32val & 0x00FF0000) >> 8  |             \
     (ui32val & 0xFF000000) >> 24)

#define GPA_CONVERT_ENDIAN_WORD(ui16val)            \
    ((ui16val & 0x00FF) << 8 |                  \
     (ui16val & 0xFF00) >> 8)

#else
#define GPA_CONVERT_ENDIAN_DWORD(ui32val) (ui32val)
#define GPA_CONVERT_ENDIAN_WORD(ui16val) (ui16val)
#endif

#ifndef IsNullOrEmptyString

#define IsNullOrEmptyString(pszStr)             \
    (pszStr == NULL || *pszStr == '\0')

#endif

#endif /* __GPODEFINES_H__ */
