/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "../includes.h"


DWORD
GetCachedPolicyFiles(
    DWORD    dwPolicyType,
    PCSTR    pszgGpSysVolPath,
    PCSTR    pszgCseIdentifier,
    PCSTR    pszDestFolderRootPath,
    PSTR *   ppszDestFolder,
    PBOOLEAN pbPolicyExists
    )
{
    DWORD dwError = 0;
    PSTR pszDestFolderPath = NULL;
    PSTR pszDestGPFolderPath = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    BOOLEAN bPolicyExists = FALSE;
    
    if (!ppszDestFolder || !pbPolicyExists)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }
  
    dwError =  ADUCrackFileSysPath(
                    pszgGpSysVolPath,
                    &pszDomainName,
                    &pszSourcePath,
                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszDestGPFolderPath,
                    "%s/%s",
                    IsNullOrEmptyString(pszDestFolderRootPath) ? LWDS_ADMIN_CACHE_DIR : pszDestFolderRootPath,
                    pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    if ( pszgCseIdentifier != NULL ) {
        dwError = LwAllocateStringPrintf(
                &pszDestFolderPath,
                "%s/%s",
                pszDestGPFolderPath,
                pszgCseIdentifier);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LwCheckFileTypeExists(
                    pszDestFolderPath,
                    LWFILE_DIRECTORY,
                    &bPolicyExists);
    BAIL_ON_MAC_ERROR(dwError); 
    
    *ppszDestFolder = pszDestFolderPath;
    pszDestFolderPath = NULL;
    *pbPolicyExists = bPolicyExists;

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszDestFolderPath);
    LW_SAFE_FREE_STRING(pszDestGPFolderPath);

    return dwError;
    
error:

    if (ppszDestFolder)
    {
        *ppszDestFolder = NULL;
    }
    
    if (pbPolicyExists)
    {
        *pbPolicyExists = FALSE;
    }

    goto cleanup;
}

DWORD
IsCacheDataCurrentForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    PDWORD   pdwVersion,
    PDWORD   pdwFileVersion,
    PBOOLEAN pbCurrent
    )
{
    DWORD dwError = 0;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    PSTR pszCachePath = NULL;
    PSTR pszGPOVersionFile = NULL;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bFileExists = FALSE;
    DWORD dwVersion = 0;
    DWORD dwFileGPTVersion = 0;
    PSTR  pszGPTFilePath = NULL;
    BOOLEAN  bIsCurrent = FALSE;
    
    if (!pdwVersion || !pdwFileVersion || !pbCurrent)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = ADUGetGPTFileAndVersionNumber(
                    pGPO->pszgPCFileSysPath,
                    &pszGPTFilePath,
                    &dwFileGPTVersion);
    if (dwError)
    {
        LOG_ERROR("GPO GPT.INI file not found on DC, maybe this GPO no longer exists");
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
        BAIL_ON_MAC_ERROR(dwError);
    }

    LOG("The GPT.INI in sysvol for GPO is at version %d", dwFileGPTVersion);

    dwError =  ADUCrackFileSysPath(
                    pGPO->pszgPCFileSysPath,
                    &pszDomainName,
                    &pszSourcePath,
                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszCachePath,
                    "%s/%s",
                    LWDS_ADMIN_CACHE_DIR,
                    pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszGPOVersionFile,
                    "%s/GPT.INI",
                    pszCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwCheckFileTypeExists(
                    pszCachePath, 
                    LWFILE_DIRECTORY,
                    &bDirExists);
    BAIL_ON_MAC_ERROR(dwError);
    
    if (bDirExists)
    {
        dwError = LwCheckFileTypeExists(
                        pszGPOVersionFile,
                        LWFILE_REGULAR,
                        &bFileExists);
        BAIL_ON_MAC_ERROR(dwError);
    
        if (bFileExists)
        {
            dwError = ADUParseAndGetGPTVersion(
                            pszGPOVersionFile,
                            &dwVersion);
            BAIL_ON_MAC_ERROR(dwError);

            if (dwVersion == dwFileGPTVersion)
            {
                bIsCurrent = TRUE;
            }

            LwRemoveFile(pszGPOVersionFile);
        }
    }
    else
    {
        mode_t perms = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
        
        dwError = LwCreateDirectory(
                        pszCachePath,
                        perms);
        BAIL_ON_MAC_ERROR(dwError);
    } 

    dwError = LwMoveFile( pszGPTFilePath, pszGPOVersionFile);
    BAIL_ON_MAC_ERROR(dwError);

    *pdwVersion = dwVersion;
    *pdwFileVersion = dwFileGPTVersion;
    *pbCurrent = bIsCurrent;

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);
    LW_SAFE_FREE_STRING(pszGPTFilePath);
    LW_SAFE_FREE_STRING(pszCachePath);
    LW_SAFE_FREE_STRING(pszGPOVersionFile);

    return dwError;
    
error:

    if (pdwVersion)
    {
        *pdwVersion = 0;
    }

    if (pdwFileVersion)
    {
        *pdwFileVersion = 0;
    }
    
    if (pbCurrent)
    {
        *pbCurrent = FALSE;
    }
    
    if (dwError == MAC_AD_ERROR_NO_SUCH_POLICY)
    {
        dwError = 0;
    }

    goto cleanup;
}

