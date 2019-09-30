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
 *        lwsessioninfo.h
 *
 * Abstract:
 *
 *        BeyondTrust IO (LWIO)
 *
 *        SMB session library definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *          Sriram Nambakam  (snambakam@likewise.com)
 */

#ifndef _LWSESSIONINFO_H_
#define _LWSESSIONINFO_H_

typedef union _SESSION_INFO_UNION
{
    PSESSION_INFO_0   p0;
    PSESSION_INFO_1   p1;
    PSESSION_INFO_2   p2;
    PSESSION_INFO_10  p10;
    PSESSION_INFO_502 p502;

} SESSION_INFO_UNION, *PSESSION_INFO_UNION;

typedef struct _SESSION_INFO_ENUM_IN_PARAMS
{
    PWSTR  pwszServername;
    PWSTR  pwszUncClientname;
    PWSTR  pwszUsername;
    DWORD  dwInfoLevel;
    DWORD  dwPreferredMaxLength;
    PDWORD pdwResumeHandle;

} SESSION_INFO_ENUM_IN_PARAMS, *PSESSION_INFO_ENUM_IN_PARAMS;

typedef struct _SESSION_INFO_ENUM_OUT_PREAMBLE
{
    DWORD  dwInfoLevel;
    DWORD  dwEntriesRead;
    DWORD  dwTotalEntries;
    PDWORD pdwResumeHandle;

} SESSION_INFO_ENUM_OUT_PREAMBLE, *PSESSION_INFO_ENUM_OUT_PREAMBLE;

typedef struct _SESSION_INFO_DELETE_PARAMS
{
    PWSTR pwszServername;
    PWSTR pwszUncClientname;
    PWSTR pwszUncUsername;
} SESSION_INFO_DELETE_PARAMS, *PSESSION_INFO_DELETE_PARAMS;

LW_NTSTATUS
LwSessionInfoMarshalEnumInputParameters(
    PSESSION_INFO_ENUM_IN_PARAMS pParams,
    PBYTE*                       ppBuffer,
    ULONG*                       pulBufferSize
    );

LW_NTSTATUS
LwSessionInfoUnmarshalEnumInputParameters(
    PBYTE                         pBuffer,
    ULONG                         ulBufferSize,
    PSESSION_INFO_ENUM_IN_PARAMS* ppParams
    );

LW_NTSTATUS
LwSessionInfoFreeEnumInputParameters(
    PSESSION_INFO_ENUM_IN_PARAMS pParams
    );

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputPreamble(
    PBYTE                           pBuffer,
    ULONG                           ulBufferSize,
    PSESSION_INFO_ENUM_OUT_PREAMBLE pPreamble,
    PULONG                          pulBytesUsed
    );

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_0(
    PSESSION_INFO_0 pSessionInfo,
    PBYTE           pBuffer,
    ULONG           ulBufferSize,
    PULONG          pulBytesUsed
    );

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_1(
    PSESSION_INFO_1 pSessionInfo,
    PBYTE           pBuffer,
    ULONG           ulBufferSize,
    PULONG          pulBytesUsed
    );

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_2(
    PSESSION_INFO_2 pSessionInfo,
    PBYTE           pBuffer,
    ULONG           ulBufferSize,
    PULONG          pulBytesUsed
    );

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_10(
    PSESSION_INFO_10 pSessionInfo,
    PBYTE            pBuffer,
    ULONG            ulBufferSize,
    PULONG           pulBytesUsed
    );

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_502(
    PSESSION_INFO_502 pSessionInfo,
    PBYTE             pBuffer,
    ULONG             ulBufferSize,
    PULONG            pulBytesUsed
    );

LW_NTSTATUS
LwSessionInfoUnmarshalEnumOutputParameters(
    PBYTE                            pBuffer,
    ULONG                            ulBufferSize,
    PSESSION_INFO_ENUM_OUT_PREAMBLE* ppPreamble,
    PSESSION_INFO_UNION*             ppSessionInfo
    );

LW_NTSTATUS
LwSessionInfoFreeEnumOutPreamble(
    PSESSION_INFO_ENUM_OUT_PREAMBLE pPreamble
    );

LW_NTSTATUS
LwSessionInfoFree(
    DWORD               dwInfoLevel,
    DWORD               dwCount,
    PSESSION_INFO_UNION pSessionInfo
    );

LW_NTSTATUS
LwSessionInfoMarshalDeleteParameters(
    PSESSION_INFO_DELETE_PARAMS pParams,
    PBYTE*                      ppBuffer,
    ULONG*                      pulBufferSize
    );

LW_NTSTATUS
LwSessionInfoUnmarshalDeleteParameters(
    PBYTE                        pBuffer,
    ULONG                        ulBufferSize,
    PSESSION_INFO_DELETE_PARAMS* ppParams
    );

#endif /* _LWSESSIONINFO_H_ */

