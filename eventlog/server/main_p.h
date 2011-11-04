/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server (Process Utilities)
 *
 */
#ifndef __MAIN_P_H__
#define __MAIN_P_H__

typedef struct {
    /* Data as read from configuration */
    PSTR  configData;
    /* Data as parsed using LSASS */
    PVOID pAllowedTo;
} EVTALLOWEDDATA, *PEVTALLOWEDDATA;

/* This structure captures the arguments that must be
 * sent to the Group Policy Service
 */
typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
    /* Should start as daemon */
    DWORD dwStartAsDaemon;
    /* Log to syslog if this is true or starting as a daemon without a log
     * file. */
    BOOLEAN bLogToSyslog;
    /* How much logging do you want? */
    DWORD dwLogLevel;
    /* log file path */
    char szLogFilePath[PATH_MAX + 1];
    /* config file path */
    char szConfigFilePath[PATH_MAX + 1];
    /* Cache path */
    char szCachePath[PATH_MAX+1];
    /* Prefix path */
    char szPrefixPath[PATH_MAX+1];
    /* Process termination flag */
    BOOLEAN  bProcessShouldExit;
    /* Process Exit Code */
    DWORD dwExitCode;
    /* Flag to overrite any existing db   */
    BOOLEAN bReplaceDB;
    /* Maximum number of log file size in KB that is supported */
    DWORD dwMaxLogSize;
    /* Maximum number of records that can be hold*/
    DWORD dwMaxRecords;
    /* Remove the events older than*/
    DWORD dwMaxAge;
    /* Purge the records at the interval */
    DWORD dwPurgeInterval;
    /* Flag to prune database*/
    BOOLEAN bRemoveAsNeeded;
    /* Flag to Register TCP/IP RPC endpoints */
    BOOLEAN bRegisterTcpIp;

    /* Who is allowed to read, write, and delete events. The security
     * descriptor is set when all of the users/groups can be resolved. */
    PSTR pszAllowReadTo;
    PSTR pszAllowWriteTo;
    PSTR pszAllowDeleteTo;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAccess;
} EVTSERVERINFO, *PEVTSERVERINFO;

extern EVTSERVERINFO gServerInfo;

#define EVENTLOG_READ_RECORD    1
#define EVENTLOG_WRITE_RECORD   2
#define EVENTLOG_DELETE_RECORD  4

VOID
EVTFreeSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE pDescriptor
    );

DWORD
EVTGetConfigPath(
    PSTR* ppszPath
    );

DWORD
EVTGetCachePath(
    PSTR* ppszPath
    );

DWORD
EVTGetPrefixPath(
    PSTR* ppszPath
    );

DWORD
EVTCheckAllowed(
    PACCESS_TOKEN pUserToken,
    ACCESS_MASK dwAccessMask
    );

DWORD
EVTGetMaxRecords(
    DWORD* pdwMaxRecords
    );

DWORD
EVTGetRemoveAsNeeded(
    PBOOLEAN pbRemoveAsNeeded
    );

DWORD
EVTGetMaxAge(
    DWORD* pdwMaxLogSize
    );

DWORD
EVTGetMaxLogSize(
    DWORD* pdwMaxLogSize
    );

DWORD
EVTGetRemoveEventsFlag(
    PBOOLEAN pbRemoveEvents
    );

void
EVTUnlockServerInfo();

void
EVTFreeAllowData(
    PEVTALLOWEDDATA pAllowData
    );

DWORD
EVTSetAllowData(
    PCSTR   pszValue,
    PEVTALLOWEDDATA ppAllowData
    );

BOOLEAN
EVTProcessShouldExit();

void
EVTSetProcessShouldExit(
	BOOLEAN val
	);

DWORD
EVTServerMain(
    int argc,
    char* argv[]
    );

void
EVTServerExit(
    int retCode
    );

#endif /* __MAIN_P_H__ */
