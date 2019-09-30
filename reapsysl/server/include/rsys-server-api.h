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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        rsys-server-api.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        Server API
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 * 
 */
#ifndef __RSYS_SRVAPI_H__
#define __RSYS_SRVAPI_H__

#include <lwdlinked-list.h>

typedef enum _RSYS_USER_TYPE_FILTER
{
    RSYS_ANY_USER,
    RSYS_AD_USER,
    RSYS_LOCAL_USER,
} RSYS_USER_TYPE_FILTER;

typedef struct _RSYS_MESSAGE_PATTERN
{
    DWORD ulId;
    PSTR pszEventType; //"Success Audit" or "Failure Audit"
    PSTR pszRawMessageRegEx;
    DWORD ulUserMatchIndex;
    RSYS_USER_TYPE_FILTER filter;

    BOOLEAN bCompiled;
    regex_t compiledRegEx;
} RSYS_MESSAGE_PATTERN;

DWORD
RSysSrvApiInit(
    VOID
    );

DWORD
RSysSrvApiShutdown(
    VOID
    );

DWORD
RSysSrvGetEscrowTime(
    HANDLE hServer,
    DWORD* pdwEscrowTime
    );

DWORD
RSysSrvGetLogUnmatchedEvents(
    HANDLE hServer,
    BOOLEAN* pbLogUnmatchedErrorEvents,
    BOOLEAN* pbLogUnmatchedWarningEvents,
    BOOLEAN* pbLogUnmatchedInfoEvents
    );

DWORD
RSysSrvLockPatternList(
    HANDLE hServer,
    PLW_DLINKED_LIST* ppPatternList
    );

DWORD
RSysSrvUnlockPatternList(
    HANDLE hServer,
    PLW_DLINKED_LIST pPatternList
    );

DWORD
RSysSrvRefreshConfiguration(
    VOID
    );

#endif /* __RSYS_SRVAPI_H__ */

