
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
 *        copyutil.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Tool to copy files/directories
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
CheckPathIsDirectory(
    IN     PCWSTR    pwszPath,
    IN OUT PBOOLEAN pbIsDirectory
    )
{
    DWORD dwError = 0;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_STANDARD_INFORMATION fileStdInfo;

    dwError = LwNtStatusToWin32Error(
                OpenFile(
                    pwszPath,
                    NULL,
                    NULL,
                    0,
                    READ_CONTROL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    &hFile));
    BAIL_ON_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                LwNtQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    (PVOID*)&fileStdInfo,
                    sizeof(fileStdInfo),
                    FileStandardInformation));
    BAIL_ON_ERROR(dwError);

    *pbIsDirectory = fileStdInfo.Directory;

cleanup:

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    return dwError;

error:

    *pbIsDirectory = FALSE;

    goto cleanup;
}

DWORD
GetAbsoluteSD(
    HANDLE hFile,
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsolute,
    PULONG pulAbsSize,
    PSECURITY_DESCRIPTOR_RELATIVE *ppRelative,
    PULONG pulRelativeSize
    )
{
    DWORD dwError = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    ULONG ulOwnerSidSize = 0;
    ULONG ulGroupSidSize = 0;
    ULONG ulDaclSize = 0;
    ULONG ulSaclSize = 0;
    ULONG ulAbsSize = 0;
    IO_STATUS_BLOCK ioStatus = {0};
    BYTE securityBuffer[2048];
    PSECURITY_DESCRIPTOR_RELATIVE pRelative = NULL;
    ULONG ulRelativeSize = 0;

    dwError = LwNtStatusToWin32Error(
                LwNtQuerySecurityFile(
                    hFile,
                    NULL,
                    &ioStatus,
                    OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                    (PSECURITY_DESCRIPTOR_RELATIVE)(PBYTE)securityBuffer,
                    sizeof(securityBuffer)));
    BAIL_ON_ERROR(dwError);

    if (ppRelative)
    {
        ulRelativeSize = sizeof(securityBuffer);

        dwError = LwAllocateMemory(ulRelativeSize, (PVOID*)&pRelative);
        BAIL_ON_ERROR(dwError);

        memcpy(pRelative, securityBuffer, ulRelativeSize);
    }

    if (ppAbsolute)
    {
        /* Get required sizes */
        RtlSelfRelativeToAbsoluteSD(
            (PSECURITY_DESCRIPTOR_RELATIVE)securityBuffer,
            pAbsolute,
            &ulAbsSize,
            pDacl,
            &ulDaclSize,
            pSacl,
            &ulSaclSize,
            pOwnerSid,
            &ulOwnerSidSize,
            pGroupSid,
            &ulGroupSidSize);

        /* Allocate space */
        dwError = LwAllocateMemory(ulAbsSize, OUT_PPVOID(&pAbsolute));
        BAIL_ON_ERROR(dwError);

        if (ulDaclSize)
        {
            dwError = LwAllocateMemory(ulDaclSize, OUT_PPVOID(&pDacl));
            BAIL_ON_ERROR(dwError);
        }

        if (ulSaclSize)
        {
            dwError = LwAllocateMemory(ulSaclSize, OUT_PPVOID(&pSacl));
            BAIL_ON_ERROR(dwError);
        }

        if (ulOwnerSidSize)
        {
            dwError = LwAllocateMemory(ulOwnerSidSize, OUT_PPVOID(&pOwnerSid));
            BAIL_ON_ERROR(dwError);
        }

        if (ulGroupSidSize)
        {
            dwError = LwAllocateMemory(ulGroupSidSize, OUT_PPVOID(&pGroupSid));
            BAIL_ON_ERROR(dwError);
        }

        /* Unpack descriptor */
        dwError = LwNtStatusToWin32Error(
                     RtlSelfRelativeToAbsoluteSD(
                        (PSECURITY_DESCRIPTOR_RELATIVE)securityBuffer,
                        pAbsolute,
                        &ulAbsSize,
                        pDacl,
                        &ulDaclSize,
                        pSacl,
                        &ulSaclSize,
                        pOwnerSid,
                        &ulOwnerSidSize,
                        pGroupSid,
                        &ulGroupSidSize));
          BAIL_ON_ERROR(dwError);
    }

    if (ppAbsolute)
    {
        *ppAbsolute = pAbsolute;
        *pulAbsSize = ulAbsSize;
    }

    if (ppRelative)
    {
        *ppRelative = pRelative;
        *pulRelativeSize = ulRelativeSize;
    }

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pAbsolute);
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pSacl);
    LW_SAFE_FREE_MEMORY(pOwnerSid);
    LW_SAFE_FREE_MEMORY(pGroupSid);
    pAbsolute = NULL;
    ulAbsSize = 0;
    LW_SAFE_FREE_MEMORY(pRelative);
    pRelative = NULL;
    ulRelativeSize = 0;

    goto cleanup;
}

