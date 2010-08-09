/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2010
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
 *        lmwksta.h
 *
 * Abstract:
 *
 *        Likewise Network Management API
 *
 *        LanMan API (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LM_WKSTA_H_
#define _LM_WKSTA_H_

#include <lw/security-types.h>

/*
 * WKSTA_USER_INFO levels
 */

typedef struct _WKSTA_USER_INFO_0
{
    PWSTR  wkui0_username;

} WKSTA_USER_INFO_0, *PWKSTA_USER_INFO_0;


typedef struct _WKSTA_USER_INFO_1
{
    PWSTR  wkui1_username;
    PWSTR  wkui1_logon_domain;
    PWSTR  wkui1_oth_domains;
    PWSTR  wkui1_logon_server;

} WKSTA_USER_INFO_1, *PWKSTA_USER_INFO_1;


NET_API_STATUS
NetWkstaUserEnum(
    PWSTR   pwszHostname,
    DWORD   dwLevel,
    PVOID  *ppBuffer,
    DWORD   dwPrefMaxLen,
    PDWORD  pdwNumEntries,
    PDWORD  pdwTotalNumEntries,
    PDWORD  pdwResume
    );


#endif /* _LM_WKSTA_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
