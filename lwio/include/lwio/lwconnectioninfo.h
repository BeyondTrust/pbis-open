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
 *        lwconnectioninfo.h
 *
 * Abstract:
 *
 *        Likewise Input/Output Subsystem (LWIO)
 *
 *        SMB connections library definitions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LWCONNECTIONINFO_H_
#define _LWCONNECTIONINFO_H_

typedef union _CONNECTION_INFO_UNION
{
    PCONNECTION_INFO_0     p0;
    PCONNECTION_INFO_1     p1;
} CONNECTION_INFO_UNION, *PCONNECTION_INFO_UNION;


typedef struct _CONNECTION_INFO_ENUM_IN_PARAMS
{
    PWSTR  pwszQualifier;
    DWORD  dwLevel;
    DWORD  dwPreferredMaxLen;
    DWORD  dwResume;
} CONNECTION_INFO_ENUM_IN_PARAMS, *PCONNECTION_INFO_ENUM_IN_PARAMS;


typedef struct _CONNECTION_INFO_ENUM_OUT_PARAMS
{
    DWORD  dwLevel;
    DWORD  dwNumEntries;
    DWORD  dwTotalNumEntries;
    DWORD  dwResume;
} CONNECTION_INFO_ENUM_OUT_PARAMS, *PCONNECTION_INFO_ENUM_OUT_PARAMS;


LW_NTSTATUS
LwConnectionInfoMarshalEnumInputParameters(
    PCONNECTION_INFO_ENUM_IN_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    );


LW_NTSTATUS
LwConnectionInfoUnmarshalEnumInputParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PCONNECTION_INFO_ENUM_IN_PARAMS* ppParams
    );


LW_NTSTATUS
LwConnectionInfoFreeEnumInputParameters(
    PCONNECTION_INFO_ENUM_IN_PARAMS pParams
    );


LW_NTSTATUS
LwConnectionInfoMarshalEnumOutputParameters(
    PCONNECTION_INFO_ENUM_OUT_PARAMS pParams,
    PBYTE                            pBuffer,
    ULONG                            ulBufferSize,
    PULONG                           pulBytesUsed
    );


LW_NTSTATUS
LwConnectionInfoMarshalEnumOutputInfo_level_0(
    PCONNECTION_INFO_0  pConnectionInfo,
    PBYTE               pBuffer,
    ULONG               ulBufferSize,
    PULONG              pulBytesUsed
    );


LW_NTSTATUS
LwConnectionInfoMarshalEnumOutputInfo_level_1(
    PCONNECTION_INFO_1  pConnectionInfo,
    PBYTE               pBuffer,
    ULONG               ulBufferSize,
    PULONG              pulBytesUsed
    );


LW_NTSTATUS
LwConnectionInfoUnmarshalEnumOutputParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PCONNECTION_INFO_ENUM_OUT_PARAMS* ppParams,
    PCONNECTION_INFO_UNION*           ppConnectionInfo
    );


LW_NTSTATUS
LwConnectionInfoFreeEnumOutputParameters(
    PCONNECTION_INFO_ENUM_OUT_PARAMS pParams
    );


#endif /* _LWCONNECTIONINFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
