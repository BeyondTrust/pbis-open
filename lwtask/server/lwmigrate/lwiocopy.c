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
 *        lwiocopy.c
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


DWORD g_dwSigUser1State = 0;  /* 0 -- untripped. 1 -- tripped. */

static HANDLE ghLsa = NULL;
static PSID gpDomainSid = NULL;
static PSID gpComputerSid = NULL;

DWORD g_dwFilesCopied = 0;
DWORD g_dwBytesCopied = 0;

static
DWORD
CopyFile_Internal(
    PCWSTR   pwszSrcPath,
    PCWSTR   pwszDestPath,
    BOOLEAN bCopyRecursive
    );

static
DWORD
CopyJustFile(
    IN PCWSTR pwszSourcePath,
    IN PCWSTR pwszTargetPath
    );

static
DWORD
CopyDir(
    IN PCWSTR pwszSourcePath,
    IN PCWSTR pwszTargetPath
    );

DWORD
PrintAbsoluteSD(
    HANDLE hLsa,
    PSID DomainSid,
    PSID ComputerSid,
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute
    )
{
    DWORD dwError = 0;
    PSID pGroupSid = NULL;
    BOOLEAN bIsGroupDefaulted = FALSE;
#if 0
    PACL pSacl = NULL;
#endif
    PSID pOwnerSid = NULL;
    BOOLEAN bIsOwnerDefaulted = FALSE;

    PACL pDacl = NULL;
    BOOLEAN bIsDaclPresent = FALSE;
    BOOLEAN bIsDaclDefaulted = FALSE;
    ULONG ulIndex = 0;
    PVOID pAce = NULL;
    PACCESS_ALLOWED_ACE pAllow = NULL;
    PACCESS_DENIED_ACE pDeny = NULL;
    PSID pSid = NULL;
    PWSTR pwszUser = NULL;
    BOOLEAN bFoundName = FALSE;
    PWSTR pwszAccessMask = NULL;

    dwError = LwNtStatusToWin32Error(
                RtlGetOwnerSecurityDescriptor(
                    pAbsolute,
                    &pOwnerSid,
                    &bIsOwnerDefaulted));
    BAIL_ON_ERROR(dwError);

    if (pOwnerSid)
    {
        dwError = MapSidToName(
                    hLsa,
                    DomainSid,
                    NULL,
                    pOwnerSid,
                    &pwszUser,
                    &bFoundName);
        BAIL_ON_ERROR(dwError);

        {
            PSTR pszMsg = NULL;
            dwError = LwWc16sToMbs(pwszUser, &pszMsg);
            if (!dwError)
            {
                fprintf(stderr, "Owner: %s\n", pszMsg);
                LW_SAFE_FREE_STRING(pszMsg);
            }
        }
        LW_SAFE_FREE_MEMORY(pwszUser);
    }

    dwError = LwNtStatusToWin32Error(
                RtlGetGroupSecurityDescriptor(
                    pAbsolute,
                    &pGroupSid,
                    &bIsGroupDefaulted));
    BAIL_ON_ERROR(dwError);

    if (pGroupSid)
    {
        dwError = MapSidToName(
                    hLsa,
                    DomainSid,
                    NULL,
                    pGroupSid,
                    &pwszUser,
                    &bFoundName);
        BAIL_ON_ERROR(dwError);

        {
            PSTR pszMsg = NULL;
            dwError = LwWc16sToMbs(pwszUser, &pszMsg);
            if (!dwError)
            {
                fprintf(stderr, "Group: %s\n", pszMsg);
                LW_SAFE_FREE_STRING(pszMsg);
            }
        }
        LW_SAFE_FREE_MEMORY(pwszUser);
    }

    dwError = LwNtStatusToWin32Error(
                RtlGetDaclSecurityDescriptor(
                    pAbsolute,
                    &bIsDaclPresent,
                    &pDacl,
                    &bIsDaclDefaulted));
    BAIL_ON_ERROR(dwError);

    if (pDacl)
    {
        for (ulIndex = 0; ulIndex < RtlGetAclAceCount(pDacl); ulIndex++)
        {
            RtlGetAce(pDacl, ulIndex, &pAce);

            switch(((PACE_HEADER) pAce)->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                pAllow = pAce;
                pSid = (PSID) &pAllow->SidStart;

                dwError = MapSidToName(
                            hLsa,
                            DomainSid,
                            NULL,
                            pSid,
                            &pwszUser,
                            &bFoundName);
                BAIL_ON_ERROR(dwError);

                dwError = MapFileAccessMaskToName(
                            pAllow->Mask,
                            &pwszAccessMask);
                BAIL_ON_ERROR(dwError);

                {
                    PSTR pszMsg = NULL;
                    PSTR pszAccess = NULL;

                    dwError = LwWc16sToMbs(pwszUser, &pszMsg);
                    if (!dwError)
                    {
                        dwError = LwWc16sToMbs(pwszAccessMask, &pszAccess);
                        if (!dwError)
                        {
                            fprintf(stderr, "Allowed: %s %s\n", pszMsg,
                                    pszAccess);
                            LW_SAFE_FREE_STRING(pszAccess);
                        }
                        LW_SAFE_FREE_STRING(pszMsg);
                    }
                }

                LW_SAFE_FREE_MEMORY(pwszUser);
                LW_SAFE_FREE_MEMORY(pwszAccessMask);

                break;

            case ACCESS_DENIED_ACE_TYPE:
                pDeny = pAce;
                pSid = (PSID) &pDeny->SidStart;

                dwError = MapSidToName(
                            hLsa,
                            DomainSid,
                            NULL,
                            pSid,
                            &pwszUser,
                            &bFoundName);
                BAIL_ON_ERROR(dwError);

                dwError = MapFileAccessMaskToName(
                            pDeny->Mask,
                            &pwszAccessMask);
                BAIL_ON_ERROR(dwError);

                {
                    PSTR pszMsg = NULL;
                    PSTR pszAccess = NULL;

                    dwError = LwWc16sToMbs(pwszUser, &pszMsg);
                    if (!dwError)
                    {
                        dwError = LwWc16sToMbs(pwszAccessMask, &pszAccess);
                        if (!dwError)
                        {
                            fprintf(stderr, "Denied: %s %s\n", pszMsg,
                                    pszAccess);
                            LW_SAFE_FREE_STRING(pszAccess);
                        }
                        LW_SAFE_FREE_STRING(pszMsg);
                    }
                }

                LW_SAFE_FREE_MEMORY(pwszUser);
                LW_SAFE_FREE_MEMORY(pwszAccessMask);

                break;
            default:
                break;
            }
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszUser);
    LW_SAFE_FREE_MEMORY(pwszAccessMask);

    return dwError;

error:

    goto cleanup;
}

DWORD
GetDomainSid(
    PSID *ppSid
    )
{
    DWORD dwError = 0;
    DWORD iCount = 0;
    HANDLE hLsa = NULL;
    PSID pSid = NULL;
    PLSASTATUS pLsaStatus = NULL;

    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_ERROR(dwError);

    dwError = LsaGetStatus(hLsa, &pLsaStatus);
    BAIL_ON_ERROR(dwError);

    for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
    {
        PLSA_AUTH_PROVIDER_STATUS pProviderStatus =
            &pLsaStatus->pAuthProviderStatusList[iCount];
        if (!strcmp(pProviderStatus->pszId, "lsa-activedirectory-provider"))
        {
            if (pProviderStatus->pTrustedDomainInfoArray)
            {
                PLSA_TRUSTED_DOMAIN_INFO pDomainInfo =
                    &pProviderStatus->pTrustedDomainInfoArray[0];

                dwError = LwNtStatusToWin32Error(
                            RtlAllocateSidFromCString(
                                &pSid,
                                pDomainInfo->pszDomainSID));
                BAIL_ON_ERROR(dwError);

                break;
            }
        }
    }

    *ppSid = pSid;

cleanup:

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
        pLsaStatus = NULL;
    }

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
GetMachineSid(
    PSID *ppSid
    );

DWORD
PrintStatus(
    )
{
    if (g_dwSigUser1State)
    {
        fprintf(stderr, "Files: %10lu\n", (unsigned long) g_dwFilesCopied);
        fprintf(stderr, "Bytes: %10lu\n", (unsigned long) g_dwBytesCopied);
        g_dwSigUser1State = 0;
    }
    return 0;
}

DWORD
CopyFile(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    DWORD dwError = 0;
    PWSTR pwszSrcPath = NULL;
    PWSTR pwszDestPath = NULL;

    LsaOpenServer(&ghLsa);

    PrintStatus();

    GetDomainSid(&gpDomainSid);

    PrintStatus();

    dwError = GetFilenameWithService(pszSrcPath, &pwszSrcPath);
    BAIL_ON_ERROR(dwError);

    dwError = GetFilenameWithService(pszDestPath, &pwszDestPath);
    BAIL_ON_ERROR(dwError);

    dwError = CopyFile_Internal(
                    pwszSrcPath,
                    pwszDestPath,
                    bCopyRecursive);
    BAIL_ON_ERROR(dwError);

error:

    if (ghLsa)
    {
        LsaCloseServer(ghLsa);
        ghLsa = NULL;
    }

    if (gpDomainSid)
    {
        LW_SAFE_FREE_MEMORY(gpDomainSid);
    }

    if (gpComputerSid)
    {
        LW_SAFE_FREE_MEMORY(gpComputerSid);
    }

    LW_SAFE_FREE_MEMORY(pwszSrcPath);
    LW_SAFE_FREE_MEMORY(pwszDestPath);

    return dwError;
}


static
DWORD
CopyFile_Internal(
    PCWSTR   pwszSrcPath,
    PCWSTR   pwszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsDirectory = FALSE;

    dwError = CheckPathIsDirectory(pwszSrcPath, &bIsDirectory);
    BAIL_ON_ERROR(dwError);

    if (bIsDirectory)
    {
        dwError = CopyDir(pwszSrcPath, pwszDestPath);
    }
    else
    {
        dwError = CopyJustFile(pwszSrcPath, pwszDestPath);
    }
    BAIL_ON_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
CopyJustFile(
    IN PCWSTR pwszSourcePath,
    IN PCWSTR pwszTargetPath
    )
{
    DWORD dwError = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    ULONG ulAbsSize = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pRelative = NULL;
    ULONG ulRelativeSize = 0;
    IO_FILE_HANDLE hSrcFile = NULL;
    IO_FILE_HANDLE hDstFile = NULL;

    BAIL_ON_NULL_POINTER(pwszSourcePath);
    BAIL_ON_NULL_POINTER(pwszTargetPath);

    dwError = OpenFile(
                    pwszSourcePath,
                    NULL,
                    NULL,
                    0,
                    READ_CONTROL | FILE_READ_ATTRIBUTES | FILE_READ_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,               /* Create disposition */
                    FILE_NON_DIRECTORY_FILE, /* Create options */
                    &hSrcFile);
    BAIL_ON_ERROR(dwError);
    
    PrintStatus();

    dwError = GetAbsoluteSD(
                    hSrcFile,
                    &pAbsolute,
                    &ulAbsSize,
                    &pRelative,
                    &ulRelativeSize);
    BAIL_ON_ERROR(dwError);

    dwError = PrintAbsoluteSD(ghLsa, gpDomainSid, gpComputerSid, pAbsolute);

    dwError = OpenFile(
                    pwszTargetPath,
                    pAbsolute,
                    pRelative,
                    ulRelativeSize,
                    DELETE | READ_CONTROL | WRITE_DAC | WRITE_OWNER |
                        FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA |
                        ACCESS_SYSTEM_SECURITY,
                    FILE_SHARE_READ,
                    FILE_SUPERSEDE,
                    FILE_NON_DIRECTORY_FILE,
                    &hDstFile);
    BAIL_ON_ERROR(dwError);

    PrintStatus();

    do
    {
        BYTE  szBuff[BUFF_SIZE];
        DWORD dwRead = 0;
        DWORD dwWrote = 0;

        dwError = ReadFile(
                        hSrcFile,
                        szBuff,
                        sizeof(szBuff),
                        &dwRead);
        BAIL_ON_ERROR(dwError);

        if (!dwRead)
        {
            break;
        }

        PrintStatus();

        dwError = WriteFile(
                            hDstFile,
                            szBuff,
                            dwRead,
                            &dwWrote);
        BAIL_ON_ERROR(dwError);
        g_dwBytesCopied += dwWrote;

        PrintStatus();
    } while(1);

    g_dwFilesCopied++;

    PrintStatus();

cleanup:

    if (hSrcFile)
    {
        LwNtCloseFile(hSrcFile);
    }

    if (hDstFile)
    {
        LwNtCloseFile(hDstFile);
    }

    LW_SAFE_FREE_MEMORY(pAbsolute);
    LW_SAFE_FREE_MEMORY(pRelative);

    return dwError;

error:

    goto cleanup;

}

static
DWORD
CopyDir(
    IN PCWSTR pwszSourcePath,
    IN PCWSTR pwszTargetPath
    )
{
    DWORD dwError = 0;
    NTSTATUS status = 0;
    BOOL bRestart = TRUE;
    IO_FILE_HANDLE hSrcFile = NULL;
    IO_FILE_HANDLE hDstFile = NULL;
    IO_STATUS_BLOCK ioStatus ;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
    PWSTR pwszTargetPathFileName = NULL;
    PWSTR pwszSourcePathFileName = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    ULONG ulAbsSize = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pRelative = NULL;
    ULONG ulRelativeSize = 0;

    BAIL_ON_NULL_POINTER(pwszSourcePath);
    BAIL_ON_NULL_POINTER(pwszTargetPath);

    dwError = OpenFile(
                        pwszSourcePath,
                        NULL,
                        NULL,
                        0,
                        READ_CONTROL | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN,           /* Create disposition */
                        FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT, /* Create options */
                        &hSrcFile);
    BAIL_ON_ERROR(dwError);

    dwError = GetAbsoluteSD(
                    hSrcFile,
                    &pAbsolute,
                    &ulAbsSize,
                    &pRelative,
                    &ulRelativeSize);
    BAIL_ON_ERROR(dwError);

    dwError = PrintAbsoluteSD(ghLsa, gpDomainSid, gpComputerSid, pAbsolute);

    dwError = OpenFile(
                    pwszTargetPath,
                    pAbsolute,
                    pRelative,
                    ulRelativeSize,
                    WRITE_DAC | WRITE_OWNER | FILE_WRITE_ATTRIBUTES |
                    ACCESS_SYSTEM_SECURITY | DELETE,
                    0,
                    FILE_OPEN_IF,
                    FILE_DIRECTORY_FILE,
                    &hDstFile);
    BAIL_ON_ERROR(dwError);

    LW_SAFE_FREE_MEMORY(pAbsolute);
    LW_SAFE_FREE_MEMORY(pRelative);

    for (;;)
    {
        status = LwNtQueryDirectoryFile(
            hSrcFile,                        /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                          /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,       /* Info level */
            FALSE,                              /* Do not return single entry */
            NULL,                               /* File spec */
            bRestart);                          /* Restart scan */

        switch (status)
        {
        case STATUS_NO_MORE_MATCHES:
            status = STATUS_SUCCESS;
            goto cleanup;
        default:
            dwError = LwNtStatusToWin32Error(status);
            BAIL_ON_ERROR(dwError);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            LW_SAFE_FREE_MEMORY(pwszSourcePathFileName);
            LW_SAFE_FREE_MEMORY(pwszTargetPathFileName);

            if ( pInfo->FileName[0] == '.' &&
                 (pInfo->FileName[1] == '\0' ||
                  (pInfo->FileName[1] == '.' && pInfo->FileName[2] == '\0')))
                continue;

            dwError = LwAllocateWc16sPrintfW(
                        &pwszSourcePathFileName,
                        L"%ws/%ws",
                        pwszSourcePath,
                        pInfo->FileName);
            BAIL_ON_ERROR(dwError);

            dwError = LwAllocateWc16sPrintfW(
                        &pwszTargetPathFileName,
                        L"%ws/%ws",
                        pwszTargetPath,
                        pInfo->FileName);
            BAIL_ON_ERROR(dwError);

            PrintStatus();

            if (pInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                fprintf(stderr, "\n\nreparse point\n\n");
            }
            else if(pInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                dwError = CopyDir(
                            pwszSourcePathFileName,
                            pwszTargetPathFileName);
                BAIL_ON_ERROR(dwError);
            }
            else
            {
                dwError = CopyJustFile(
                            pwszSourcePathFileName,
                            pwszTargetPathFileName);
                BAIL_ON_ERROR(dwError);
            }
        }
    }
   
    PrintStatus();

cleanup:

    if (hSrcFile)
    {
        LwNtCloseFile(hSrcFile);
    }

    if (hDstFile)
    {
        LwNtCloseFile(hDstFile);
    }

    LW_SAFE_FREE_MEMORY(pAbsolute);
    LW_SAFE_FREE_MEMORY(pRelative);
    LW_SAFE_FREE_MEMORY(pwszTargetPathFileName);
    LW_SAFE_FREE_MEMORY(pwszSourcePathFileName);

    return dwError;

error:

    goto cleanup;
}

