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
 *        lmfileinfo.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        SMB file library definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *          Sriram Nambakam  (snambakam@likewise.com)
 */

#ifndef _LWFILEINFO_H_
#define _LWFILEINFO_H_

typedef union _FILE_INFO_UNION
{
    PFILE_INFO_2  p2;
    PFILE_INFO_3  p3;

} FILE_INFO_UNION, *PFILE_INFO_UNION;

typedef struct _FILE_INFO_ENUM_IN_PARAMS
{
    PWSTR           pwszBasepath;
    PWSTR           pwszUsername;
    DWORD           dwInfoLevel;
    DWORD           dwPreferredMaxLength;
    DWORD           dwEntriesRead;
    DWORD           dwTotalEntries;
    PDWORD          pdwResumeHandle;
} FILE_INFO_ENUM_IN_PARAMS, *PFILE_INFO_ENUM_IN_PARAMS;

typedef struct _FILE_INFO_ENUM_OUT_PREAMBLE
{
    DWORD  dwInfoLevel;
    DWORD  dwEntriesRead;
    DWORD  dwTotalEntries;
    PDWORD pdwResumeHandle;

} FILE_INFO_ENUM_OUT_PREAMBLE, *PFILE_INFO_ENUM_OUT_PREAMBLE;

typedef struct _FILE_INFO_GET_INFO_IN_PARAMS
{
    DWORD           dwInfoLevel;
    DWORD           dwFileId;
} FILE_INFO_GET_INFO_IN_PARAMS, *PFILE_INFO_GET_INFO_IN_PARAMS;

typedef struct _FILE_INFO_CLOSE_PARAMS
{
    DWORD              dwFileId;
} FILE_INFO_CLOSE_PARAMS, *PFILE_INFO_CLOSE_PARAMS;

LW_NTSTATUS
LwFileInfoMarshalEnumInputParameters(
    PFILE_INFO_ENUM_IN_PARAMS pParams,
    PBYTE*                    ppBuffer,
    ULONG*                    pulBufferSize
    );

LW_NTSTATUS
LwFileInfoUnmarshalEnumInputParameters(
    PBYTE                      pBuffer,
    ULONG                      ulBufferSize,
    PFILE_INFO_ENUM_IN_PARAMS* ppParams
    );

LW_NTSTATUS
LwFileInfoFreeEnumInputParameters(
    PFILE_INFO_ENUM_IN_PARAMS pParams
    );

LW_NTSTATUS
LwFileInfoMarshalEnumOutputPreamble(
    PBYTE                        pBuffer,
    ULONG                        ulBufferSize,
    PFILE_INFO_ENUM_OUT_PREAMBLE pPreamble,
    PULONG                       pulBytesUsed
    );

LW_NTSTATUS
LwFileInfoMarshalEnumOutputInfo_level_2(
    PFILE_INFO_2 pFileInfo,
    PBYTE        pBuffer,
    ULONG        ulBufferSize,
    PULONG       pulBytesUsed
    );

LW_NTSTATUS
LwFileInfoMarshalEnumOutputInfo_level_3(
    PFILE_INFO_3 pFileInfo,
    PBYTE        pBuffer,
    ULONG        ulBufferSize,
    PULONG       pulBytesUsed
    );

LW_NTSTATUS
LwFileInfoUnmarshalEnumOutputParameters(
    PBYTE                         pBuffer,
    ULONG                         ulBufferSize,
    PFILE_INFO_ENUM_OUT_PREAMBLE* ppPreamble,
    PFILE_INFO_UNION*             ppFileInfo
    );

LW_NTSTATUS
LwFileInfoFreeEnumOutPreamble(
    PFILE_INFO_ENUM_OUT_PREAMBLE pPreamble
    );

LW_NTSTATUS
LwFileInfoFree(
    DWORD            dwInfoLevel,
    DWORD            dwCount,
    PFILE_INFO_UNION pFileInfo
    );

LW_NTSTATUS
LwFileInfoMarshalGetInfoInParameters(
    PFILE_INFO_GET_INFO_IN_PARAMS pParams,
    PBYTE*                        ppBuffer,
    ULONG*                        pulBufferSize
    );

LW_NTSTATUS
LwFileInfoUnmarshalGetInfoInParameters(
    PBYTE                          pBuffer,
    ULONG                          ulBufferSize,
    PFILE_INFO_GET_INFO_IN_PARAMS* ppParams
    );

LW_NTSTATUS
LwFileInfoFreeGetInfoInParameters(
    PFILE_INFO_GET_INFO_IN_PARAMS pParams
    );

LW_NTSTATUS
LwFileInfoUnmarshalGetInfoOutParameters(
    PBYTE             pBuffer,
    ULONG             ulBufferSize,
    DWORD             dwInfoLevel,
    PFILE_INFO_UNION* ppFileInfo
    );

LW_NTSTATUS
LwFileInfoMarshalCloseParameters(
    PFILE_INFO_CLOSE_PARAMS pParams,
    PBYTE*                  ppBuffer,
    ULONG*                  pulBufferSize
    );

LW_NTSTATUS
LwFileInfoUnmarshalCloseParameters(
    PBYTE                    pBuffer,
    ULONG                    ulBufferSize,
    PFILE_INFO_CLOSE_PARAMS* ppParams
    );

#endif /* _LWFILEINFO_H_ */
