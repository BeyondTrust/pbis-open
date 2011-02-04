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
GetPreferredDCAddress(
    PSTR  pszDomain,
    PSTR* ppszDCAddress
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PLWNET_DC_INFO pDCInfo = NULL;
    PSTR pszDCAddress = NULL;

    dwError = LWNetGetDCName(
                NULL,
                pszDomain,
                NULL,  //pszSiteName
                0,     //dwFlags
                &pDCInfo);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateString(
                  pDCInfo->pszDomainControllerName,
                  &pszDCAddress);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszDCAddress = pszDCAddress;

cleanup:

    if (pDCInfo)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    return dwError;

error:

    *ppszDCAddress = NULL;

    LW_SAFE_FREE_STRING(pszDCAddress);

    goto cleanup;
}

DWORD
ADUCrackFileSysPath(
    PCSTR  pszFileSysPath,
    PSTR * ppszDomainName,
    PSTR * ppszSourcePath,
    PSTR * ppszPolicyIdentifier
    )
{
    DWORD dwError = 0;
    PSTR pTemp = NULL;
    char szDomainName[256];
    char szPolicyId[256];
    PSTR pSysVol = NULL;
    PSTR pPolicy = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;

    pTemp = (PSTR)pszFileSysPath + 2;
    pSysVol = strchr(pTemp, '\\');

    /* TODO: make sure pSysVol points to \\sysvol\\ */
    memset(szDomainName, 0, sizeof(szDomainName));
    strncpy(szDomainName, pTemp, pSysVol - pTemp);
    szDomainName[pSysVol - pTemp] = 0;

    dwError = LwAllocateString(szDomainName, &pszDomainName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateString(pSysVol + sizeof("\\sysvol\\") - 1, &pszSourcePath);
    BAIL_ON_MAC_ERROR(dwError);

    pTemp = pSysVol + sizeof("\\sysvol\\");
    pPolicy = strchr(pTemp, '{');

    /* TODO: make sure pPolicy points to end of \\sysvol\\{ */
    if ( pPolicy && strlen(pPolicy) >= 38 ) {
        strcpy(szPolicyId, pPolicy);
        szPolicyId[38] = 0;
    } else {
        dwError = MAC_AD_ERROR_INVALID_ATTRIBUTE_TYPE;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LwAllocateString(szPolicyId, &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszDomainName = pszDomainName;
    *ppszSourcePath = pszSourcePath;
    *ppszPolicyIdentifier = pszPolicyIdentifier;

    return dwError;

cleanup:

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszSourcePath);
    LW_SAFE_FREE_STRING(pszPolicyIdentifier);

    *ppszDomainName = NULL;
    *ppszSourcePath = NULL;
    *ppszPolicyIdentifier = NULL;

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUSMBGetFile(
    PSTR  pszDomainName,
    PSTR  pszSourcePath,
    PSTR  pszDestPath
    )
{
    DWORD   dwError = MAC_AD_ERROR_SUCCESS;
    PSTR    pszFQSrcPath = NULL;
    PSTR    pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourcePath) ||
        IsNullOrEmptyString(pszDestPath))
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFQSrcPath,
                    "//%s/sysvol/%s",
                    pszDCHostname,
                    *pszSourcePath == '/' ? pszSourcePath+1 : pszSourcePath);
    BAIL_ON_MAC_ERROR(dwError);

    LOG("Calling ADUCopyFileFromRemote(Src:%s, Dest:%s)", pszFQSrcPath, pszDestPath);

    dwError = ADUCopyFileFromRemote(pszFQSrcPath, pszDestPath);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszFQSrcPath);
    LW_SAFE_FREE_STRING(pszDCHostname);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUSMBPutFile(
    PSTR  pszDomainName,
    PSTR  pszSourceFolder,
    PSTR  pszFileName,
    PSTR  pszDestFolder
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR  pszFQSrcPath = NULL;
    PSTR  pszFQDstPath = NULL;
    PSTR  pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolder) ||
        IsNullOrEmptyString(pszFileName) ||
        IsNullOrEmptyString(pszDestFolder))
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFQSrcPath,
                    "/%s/%s",
                    *pszSourceFolder == '/' ? pszSourceFolder+1 : pszSourceFolder,
                    pszFileName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFQDstPath,
                    "//%s/sysvol/%s/%s",
                    pszDCHostname,
                    *pszDestFolder == '/' ? pszDestFolder+1 : pszDestFolder,
                    pszFileName);
    BAIL_ON_MAC_ERROR(dwError);

    LOG("Calling ADUSMBPutFile(Domain: %s, Src Path: %s, Dest Path: %s)", pszDomainName, pszFQSrcPath, pszFQDstPath);

    dwError = ADUCopyFileToRemote(pszFQSrcPath, pszFQDstPath);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszFQSrcPath);
    LW_SAFE_FREE_STRING(pszFQDstPath);
    LW_SAFE_FREE_STRING(pszDCHostname);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUSMBGetFolder(
    PSTR  pszDomainName,
    PSTR  pszSourceFolder,
    PSTR  pszDestFolder
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR  pszFQSrcPath = NULL;
    PSTR  pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolder) ||
        IsNullOrEmptyString(pszDestFolder))
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwCreateDirectory(
                    pszDestFolder,
                    S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFQSrcPath,
                    "//%s/sysvol/%s",
                    pszDCHostname,
                    *pszSourceFolder == '/' ? pszSourceFolder+1 : pszSourceFolder);
    BAIL_ON_MAC_ERROR(dwError);

    LOG("Calling ADUSMBGetFolder(Src:%s, Dest:%s)", pszFQSrcPath, pszDestFolder);

    dwError = ADUCopyDirFromRemote(
                    pszFQSrcPath,
                    pszDestFolder);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING( pszFQSrcPath );
    LW_SAFE_FREE_STRING(pszDCHostname);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUSMBPutFolder(
    PSTR    pszDomainName,
    PSTR    pszSourceFolderParent,
    PSTR    pszFolderName,
    PSTR    pszDestFolder,
    BOOLEAN fReplace
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR  pszFQSrcPath = NULL;
    PSTR  pszFQDstPath = NULL;
    PSTR  pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolderParent) ||
        IsNullOrEmptyString(pszFolderName) ||
        IsNullOrEmptyString(pszDestFolder))
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFQSrcPath,
                    "/%s/%s",
                    pszSourceFolderParent,
                    pszFolderName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFQDstPath,
                    "//%s/sysvol/%s",
                    pszDCHostname,
                    *pszDestFolder == '/' ? pszDestFolder+1 : pszDestFolder);
    BAIL_ON_MAC_ERROR(dwError);

    LOG("Calling ADUSMBPutFolder(Src : %s, Dest:%s)", pszFQSrcPath, pszFQDstPath);

    dwError = ADUCopyDirToRemote(pszFQSrcPath, pszFQDstPath);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszFQSrcPath);
    LW_SAFE_FREE_STRING(pszFQDstPath);
    LW_SAFE_FREE_STRING(pszDCHostname);

    return dwError;

error:

    goto cleanup;
}
