/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nfssvcutils.h
 *
 * Abstract:
 *
 *        Likewise Server Service Service (LWNFSSVC)
 *
 *        Public header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __NFSSVCUTILS_H__
#define __NFSSVCUTILS_H__

#define IsNullOrEmptyString(pszStr)     \
    (pszStr == NULL || *pszStr == '\0')

#define NFSSVC_SAFE_FREE_STRING(str) \
        do {                      \
           if (str) {             \
              NfsSvcFreeString(str); \
              (str) = NULL;       \
           }                      \
        } while(0);

/*
 * Logging
 */

typedef enum
{
    NFSSVC_LOG_LEVEL_ALWAYS = 0,
    NFSSVC_LOG_LEVEL_ERROR,
    NFSSVC_LOG_LEVEL_WARNING,
    NFSSVC_LOG_LEVEL_INFO,
    NFSSVC_LOG_LEVEL_VERBOSE,
    NFSSVC_LOG_LEVEL_DEBUG
} NFSSVC_LOG_LEVEL;

typedef enum
{
    NFSSVC_LOG_TARGET_DISABLED = 0,
    NFSSVC_LOG_TARGET_CONSOLE,
    NFSSVC_LOG_TARGET_FILE,
    NFSSVC_LOG_TARGET_SYSLOG
} NFSSVC_LOG_TARGET;

typedef VOID (*PFN_NFSSVC_LOG_MESSAGE)(
                            HANDLE           hLog,
                            NFSSVC_LOG_LEVEL logLevel,
                            PCSTR            pszFormat,
                            va_list          msgList
                            );

typedef struct _NFSSVC_LOG_INFO
{
    NFSSVC_LOG_LEVEL  maxAllowedLogLevel;
    NFSSVC_LOG_TARGET logTarget;
    PSTR              pszPath;
} NFSSVC_LOG_INFO, *PNFSSVC_LOG_INFO;

#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gNfsSvcLogLock;

#define NFSSVC_LOCK_LOGGER   pthread_mutex_lock(&gNfsSvcLogLock)
#define NFSSVC_UNLOCK_LOGGER pthread_mutex_unlock(&gNfsSvcLogLock)

#define _NFSSVC_LOG_PREFIX_THREAD(Format) \
    "0x%lx:" Format, ((unsigned long)pthread_self())

#else

#define NFSSVC_LOCK_LOGGER
#define NFSSVC_UNLOCK_LOGGER

#define _NFSSVC_LOG_PREFIX_THREAD(Format) \
    Format

#endif

#define _NFSSVC_LOG_PREFIX_LOCATION(Format, Function, File, Line) \
    _NFSSVC_LOG_PREFIX_THREAD("[%s() %s:%d] " Format), \
    (Function), \
    (File), \
    (Line)

