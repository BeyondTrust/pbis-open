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
 * Copyright (C) Likewise Software. All rights reserved.
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