VOID
FreeAbsoluteSD(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL))
    {
        return;
    }

    pSecDesc = *ppSecDesc;

    ntStatus = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    ntStatus = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);

    ntStatus = RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    ntStatus = RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    RTL_FREE(&pSecDesc);
    RTL_FREE(&pOwner);
    RTL_FREE(&pGroup);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}

DWORD
GetFilenameWithService(
    PCSTR pszPath,
    PWSTR* ppwszFilename
    )
{
    DWORD dwError = 0;
    PWSTR pwszFilename = NULL;

    if ((pszPath[0] == '/' && pszPath[1] == '/') ||
        (pszPath[0] == '\\' && pszPath[1] == '\\'))
    {
        dwError = LwAllocateWc16sPrintfW(
                    &pwszFilename,
                    L"/rdr%s",
                    pszPath + 1);
        BAIL_ON_ERROR(dwError);
    }
    else if (pszPath[0] == '/')
    {
        dwError = LwAllocateWc16sPrintfW(
                    &pwszFilename,
                    L"/pvfs%s",
                    pszPath);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_ERROR(dwError);
    }

    *ppwszFilename = pwszFilename;

cleanup:

    return dwError;

error:

    *ppwszFilename = NULL;

    goto cleanup;
}

DWORD
OpenFile(
    IN  PCWSTR          pwszFileName,
    IN  PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute,
    IN  PSECURITY_DESCRIPTOR_RELATIVE pRelative,
    IN  ULONG           ulRelativeLength,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    )
{
    DWORD dwError = 0;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;

    BAIL_ON_NULL_POINTER(pwszFileName);

    filename.FileName = (PWSTR) pwszFileName;

    {
        PSTR pszMsg = NULL;
        dwError = LwWc16sToMbs(pwszFileName, &pszMsg);
        if (!dwError)
        {
            fprintf(stderr, "%s\n", pszMsg);
            LW_SAFE_FREE_STRING(pszMsg);
        }
    }

    dwError = LwNtStatusToWin32Error(
                LwNtCreateFile(
                &handle,                 /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                &filename,               /* Filename */
                NULL,//pAbsolute,               /* Security descriptor */
                NULL,                    /* Security QOS */
                ulDesiredAccess,         /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                ulShareAccess,           /* Share access */
                ulCreateDisposition,     /* Create disposition */
                ulCreateOptions,         /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                   /* ECP list */
    BAIL_ON_ERROR(dwError);

    *phFile = handle;

    if (pRelative && ulRelativeLength > 0)
    {
        /*dwError =*/ LwNtStatusToWin32Error(
                   LwNtSetSecurityFile(
                        handle,
                        NULL,
                        &ioStatus,
                        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
                        pRelative,
                        ulRelativeLength));
    }

cleanup:

    return dwError;

error:

    if (handle)
        LwNtCloseFile(handle);
    handle = NULL;

    goto cleanup;
}

DWORD
ReadFile(
    IN HANDLE hFile,
    OUT PVOID pBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus;

    status = LwNtReadFile(
        hFile,                               // File handle
        NULL,                                // Async control block
        &ioStatus,                           // IO status block
        pBuffer,                             // Buffer
        (ULONG) dwNumberOfBytesToRead,       // Buffer size
        NULL,                                // File offset
        NULL);                               // Key
    if (status == STATUS_END_OF_FILE)
    {
        status = 0;;
    }
    dwError = LwNtStatusToWin32Error(status);
    BAIL_ON_ERROR(dwError);

    *pdwBytesRead = (int) ioStatus.BytesTransferred;

cleanup:

    return dwError;

error:

    *pdwBytesRead = 0;
    pBuffer = NULL;

    goto cleanup;
}

DWORD
WriteFile(
    IN HANDLE hFile,
    IN PVOID pBuffer,
    IN DWORD dwNumBytesToWrite,
    OUT PDWORD pdwNumBytesWritten
    )
{
    DWORD dwError = 0;
    IO_STATUS_BLOCK ioStatus ;

    BAIL_ON_NULL_POINTER(pBuffer);

    dwError = LwNtStatusToWin32Error(
                LwNtWriteFile(
                    hFile,                                 // File handle
                    NULL,                                 // Async control block
                    &ioStatus,                             // IO status block
                    pBuffer,                             // Buffer
                    (ULONG) dwNumBytesToWrite,             // Buffer size
                    NULL,                                 // File offset
                    NULL));                                 // Key
    BAIL_ON_ERROR(dwError);

    *pdwNumBytesWritten = (int) ioStatus.BytesTransferred;

cleanup:

    return dwError;

error:

    *pdwNumBytesWritten = 0;

    goto cleanup;

}

