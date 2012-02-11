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
 *        homedirectory.c
 *
 * Abstract:
 *
 *        Create and provision local home directories and mount remote user
 *        specific file systems.
 *
 */
#include "adprovider.h"
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

DWORD
AD_ProvisionHomeDir(
    PLSA_AD_PROVIDER_STATE pState,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    PSTR pszSkelPaths = NULL;
    PSTR pszSkelPath = NULL;
    PSTR pszIter = NULL;
    size_t stLen = 0;

    dwError = AD_GetSkelDirs(pState, &pszSkelPaths);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszSkelPaths))
    {
        goto cleanup;
    }

    pszIter = pszSkelPaths;
    while ((stLen = strcspn(pszIter, ",")) != 0)
    {
        dwError = LwStrndup(
                      pszIter,
                      stLen,
                      &pszSkelPath);
        BAIL_ON_LSA_ERROR(dwError);

        LwStripWhitespace(pszSkelPath, TRUE, TRUE);

        if (LW_IS_NULL_OR_EMPTY_STR(pszSkelPath))
        {
            LW_SAFE_FREE_STRING(pszSkelPath);
            continue;
        }

        dwError = LsaCheckDirectoryExists(
                        pszSkelPath,
                        &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (bExists)
        {
            dwError = LsaCopyDirectory(
                        pszSkelPath,
                        ownerUid,
                        ownerGid,
                        pszHomedirPath);
            BAIL_ON_LSA_ERROR(dwError);
        }

        LW_SAFE_FREE_STRING(pszSkelPath);

        pszIter += stLen;
        stLen = strspn(pszIter, ",");
        pszIter += stLen;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszSkelPath);
    LW_SAFE_FREE_STRING(pszSkelPaths);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CreateHomeDirectory(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT pObject
)
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    BOOLEAN bRemoveDir = FALSE;
    mode_t  umask = 0;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    PSTR pszMountCommand = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszHomedir)) {
        dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_ShouldCreateHomeDir(pState))
    {
        // If the home directory's parent directory is NFS mounted from a
        // windows server with "allow root" disabled, then root is blocked from
        // even read only access. In that case, this function will fail with
        // LW_ERROR_ACCESS_DENIED.
        dwError = LsaCheckDirectoryExists(
                        pObject->userInfo.pszHomedir,
                        &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bExists)
        {
            umask = AD_GetUmask(pState);

            dwError = LsaCreateDirectory(
                        pObject->userInfo.pszHomedir,
                        perms);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaChangePermissions(
                        pObject->userInfo.pszHomedir,
                        perms & (~umask));
            BAIL_ON_LSA_ERROR(dwError);

            bRemoveDir = TRUE;

            dwError = LsaChangeOwner(
                        pObject->userInfo.pszHomedir,
                        pObject->userInfo.uid,
                        pObject->userInfo.gid);
            BAIL_ON_LSA_ERROR(dwError);

            bRemoveDir = FALSE;

            dwError = AD_ProvisionHomeDir(
                        pState,
                        pObject->userInfo.uid,
                        pObject->userInfo.gid,
                        pObject->userInfo.pszHomedir);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LW_SAFE_FREE_STRING(pszMountCommand);

    return dwError;

error:

    if (bRemoveDir)
    {
       LsaRemoveDirectory(pObject->userInfo.pszHomedir);
    }

    if (dwError != LW_ERROR_FAILED_CREATE_HOMEDIR)
    {
        LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %u", LSA_SAFE_LOG_STRING(pObject->userInfo.pszUnixName), dwError);
    }

    dwError = LW_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}


