/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "../includes.h"


#define STATIC_PATH_BUFFER_SIZE 256


static
DWORD
RemoveDirectory(
    PCSTR pszPath
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);

            if (rmdir(szBuf) < 0) {
                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);
            }
        }
        else
        {
            dwError = LwRemoveFile(szBuf);
            BAIL_ON_MAC_ERROR(dwError);
        }
    }
    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    pDir = NULL;

    if (rmdir(pszPath) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    if (pDir)
        closedir(pDir);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUGetPolicyFiles(
    DWORD     dwPolicyType,
    PSTR      pszgGpSysVolPath,
    PSTR      pszgCseIdentifier,
    PSTR      pszDestFolderRootPath,
    PSTR *    ppszDestFolder,
    BOOLEAN * pbPolicyExists
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    char szSourceFolderPath[STATIC_PATH_BUFFER_SIZE];
    char szDestFolderPath[STATIC_PATH_BUFFER_SIZE];
    char szDestGPFolderPath[STATIC_PATH_BUFFER_SIZE];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bFolderExists = FALSE;

    memset(szSourceFolderPath, 0, sizeof(szSourceFolderPath));
    memset(szDestFolderPath, 0, sizeof(szDestFolderPath));
    memset(szDestGPFolderPath, 0, sizeof(szDestGPFolderPath));

    dwError =  ADUCrackFileSysPath(pszgGpSysVolPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    sprintf(szDestGPFolderPath,
            "%s/%s",
            IsNullOrEmptyString(pszDestFolderRootPath) ? LWDS_ADMIN_CACHE_DIR : pszDestFolderRootPath,
            pszPolicyIdentifier);

    sprintf(szSourceFolderPath, "%s\\%s\\Centeris\\Identity\\%s",
            pszSourcePath,
            (dwPolicyType == MACHINE_GROUP_POLICY ? "Machine" : "User"),
            pszgCseIdentifier );

    sprintf(szDestFolderPath, "%s/%s", szDestGPFolderPath, pszgCseIdentifier);

    dwError = LwCheckFileTypeExists(szDestFolderPath, LWFILE_DIRECTORY, &bFolderExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (bFolderExists) {
        dwError = RemoveDirectory(szDestFolderPath);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = ADUSMBGetFolder(pszDomainName,
                              szSourceFolderPath,
                              szDestFolderPath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwCheckFileTypeExists(szDestFolderPath, LWFILE_DIRECTORY, pbPolicyExists);
    BAIL_ON_MAC_ERROR(dwError);

    if ( *pbPolicyExists )
    {
        dwError = LwAllocateString( szDestFolderPath, ppszDestFolder );
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return dwError;

error:

    if (pbPolicyExists)
        *pbPolicyExists = FALSE;

    goto cleanup;
}

DWORD
ADUPutPolicyFiles(
    PSTR    pszSourceFolderRootPath,
    BOOLEAN fReplace,
    DWORD   dwPolicyType,
    PSTR    pszgGpSysVolPath,
    PSTR    pszgCseIdentifier
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    char szSourceFolderPath[STATIC_PATH_BUFFER_SIZE];
    char szDestFolderPath[STATIC_PATH_BUFFER_SIZE];
    PSTR pszDomainName = NULL;
    PSTR pszDestPath = NULL;
    PSTR pszPolicyIdentifier = NULL;

    memset(szSourceFolderPath, 0, sizeof(szSourceFolderPath));
    memset(szDestFolderPath, 0, sizeof(szDestFolderPath));

    dwError =  ADUCrackFileSysPath(pszgGpSysVolPath,
                                    &pszDomainName,
                                    &pszDestPath,
                                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    sprintf(szSourceFolderPath,
            "%s/%s",
            IsNullOrEmptyString(pszSourceFolderRootPath) ? LWDS_ADMIN_CACHE_DIR : pszSourceFolderRootPath,
            pszPolicyIdentifier);

    sprintf(szDestFolderPath,
            "%s\\%s\\Centeris\\Identity\\%s\\",
            pszDestPath, (dwPolicyType == MACHINE_GROUP_POLICY ? "Machine" : "User"),
            pszgCseIdentifier );

    dwError = ADUSMBPutFolder(pszDomainName,
                              szSourceFolderPath,
                              pszgCseIdentifier,
                              szDestFolderPath,
                              fReplace);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszDestPath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    return dwError;

error:

    goto cleanup;
}
