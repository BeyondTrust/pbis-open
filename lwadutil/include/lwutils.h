#ifndef __LWUTILS_H__
#define __LWUTILS_H__

//
// Logging support
//
extern LWUtilLogLevel gLWUtilMaxLogLevel;

#define LWUTIL_LOG_ALWAYS(szFmt...) \
         LWLogMessage(LWUTIL_LOG_LEVEL_ALWAYS, ## szFmt);

#define LWUTIL_LOG_ERROR(szFmt...) \
    if (gLWUtilMaxLogLevel >= LWUTIL_LOG_LEVEL_ERROR)      \
    {                                                     \
        LWLogMessage(LWUTIL_LOG_LEVEL_ERROR, ## szFmt);   \
    }

#define LWUTIL_LOG_WARNING(szFmt...) \
    if (gLWUtilMaxLogLevel >= LWUTIL_LOG_LEVEL_WARNING)    \
    {                                                     \
        LWLogMessage(LWUTIL_LOG_LEVEL_WARNING, ## szFmt); \
    }

#define LWUTIL_LOG_INFO(szFmt...) \
    if (gLWUtilMaxLogLevel >= LWUTIL_LOG_LEVEL_INFO)       \
    {                                                     \
        LWLogMessage(LWUTIL_LOG_LEVEL_INFO, ## szFmt);    \
    }

#define LWUTIL_LOG_VERBOSE(szFmt...) \
    if (gLWUtilMaxLogLevel >= LWUTIL_LOG_LEVEL_VERBOSE)    \
    {                                                     \
        LWLogMessage(LWUTIL_LOG_LEVEL_VERBOSE, ## szFmt); \
    }

#ifndef IsNullOrEmptyString

#define IsNullOrEmptyString(pszStr)             \
    (pszStr == NULL || *pszStr == '\0')

#endif

#define LW_SAFE_FREE_STRING(str) \
    do { if (str) { LWFreeString(str); (str) = NULL; } } while (0)

#define LW_SAFE_LOG_STR(s) ((s)?(s):"(null)")

#ifdef BAIL_ON_LWUTIL_ERROR
#undef BAIL_ON_LWUTIL_ERROR
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define BAIL_ON_LWUTIL_ERROR(dwError) \
    if (dwError) {                                                                                                  \
       LWUTIL_LOG_ERROR("Error %d (%s) at %s:%d", dwError, LW_SAFE_LOG_STR(GetErrorToString(dwError)), __FILE__, __LINE__); \
       goto error;                                                                                                  \
    }

#define BAIL_ON_LWUTIL_ERROR_NO_LOG(dwError) \
    if (dwError) { \
       goto error; \
    }

#define BAIL_ON_INVALID_HANDLE(h) \
    if (h == (HANDLE)NULL) { \
        dwError = EINVAL;                                                                                            \
        LWUTIL_LOG_ERROR("Error %d (%s) at %s:%d", dwError, LW_SAFE_LOG_STR(GetErrorToString(dwError)), __FILE__, __LINE__); \
        goto error;                                                                                                  \
    }

#define BAIL_ON_INVALID_POINTER(p) \
    if ((p) == NULL) { \
        dwError = EINVAL;                                                                                            \
        LWUTIL_LOG_ERROR("Error %d (%s) at %s:%d", dwError, LW_SAFE_LOG_STR(GetErrorToString(dwError)), __FILE__, __LINE__); \
        goto error;                                                                                                  \
    }

#if defined(WORDS_BIGENDIAN)
#define CONVERT_ENDIAN_DWORD(ui32val)           \
    ((ui32val & 0x000000FF) << 24 |             \
     (ui32val & 0x0000FF00) << 8  |             \
     (ui32val & 0x00FF0000) >> 8  |             \
     (ui32val & 0xFF000000) >> 24)

#define CONVERT_ENDIAN_WORD(ui16val)            \
    ((ui16val & 0x00FF) << 8 |                  \
     (ui16val & 0xFF00) >> 8)

#else
#define CONVERT_ENDIAN_DWORD(ui32val) (ui32val)
#define CONVERT_ENDIAN_WORD(ui16val) (ui16val)
#endif

/* ERRORS */
#define LWUTIL_ERROR_SUCCESS                                0x0000
#define LWUTIL_ERROR_NOT_IMPLEMENTED                        0xC001 // 49153
#define LWUTIL_ERROR_INVALID_PARAMETER                      0xC002 // 49154
#define LWUTIL_ERROR_NOT_SUPPORTED                          0xC003 // 49155
#define LWUTIL_ERROR_LOGON_FAILURE                          0xC004 // 49156
#define LWUTIL_ERROR_INVALID_NAME                           0xC005 // 49157
#define LWUTIL_ERROR_NULL_PARAMETER                         0xC006 // 49158
#define LWUTIL_ERROR_INVALID_TAG                            0xC007 // 49159
#define LWUTIL_ERROR_NO_SUCH_ATTRIBUTE                      0xC008 // 49160
#define LWUTIL_ERROR_INVALID_RECORD_TYPE                    0xC009 // 49161
#define LWUTIL_ERROR_INVALID_ATTRIBUTE_TYPE                 0xC00A // 49162
#define LWUTIL_ERROR_INSUFFICIENT_BUFFER                    0xC00B // 49163
#define LWUTIL_ERROR_IPC_FAILED                             0xC00C // 49164
#define LWUTIL_ERROR_NO_PROC_STATUS                         0xC00D // 49165
#define LWUTIL_ERROR_KRB5_PASSWORD_MISMATCH                 0xC00E // 49166
#define LWUTIL_ERROR_KRB5_PASSWORD_EXPIRED                  0xC00F // 49167
#define LWUTIL_ERROR_KRB5_CLOCK_SKEW                        0xC010 // 49168
#define LWUTIL_ERROR_KRB5_ERROR                             0xC011 // 49169
#define LWUTIL_ERROR_GSS_API_FAILED                         0xC012 // 49170
#define LWUTIL_ERROR_NO_SUCH_POLICY                         0xC013 // 49171
#define LWUTIL_ERROR_LDAP_OPEN                              0xC014 // 49172
#define LWUTIL_ERROR_LDAP_SET_OPTION                        0xC015 // 49173
#define LWUTIL_ERROR_LDAP_QUERY_FAILED                      0xC016 // 49174
#define LWUTIL_ERROR_COMMAND_FAILED                         0xC017 // 49175
#define LWUTIL_ERROR_UPN_NOT_FOUND                          0xC018 // 49176
#define LWUTIL_ERROR_LOLWUTIL_LIBRARY_FAILED                0xC019 // 49177
#define LWUTIL_ERROR_LOOKUP_SYMBOL_FAILED                   0xC01A // 49178
#define LWUTIL_ERROR_UID_NOT_FOUND                          0xC01B // 49179
#define LWUTIL_ERROR_MEMORY_ALLOC                           0xC01C // 49180
#define LWUTIL_ERROR_CREATE_FAILED                          0xC01D // 49181
#define LWUTIL_ERROR_WRITE_FAILED                           0xC01E // 49182
#define LWUTIL_ERROR_READ_FAILED                            0xC01F // 49183
#define LWUTIL_ERROR_ACCESS_DENIED                          0xC020 // 49184

#define LWUTIL_ERROR_MASK(_e_)                              (_e_ & 0xC000)

PCSTR
GetErrorToString(
    DWORD dwError
    );

VOID
LWSetLogHandler(
    HANDLE hLog,
    PFN_LWUTIL_LOG_MESSAGE pfnLogHandler,
    LWUtilLogLevel maxLogLevel
    );

VOID
LWResetLogHandler(
    VOID
    );

VOID
LWLogMessage(
    LWUtilLogLevel logLevel,
    PCSTR         pszFormat,
    ...
    );

DWORD
LWAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

DWORD
LWReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );

VOID
LWFreeMemory(
    PVOID pMemory
    );

DWORD
LWAllocateString(
    PCSTR pszInputString,
    PSTR * ppszOutputString
    );

VOID
LWFreeString(
    PSTR pszString
    );

DWORD
LWAllocateStringPrintf(
    PSTR* result,
    PCSTR format,
    ...
    );

DWORD
LWStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );


/* cfgparser.h */

typedef struct __NVPAIR {

    PSTR pszName;
    PSTR pszValue;

    struct __NVPAIR *pNext;

} NVPAIR, *PNVPAIR;

typedef struct __CFGSECTION {

    PSTR pszName;

    PNVPAIR pNVPairList;

    struct __CFGSECTION *pNext;

} CFGSECTION, *PCFGSECTION;

DWORD
LWParseConfigFile(
    PCSTR pszFilePath,
    PCFGSECTION* ppSectionList,
    BOOLEAN bWindowsDoubleByteFormat
    );

void
LWFreeConfigSectionList(
    PCFGSECTION pSectionList
    );

DWORD
LWSaveConfigSectionList(
    PCSTR pszConfigFilePath,
    PCFGSECTION pSectionList
    );

DWORD
LWGetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    );

DWORD
LWSetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    );


/* fileutils.h */

DWORD
LWRemoveFile(
    PCSTR pszPath
    );

DWORD
LWRemoveFiles(
    PSTR pszPath,
    BOOLEAN fDirectory,
    BOOLEAN fRecursive
    );

DWORD
LWRemoveDirectory(
    PCSTR pszPath
    );

DWORD
LWCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LWCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LWCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LWMoveFileAcrossDevices(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );


/* strutils.h */

void
LWStripWhitespace(
    PSTR pszString
    );


#endif /* __LWUTILS_H__ */
