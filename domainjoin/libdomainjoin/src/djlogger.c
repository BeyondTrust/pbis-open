/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "domainjoin.h"

static const PSTR ERROR_TAG   = "ERROR";
static const PSTR WARN_TAG    = "WARNING";
static const PSTR INFO_TAG    = "INFO";
static const PSTR VERBOSE_TAG = "VERBOSE";

static const PSTR LOG_TIME_FORMAT = "%Y%m%d%H%M%S";

LOGINFO gdjLogInfo =
{
    LOG_LEVEL_ERROR,
    LOG_DISABLED,
    {"", NULL}
};

DWORD
dj_validate_log_level(
    DWORD dwLogLevel
    )
{
    DWORD ceError = ERROR_SUCCESS;

    if (dwLogLevel < 1) {
        ceError = ERROR_INVALID_PARAMETER;
    }

    return (ceError);
}

DWORD
dj_disable_logging()
{
    DWORD ceError = ERROR_SUCCESS;

    if (gdjLogInfo.logfile.logHandle != NULL &&
        gdjLogInfo.logfile.logHandle != stdout) {
        fclose(gdjLogInfo.logfile.logHandle);
    }

    gdjLogInfo.logfile.logHandle = NULL;
    gdjLogInfo.dwLogTarget = LOG_DISABLED;

    return ceError;
}

DWORD
dj_init_logging_to_console(
    DWORD dwLogLevel
    )
{
    DWORD ceError = ERROR_SUCCESS;

    if (gdjLogInfo.logfile.logHandle != NULL &&
        gdjLogInfo.logfile.logHandle != stdout) {
        fclose(gdjLogInfo.logfile.logHandle);
    }

    gdjLogInfo.logfile.logHandle = stdout;
    gdjLogInfo.dwLogTarget = LOG_TO_FILE;

    ceError = dj_set_log_level(dwLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    return ceError;
}

DWORD
dj_init_logging_to_file(
    DWORD dwLogLevel,
    PSTR  pszLogFilePath
    )
{
    DWORD ceError = ERROR_SUCCESS;

    gdjLogInfo.dwLogTarget = LOG_TO_FILE;

    strncpy(gdjLogInfo.logfile.szLogPath, pszLogFilePath, PATH_MAX);
    *(gdjLogInfo.logfile.szLogPath+PATH_MAX) = '\0';

    gdjLogInfo.logfile.logHandle = NULL;
    if (gdjLogInfo.logfile.szLogPath[0] != '\0') {
        ceError = CTOpenFile(gdjLogInfo.logfile.szLogPath, "a",
            &gdjLogInfo.logfile.logHandle);
        BAIL_ON_CENTERIS_ERROR(ceError);
        ceError = CTChangePermissions(gdjLogInfo.logfile.szLogPath, 0600);
        if(ceError == LwMapErrnoToLwError(EPERM))
        {
            //Some other user owns the file, so we can't change the
            //permissions.
            ceError = ERROR_SUCCESS;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);
        ceError = CTSetCloseOnExec(fileno(gdjLogInfo.logfile.logHandle));
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = dj_set_log_level(dwLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);

    return (ceError);

error:

    if (gdjLogInfo.logfile.logHandle != NULL) {
        fclose(gdjLogInfo.logfile.logHandle);
        gdjLogInfo.logfile.logHandle = NULL;
    }

    return (ceError);
}

DWORD
dj_init_logging_to_file_handle(
    DWORD dwLogLevel,
    FILE* handle
    )
{
    DWORD ceError = ERROR_SUCCESS;

    gdjLogInfo.dwLogTarget = LOG_TO_FILE;


    gdjLogInfo.logfile.logHandle = handle;

    ceError = dj_set_log_level(dwLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);

    return (ceError);

error:

    if (gdjLogInfo.logfile.logHandle != NULL) {
        fclose(gdjLogInfo.logfile.logHandle);
        gdjLogInfo.logfile.logHandle = NULL;
    }

    return (ceError);
}

void
dj_close_log()
{
    if (gdjLogInfo.logfile.logHandle != NULL &&
        gdjLogInfo.logfile.logHandle != stdout) {
        fclose(gdjLogInfo.logfile.logHandle);
        gdjLogInfo.logfile.logHandle = NULL;
    }
}

void
dj_log_to_file(
    DWORD dwLogLevel,
    PSTR pszFormat,
    va_list msgList
    )
{
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp;
    char timeBuf[256];

    switch (dwLogLevel)
    {
    case LOG_LEVEL_ALWAYS:
    {
        pszEntryType = INFO_TAG;
        break;
    }
    case LOG_LEVEL_ERROR:
    {
        pszEntryType = ERROR_TAG;
        break;
    }

    case LOG_LEVEL_WARNING:
    {
        pszEntryType = WARN_TAG;
        break;
    }

    case LOG_LEVEL_INFO:
    {
        pszEntryType = INFO_TAG;
        break;
    }

    default:
    {
        pszEntryType = VERBOSE_TAG;
        break;
    }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LOG_TIME_FORMAT, &tmp);

    fprintf(gdjLogInfo.logfile.logHandle, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(gdjLogInfo.logfile.logHandle, pszFormat, msgList);
    fprintf(gdjLogInfo.logfile.logHandle, "\n");
    fflush(gdjLogInfo.logfile.logHandle);
}

void
dj_log_message(
    DWORD dwLogLevel,
    PSTR pszFormat,...
    )
{
    va_list argp;

    if ( gdjLogInfo.dwLogTarget == LOG_DISABLED ||
         gdjLogInfo.logfile.logHandle == NULL ) {
        return;
    }

    if (gdjLogInfo.dwLogLevel < dwLogLevel) {
        return;
    }

    va_start(argp, pszFormat);

    dj_log_to_file(dwLogLevel, pszFormat, argp);

    va_end(argp);
}

DWORD
dj_set_log_level(
    DWORD dwLogLevel
    )
{
    DWORD ceError = ERROR_SUCCESS;

    ceError = dj_validate_log_level(dwLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);

    gdjLogInfo.dwLogLevel = dwLogLevel;

error:

    return (ceError);
}

DWORD DJLogException(
    DWORD dwLogLevel,
    const LWException *exc)
{
    PSTR exceptionString = NULL;
    DWORD ceError = ERROR_SUCCESS;

    ceError = LWExceptionToString(exc, NULL, TRUE, TRUE, &exceptionString);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dj_log_message(dwLogLevel, exceptionString);

error:
    CT_SAFE_FREE_STRING(exceptionString);
    return ceError;
}
