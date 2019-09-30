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
 *        lwconnectioninfo.h
 *
 * Abstract:
 *
 *        BeyondTrust Input/Output Subsystem (LWIO)
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
