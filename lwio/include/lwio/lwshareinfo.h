/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        lwshareinfo.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        SMB shares library definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LWSHAREINFO_H_
#define _LWSHAREINFO_H_

#include <lwio/lmshare.h>

typedef union _SHARE_INFO_UNION
{
    PSHARE_INFO_0     p0;
    PSHARE_INFO_1     p1;
    PSHARE_INFO_2     p2;
    PSHARE_INFO_501   p501;
    PSHARE_INFO_502   p502;
    PSHARE_INFO_1005  p1005;
} SHARE_INFO_UNION, *PSHARE_INFO_UNION;


typedef struct _SHARE_INFO_ADD_PARAMS
{
    DWORD dwInfoLevel;
    SHARE_INFO_UNION info;
} SHARE_INFO_ADD_PARAMS, *PSHARE_INFO_ADD_PARAMS;

typedef struct _SHARE_INFO_DELETE_PARAMS
{
    PWSTR servername;
    PWSTR netname;
    DWORD reserved;
} SHARE_INFO_DELETE_PARAMS, *PSHARE_INFO_DELETE_PARAMS;

typedef struct _SHARE_INFO_ENUM_PARAMS
{
    DWORD dwInfoLevel;
    DWORD dwNumEntries;
    SHARE_INFO_UNION info;
} SHARE_INFO_ENUM_PARAMS, *PSHARE_INFO_ENUM_PARAMS;


typedef struct _SHARE_INFO_SETINFO_PARAMS
{
    PWSTR pwszNetname;
    DWORD dwInfoLevel;
    SHARE_INFO_UNION Info;
} SHARE_INFO_SETINFO_PARAMS, *PSHARE_INFO_SETINFO_PARAMS;


typedef struct _SHARE_INFO_GETINFO_PARAMS
{
    PWSTR pwszNetname;
    DWORD dwInfoLevel;
    SHARE_INFO_UNION Info;
} SHARE_INFO_GETINFO_PARAMS, *PSHARE_INFO_GETINFO_PARAMS;

LW_NTSTATUS
LwShareInfoMarshalAddParameters(
    PSHARE_INFO_ADD_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    );


LW_NTSTATUS
LwShareInfoUnmarshalAddParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_ADD_PARAMS* ppParams
    );


LW_NTSTATUS
LwShareInfoMarshalDeleteParameters(
    PSHARE_INFO_DELETE_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    );


LW_NTSTATUS
LwShareInfoUnmarshalDeleteParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_DELETE_PARAMS* ppParams
    );


LW_NTSTATUS
LwShareInfoMarshalEnumParameters(
    PSHARE_INFO_ENUM_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    );


LW_NTSTATUS
LwShareInfoUnmarshalEnumParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_ENUM_PARAMS* ppParams
    );


LW_NTSTATUS
LwShareInfoMarshalSetParameters(
    PSHARE_INFO_SETINFO_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    );


LW_NTSTATUS
LwShareInfoUnmarshalSetParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_SETINFO_PARAMS* ppParams
    );


LW_NTSTATUS
LwShareInfoMarshalGetParameters(
    PSHARE_INFO_GETINFO_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    );


LW_NTSTATUS
LwShareInfoUnmarshalGetParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSHARE_INFO_GETINFO_PARAMS* ppParams
    );

VOID
LwShareInfoFree(
    ULONG Level,
    ULONG Count,
    PVOID pInfo
    );

#endif /* _LWSHAREINFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
