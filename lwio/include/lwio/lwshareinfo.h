/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwshareinfo.h
 *
 * Abstract:
 *
 *        BeyondTrust IO (LWIO)
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
