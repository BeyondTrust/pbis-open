/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Corporation 2004-2007
 * Copyright (C) BeyondTrust Software 2007
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

/* This structure captures the arguments that must be
 * sent to the Group Policy Service
 */
typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
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
    ACCESS_MASK dwAccessMask,
    BOOLEAN* pAllowed
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
