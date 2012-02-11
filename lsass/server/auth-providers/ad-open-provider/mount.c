/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright BeyondTrust Software
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
 * BEYOND TRUST SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH BEYOND TRUST SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY BEYOND TRUST SOFTWARE, PLEASE CONTACT BEYOND TRUST SOFTWARE
 * AT ssalley@beyondtrust.com
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        mount.c
 *
 * Abstract:
 *
 *        Mount user specific file systems.
 *
 */

#include "adprovider.h"

static
DWORD
ConvertSlashes(
    PCSTR pszPath,
    PSTR *ppszConvertedPath
    )
{
    DWORD dwError = 0;
    PSTR pszConvertedPath = NULL;
    size_t i;

    dwError = LwAllocateString(pszPath, &pszConvertedPath);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; pszConvertedPath[i]; i++)
    {
        if (pszConvertedPath[i] == '\\')
        {
            pszConvertedPath[i] = '/';
        }
    }

    *ppszConvertedPath = pszConvertedPath;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszConvertedPath);
    goto cleanup;
}

static
DWORD
ResolveRemotePath(
    PCSTR pszPath,
    PSTR *ppszPhysicalPath
    )
{
    DWORD dwError = 0;
    PWSTR pwszRemotePath = NULL;
    PSTR pszPhysicalPathWrongSlashes = NULL;
    PSTR pszPhysicalPathRightSlashes = NULL;
    PSTR pszPhysicalPath = NULL;
    PWSTR pwszPhysicalPath = NULL;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatus;

    if (pszPath[0] == '/')
    {
        if (pszPath[1] == '/')
        {
            dwError = LwAllocateWc16sPrintfW(
                            &pwszRemotePath,
                            L"/rdr%s",
                            pszPath + 1);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LwAllocateWc16sPrintfW(
                            &pwszRemotePath,
                            L"/rdr%s",
                            pszPath);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    LwRtlUnicodeStringInit(&filename.Name, pwszRemotePath);

    dwError = LwNtStatusToWin32Error(
                LwNtCreateFile(
                    &hFile,
                    NULL,
                    &ioStatus,
                    &filename,
                    NULL,
                    NULL,
                    READ_CONTROL | FILE_READ_ATTRIBUTES,
                    0,
                    0,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    FILE_DIRECTORY_FILE,
                    NULL,
                    0,
                    NULL,
                    NULL));
    BAIL_ON_LSA_ERROR(dwError);


    dwError = LwNtStatusToWin32Error(
                LwIoRdrGetPhysicalPath(
                    hFile,
                    &pwszPhysicalPath));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszPhysicalPath, &pszPhysicalPathWrongSlashes);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ConvertSlashes(
            pszPhysicalPathWrongSlashes,
            &pszPhysicalPathRightSlashes);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                &pszPhysicalPath,
                "/%s",
                pszPhysicalPathRightSlashes);
    BAIL_ON_LSA_ERROR(dwError);

    LwNtCloseFile(hFile);
    hFile = NULL;

    *ppszPhysicalPath = pszPhysicalPath;
    pszPhysicalPath = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszPhysicalPathRightSlashes);
    LW_SAFE_FREE_STRING(pszPhysicalPathWrongSlashes);
    LW_SAFE_FREE_MEMORY(pwszPhysicalPath);
    LW_SAFE_FREE_MEMORY(pwszRemotePath);

    if (hFile)
    {
        LwNtCloseFile(hFile);
        hFile = NULL;
    }
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszPhysicalPath);
    goto cleanup;
}