#define _NFSSVC_LOG_WITH_THREAD(Level, Format, ...) \
    _NFSSVC_LOG_MESSAGE(Level, \
                      _NFSSVC_LOG_PREFIX_THREAD(Format), \
                      ## __VA_ARGS__)

#define _NFSSVC_LOG_WITH_LOCATION(Level, Format, Function, File, Line, ...) \
    _NFSSVC_LOG_MESSAGE(Level, \
                  _NFSSVC_LOG_PREFIX_LOCATION(Format, Function, File, Line), \
                  ## __VA_ARGS__)

#define _NFSSVC_LOG_WITH_DEBUG(Level, Format, ...) \
    _NFSSVC_LOG_WITH_LOCATION(Level, Format, \
                            __FUNCTION__, __FILE__, __LINE__, \
                            ## __VA_ARGS__)

extern HANDLE                 ghNfsSvcLog;
extern NFSSVC_LOG_LEVEL       gNfsSvcMaxLogLevel;
extern PFN_NFSSVC_LOG_MESSAGE gpfnNfsSvcLogger;

#define _NFSSVC_LOG_MESSAGE(Level, Format, ...) \
    NfsSvcLogMessage(gpfnNfsSvcLogger, ghNfsSvcLog, Level, Format, ## __VA_ARGS__)

#define _NFSSVC_LOG_IF(Level, Format, ...)                              \
    do {                                                                \
        NFSSVC_LOCK_LOGGER;                                             \
        if (gpfnNfsSvcLogger && (gNfsSvcMaxLogLevel >= (Level)))        \
        {                                                               \
            if (gNfsSvcMaxLogLevel >= NFSSVC_LOG_LEVEL_DEBUG)           \
            {                                                           \
                _NFSSVC_LOG_WITH_DEBUG(Level, Format, ## __VA_ARGS__);  \
            }                                                           \
            else                                                        \
            {                                                           \
                _NFSSVC_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
            }                                                           \
        }                                                               \
        NFSSVC_UNLOCK_LOGGER;                                           \
    } while (0)

#define NFSSVC_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define NFSSVC_LOG_ALWAYS(szFmt, ...) \
    _NFSSVC_LOG_IF(NFSSVC_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define NFSSVC_LOG_ERROR(szFmt, ...) \
    _NFSSVC_LOG_IF(NFSSVC_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define NFSSVC_LOG_WARNING(szFmt, ...) \
    _NFSSVC_LOG_IF(NFSSVC_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define NFSSVC_LOG_INFO(szFmt, ...) \
    _NFSSVC_LOG_IF(NFSSVC_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define NFSSVC_LOG_VERBOSE(szFmt, ...) \
    _NFSSVC_LOG_IF(NFSSVC_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define NFSSVC_LOG_DEBUG(szFmt, ...) \
    _NFSSVC_LOG_IF(NFSSVC_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)

#define NFSSVC_LOG_TRACE(szFmt, ...) \
    _NFSSVC_LOG_IF(NFSSVC_LOG_LEVEL_TRACE, szFmt, ## __VA_ARGS__)

typedef struct _LOGFILEINFO {
    CHAR szLogPath[PATH_MAX+1];
    FILE* logHandle;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _SYSLOGINFO {
    CHAR szIdentifier[PATH_MAX+1];
    DWORD dwOption;
    DWORD dwFacility;
} SYSLOGINFO, *PSYSLOGINFO;

typedef struct _LOGINFO {
    pthread_mutex_t lock;
    DWORD dwLogLevel;
    DWORD logTarget;
    union _logdata {
        LOGFILEINFO logfile;
        SYSLOGINFO syslog;
    } data;
    BOOLEAN  bLoggingInitiated;
} LOGINFO, *PLOGINFO;

DWORD
NfsSvcNfsAllocateMemory(
    DWORD  dwSize,
    PVOID* ppMemory
    );

void
NfsSvcNfsFreeMemory(
    PVOID pMemory
    );

DWORD
NfsSvcStrndup(
    PCSTR  pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

VOID
NfsSvcFreeString(
    PSTR pszString
    );

BOOLEAN
NfsSvcIsWhiteSpace(
    char c
    );

/*
 * Modify PSTR in-place to convert sequences
 * of whitespace characters into
 * single spaces (0x20)
 */
DWORD
NfsSvcCompressWhitespace(
    PSTR pszString
    );

/*
 * Convert a 16-bit string to an 8-bit string
 * Allocate new memory in the process
 */
DWORD
NfsSvcLpwStrToLpStr(
    PCWSTR pszwString,
    PSTR*  ppszString
    );

VOID
NfsSvcStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

VOID
NfsSvcStrToUpper(
    PSTR pszString
    );

VOID
NfsSvcStrToLower(
    PSTR pszString
    );

DWORD
NfsSvcAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

DWORD
NfsSvcAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    );

DWORD
NfsSvcRemoveFile(
    PCSTR pszPath
    );

DWORD
NfsSvcCheckFileExists(
    PCSTR    pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
NfsSvcMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
NfsSvcChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
NfsSvcChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
NfsSvcChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
NfsSvcGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
NfsSvcChangeDirectory(
    PCSTR pszPath
    );

DWORD
NfsSvcRemoveDirectory(
    PCSTR pszPath
    );

DWORD
NfsSvcGetFileSize(
	PCSTR pszPath,
	PDWORD pdwFileSize
	);

DWORD
NfsSvcCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
NfsSvcCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
NfsSvcGetHostname(
    PSTR* ppszHostname
    );

DWORD
NfsSvcInitLogging(
    PCSTR             pszProgramName,
    NFSSVC_LOG_TARGET logTarget,
    NFSSVC_LOG_LEVEL  maxAllowedLogLevel,
    PCSTR             pszPath
    );

DWORD
NfsSvcLogGetInfo(
    PNFSSVC_LOG_INFO* ppLogInfo
    );

DWORD
NfsSvcLogSetInfo(
    PNFSSVC_LOG_INFO pLogInfo
    );

VOID
NfsSvcLogMessage(
    PFN_NFSSVC_LOG_MESSAGE pfnLogger,
    HANDLE                 hLog,
    NFSSVC_LOG_LEVEL       logLevel,
    PCSTR                  pszFormat,
    ...
    );

DWORD
NfsSvcShutdownLogging(
    VOID
    );

DWORD
NfsSvcValidateLogLevel(
    DWORD dwLogLevel
    );

#endif /* __NFSSVCUTILS_H__ */
