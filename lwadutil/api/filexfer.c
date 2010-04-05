#include "includes.h"

DWORD
GetPreferredDCAddress(
    PSTR  pszDomain,
    PSTR* ppszDCAddress
    )
{
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PLWNET_DC_INFO pDCInfo = NULL;
    PSTR pszDCAddress = NULL;

    dwError = LWNetGetDCName(
                NULL,
                pszDomain,
                NULL,  //pszSiteName
                0,     //dwFlags
                &pDCInfo);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateString(
                  pDCInfo->pszDomainControllerName,
                  &pszDCAddress);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *ppszDCAddress = pszDCAddress;

cleanup:

    if (pDCInfo)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    return dwError;

error:

    *ppszDCAddress = NULL;

    LWFreeString(pszDCAddress);

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

    dwError = LWAllocateString(szDomainName, &pszDomainName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateString(pSysVol + sizeof("\\sysvol\\") - 1, &pszSourcePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    pTemp = pSysVol + sizeof("\\sysvol\\");
    pPolicy = strchr(pTemp, '{');

    /* TODO: make sure pPolicy points to end of \\sysvol\\ */
    if ( pPolicy && strlen(pPolicy) >= 38 ) {
        strcpy(szPolicyId, pPolicy);
        szPolicyId[38] = 0;
    } else {
        dwError = LWUTIL_ERROR_INVALID_ATTRIBUTE_TYPE;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LWAllocateString(szPolicyId, &pszPolicyIdentifier);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if(ppszDomainName) {
        *ppszDomainName = pszDomainName;
	}

    if(ppszSourcePath) {
        *ppszSourcePath = pszSourcePath;
	}

    if(ppszPolicyIdentifier) {
        *ppszPolicyIdentifier = pszPolicyIdentifier;
	}

    return dwError;

cleanup:

    if (pszDomainName) {
        LWFreeString(pszDomainName);
    }

    if (pszSourcePath) {
        LWFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier) {
        LWFreeString(pszPolicyIdentifier);
    }

    if(ppszDomainName) {
        *ppszDomainName = NULL;
	}

    if(ppszSourcePath) {
        *ppszSourcePath = NULL;
	}

    if(ppszPolicyIdentifier) {
        *ppszPolicyIdentifier = NULL;
	}

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
    DWORD   dwError = LWUTIL_ERROR_SUCCESS;
    PSTR    pszFQSrcPath = NULL;
    PSTR    pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourcePath) ||
        IsNullOrEmptyString(pszDestPath))
    {
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszFQSrcPath,
                    "/%s/sysvol/%s",
                    pszDCHostname,
                    *pszSourcePath == '/' ? pszSourcePath+1 : pszSourcePath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    LWUTIL_LOG_INFO("Calling ADUSMBCopyFile(Src:%s, Dest:%s)", pszFQSrcPath, pszDestPath);

    dwError = ADUCopyFileFromRemote(pszFQSrcPath, pszDestPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pszFQSrcPath)
        LWFreeString(pszFQSrcPath);

    if (pszDCHostname)
        LWFreeString(pszDCHostname);

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
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR  pszFQSrcPath = NULL;
    PSTR  pszFQDstPath = NULL;
    PSTR  pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolder) ||
        IsNullOrEmptyString(pszFileName) ||
        IsNullOrEmptyString(pszDestFolder))
    {
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszFQSrcPath,
                    "%s/%s",
                    pszSourceFolder,
                    pszFileName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszFQDstPath,
                    "/%s/sysvol/%s/%s",
                    pszDCHostname,
                    *pszDestFolder == '/' ? pszDestFolder+1 : pszDestFolder,
                    pszFileName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    LWUTIL_LOG_INFO("Calling ADUSMBPutFile(Domain: %s, Src Path: %s, Dest Path: %s)", pszDomainName, pszFQSrcPath, pszFQDstPath);

    dwError = ADUCopyFileToRemote(pszFQSrcPath, pszFQDstPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pszFQSrcPath)
        LWFreeString(pszFQSrcPath);

    if (pszFQDstPath)
        LWFreeString(pszFQDstPath);

    if (pszDCHostname)
        LWFreeString(pszDCHostname);

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
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR  pszFQSrcPath = NULL;
    PSTR  pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolder) ||
        IsNullOrEmptyString(pszDestFolder))
    {
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWCreateDirectory(
                    pszDestFolder,
                    S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszFQSrcPath,
                    "//%s/sysvol/%s",
                    pszDCHostname,
                    *pszSourceFolder == '/' ? pszSourceFolder+1 : pszSourceFolder);
    BAIL_ON_LWUTIL_ERROR(dwError);

    LWUTIL_LOG_INFO("Calling ADUSMBCopyFolder(Src:%s, Dest:%s) which will run command: %s)", pszFQSrcPath, pszDestFolder);

    dwError = ADUCopyDirFromRemote(
                    pszFQSrcPath,
                    pszDestFolder);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if ( pszFQSrcPath )
        LWFreeString( pszFQSrcPath );

    if (pszDCHostname)
        LWFreeString(pszDCHostname);

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
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    PSTR  pszFQSrcPath = NULL;
    PSTR  pszFQDstPath = NULL;
    PSTR  pszDCHostname = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszSourceFolderParent) ||
        IsNullOrEmptyString(pszFolderName) ||
        IsNullOrEmptyString(pszDestFolder))
    {
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = GetPreferredDCAddress(
                    pszDomainName,
                    &pszDCHostname);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszFQSrcPath,
                    "%s/%s",
                    pszSourceFolderParent,
                    pszFolderName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszFQDstPath,
                    "//%s/sysvol/%s",
                    pszDCHostname,
                    *pszDestFolder == '/' ? pszDestFolder+1 : pszDestFolder);
    BAIL_ON_LWUTIL_ERROR(dwError);

    LWUTIL_LOG_INFO("Calling ADUSMBPutFolder(Src : %s, Dest:%s)", pszFQSrcPath, pszFQDstPath);

    dwError = ADUCopyDirToRemote(pszFQSrcPath, pszFQDstPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pszFQSrcPath)
        LWFreeMemory(pszFQSrcPath);

    if (pszFQDstPath)
        LWFreeMemory(pszFQDstPath);

    if (pszDCHostname)
        LWFreeString(pszDCHostname);

    return dwError;

error:

    goto cleanup;
}