static
DWORD
SplitUncPath(
    PCSTR pszUncPath,
    PSTR *ppszServerName,
    PSTR *ppszShare,
    PSTR *ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszServerName = NULL;
    PSTR pszServerNameIter = NULL;
    PSTR pszShare = NULL;
    PSTR pszShareIter = NULL;
    PSTR pszPath = NULL;

    while (*pszUncPath == '/')
    {
        pszUncPath++;
    }

    if (!*pszUncPath)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(pszUncPath, &pszServerName);
    BAIL_ON_LSA_ERROR(dwError);

    pszServerNameIter = pszServerName;
    while (*pszServerNameIter && *pszServerNameIter != '/')
        pszServerNameIter++;

    if (*pszServerNameIter)
    {
        *pszServerNameIter = '\0';

        dwError = LwAllocateString(pszServerNameIter + 1, &pszShare);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszShareIter = pszShare;
    while (*pszShareIter && *pszShareIter != '/')
        pszShareIter++;

    if (*pszShareIter)
    {
        *pszShareIter = '\0';

        dwError = LwAllocateString(pszShareIter + 1, &pszPath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppszServerName)
    {
        *ppszServerName = pszServerName;
        pszServerName = NULL;
    }

    if (ppszShare)
    {
        *ppszShare = pszShare;
        pszShare = NULL;
    }

    if (ppszPath)
    {
        *ppszPath = pszPath;
        pszPath = NULL;
    }

error:
    LW_SAFE_FREE_STRING(pszServerName);
    LW_SAFE_FREE_STRING(pszShare);
    LW_SAFE_FREE_STRING(pszPath);
    return dwError;
}

static
DWORD
CreateLwIoCredsFromLsaCredentials(
    uid_t uid,
    PCSTR pszDomainName,
    LW_PIO_CREDS *ppCreds
    )
{
    DWORD dwError = 0;
    LSA_CRED_HANDLE hCred = NULL;
    PCSTR pszCredUserName = NULL;
    PCSTR pszCredPassword = NULL;
    PSTR pszDomainUserName = NULL;
    PCSTR pszUserName = NULL;
    LW_PIO_CREDS pCreds = NULL;

    hCred = LsaGetCredential(uid);
    if (!hCred)
    {
        dwError = LW_ERROR_INTERNAL; /* fixme */
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaGetCredentialInfo(hCred, &pszCredUserName, &pszCredPassword, NULL);

    dwError = LwAllocateString(pszCredUserName, &pszDomainUserName);
    BAIL_ON_LSA_ERROR(dwError);

    {
        int i;
        for (i = 0; pszDomainUserName[i]; i++)
        {
            if (pszDomainUserName[i] == '\\')
            {
               pszDomainUserName[i] = '\0';
               pszUserName = pszDomainUserName + i + 1;
               break;
            }
        }
    }

    dwError = LwIoCreatePlainCredsA(
                    pszUserName,
                    pszDomainName,
                    pszCredPassword,
                    &pCreds);
    BAIL_ON_LSA_ERROR(dwError);

    *ppCreds = pCreds;
cleanup:

    LW_SAFE_FREE_STRING(pszDomainUserName);
    LsaReleaseCredential(&hCred);
    return dwError;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
        pCreds = NULL;
    }
    goto cleanup;
}

static
DWORD
GetIpAddress(
    PCSTR pszServerName,
    PSTR *ppszIpAddress
    )
{
    DWORD dwError = 0;
    int addrinfoResult = 0;
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *rp = NULL;
    PSTR pszIpAddress = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    addrinfoResult = getaddrinfo(pszServerName, NULL, &hints, &result);

    for (rp = result; rp; rp = rp->ai_next)
    {
        if (rp->ai_family == AF_INET)
        {
            char szIp4Address[INET_ADDRSTRLEN];
            if (inet_ntop(
                    AF_INET,
                    &((struct sockaddr_in*)rp->ai_addr)->sin_addr,
                    szIp4Address, sizeof(szIp4Address)) != NULL)
            {
                dwError = LwAllocateString(szIp4Address, &pszIpAddress);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
        }
    }

    *ppszIpAddress = pszIpAddress;

cleanup:
    return dwError;
error:
    LW_SAFE_FREE_STRING(pszIpAddress);
    goto cleanup;
}

DWORD
AD_MountRemoteWindowsDirectory(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
)
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    mode_t  umask = 0;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;
    PSTR pszUnresolvedRemotePath = NULL;
    LW_PIO_CREDS pOldCreds = NULL;
    LW_PIO_CREDS pUserCreds = NULL;
    PSTR pszResolvedRemotePath = NULL;
    PCSTR pszRemotePath = NULL;
    PSTR pszMountCommand = NULL;
    PSTR pszServerName = NULL;
    PSTR pszShare = NULL;
    PSTR pszPath = NULL;
    PSTR pszIpAddress = NULL;
    PSTR pszRemotePathWithoutPrefix = NULL;
    PSTR pszPrefixPath = NULL;
    int error;

    if (LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszLocalWindowsHomeFolder) ||
        LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszWindowsHomeFolder))
    {
        /* Nothing to do, not an error */
        goto cleanup;
    }

#ifndef __LWI_LINUX__
    LSA_LOG_INFO("Mouting remote Windows shares not supported on this platform.");
    dwError = ERROR_NOT_SUPPORTED;
    BAIL_ON_LSA_ERROR(dwError);
#endif

    /* If we are not mounting the remote share as the home directory,
       test to see if it exists and create it if it doesn't.
    */
    if (strcasecmp(
           pObject->userInfo.pszLocalWindowsHomeFolder,
           pObject->userInfo.pszHomedir) != 0)
    {
        dwError = LsaCheckDirectoryExists(
                        pObject->userInfo.pszLocalWindowsHomeFolder,
                        &bExists);
        if (!bExists)
        {
            umask = AD_GetUmask(pState);

            dwError = LsaCreateDirectory(
                        pObject->userInfo.pszLocalWindowsHomeFolder,
                        perms);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaChangePermissions(
                        pObject->userInfo.pszLocalWindowsHomeFolder,
                        perms & (~umask));
            BAIL_ON_LSA_ERROR(dwError);

            bRemoveDir = TRUE;

            dwError = LsaChangeOwner(
                        pObject->userInfo.pszLocalWindowsHomeFolder,
                        pObject->userInfo.uid,
                        pObject->userInfo.gid);
            BAIL_ON_LSA_ERROR(dwError);

            bRemoveDir = FALSE;
        }
    }

    dwError = ConvertSlashes(
                pObject->userInfo.pszWindowsHomeFolder,
                &pszUnresolvedRemotePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = CreateLwIoCredsFromLsaCredentials(
                pObject->userInfo.uid,
                pState->pszDomainName,
                &pUserCreds);
    if (pUserCreds)
    {
        dwError = LwNtStatusToWin32Error(LwIoGetThreadCreds(&pOldCreds));
        dwError = LwNtStatusToWin32Error(LwIoSetThreadCreds(pUserCreds));
        BAIL_ON_LSA_ERROR(dwError);
    }

    ResolveRemotePath(pszUnresolvedRemotePath, &pszResolvedRemotePath);

    if (pUserCreds)
    {
        LwIoSetThreadCreds(pOldCreds);
        pOldCreds = NULL;

        LwIoDeleteCreds(pUserCreds);
        pUserCreds = NULL;
    }

    pszRemotePath = pszUnresolvedRemotePath;
    if (pszResolvedRemotePath)
    {
        pszRemotePath = pszResolvedRemotePath;
    }

    dwError = SplitUncPath(pszRemotePath, &pszServerName, &pszShare, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = GetIpAddress(pszServerName, &pszIpAddress);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pszIpAddress)
    {
        LSA_LOG_ERROR(
                "Failed to resolve IP address of %s; cannot mount cifs",
                pszServerName);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }


    if (pszPath)
    {
        dwError = LwAllocateStringPrintf(
                    &pszRemotePathWithoutPrefix,
                    "//%s/%s",
                    pszServerName,
                    pszShare);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
                    &pszPrefixPath,
                    "prefixpath=%s,",
                    pszPath);
        BAIL_ON_LSA_ERROR(dwError);

        pszRemotePath = pszRemotePathWithoutPrefix;
    }

    dwError = LwAllocateStringPrintf(
                &pszMountCommand,
                "%ssec=krb5,user=%s,uid=%u,gid=%u,cruid=%u,ip=%s",
                pszPrefixPath ? pszPrefixPath: "",
                pObject->userInfo.pszUPN,
                pObject->userInfo.uid,
                pObject->userInfo.gid,
                pObject->userInfo.uid,
                pszIpAddress);
    BAIL_ON_LSA_ERROR(dwError);

#ifdef __LWI_LINUX__
    error = mount(
                pszRemotePath,
                pObject->userInfo.pszLocalWindowsHomeFolder,
                "cifs",
                MS_NODEV  | MS_NOSUID,
                pszMountCommand);
#endif

    LSA_LOG_VERBOSE("mount(\"%s\", \"%s\", \"%s\", %s, \"%s\") = %d",
            pszRemotePath,
            pObject->userInfo.pszLocalWindowsHomeFolder,
            "cifs",
            "MS_NODEV | MS_NSUID",
            pszMountCommand,
            error);
    if (error < 0)
    {
        dwError = LwMapErrnoToLwError(errno);

        LSA_LOG_ERROR(
            "Failed mount of %s on %s with data %s, error %u",
            pszRemotePath,
            pObject->userInfo.pszLocalWindowsHomeFolder,
            pszMountCommand,
            dwError);

        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaUmModifyUserMountedDirectory(
                    pObject->userInfo.uid,
                    pObject->userInfo.pszLocalWindowsHomeFolder);
        if (dwError)
        {
            LSA_LOG_WARNING(
                "Failed adding mount %s to user monitor thread",
                pObject->userInfo.pszLocalWindowsHomeFolder);
            dwError = 0;
        }
    }

cleanup:
    LW_SAFE_FREE_STRING(pszPrefixPath);
    LW_SAFE_FREE_STRING(pszRemotePathWithoutPrefix);
    LW_SAFE_FREE_STRING(pszMountCommand);
    LW_SAFE_FREE_STRING(pszIpAddress);
    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszShare);
    LW_SAFE_FREE_STRING(pszServerName);
    LW_SAFE_FREE_STRING(pszResolvedRemotePath);
    LW_SAFE_FREE_STRING(pszUnresolvedRemotePath);

    return dwError;

error:

    if (bRemoveDir)
    {
       LsaRemoveDirectory(pObject->userInfo.pszHomedir);
    }

    LSA_LOG_ERROR("Failed to mount directory for user (%s), actual error %u", LSA_SAFE_LOG_STRING(pObject->userInfo.pszUnixName), dwError);

    goto cleanup;
}

DWORD
AD_UnmountRemoteWindowsDirectory(
    PCSTR pszMountPoint
    )
{
    DWORD dwError = 0;
#ifndef __LWI_LINUX
    goto error;
#endif

#ifdef __LWI_LINUX__
    if (umount(pszMountPoint) < 0)
    {
       if (!errno == EINVAL && !errno == ENOENT)
       {
           dwError = LwMapErrnoToLwError(errno);
           BAIL_ON_LSA_ERROR(dwError);
       }
    }
#endif

error:
    return dwError;
}

