#include "includes.h"

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
    
    BAIL_ON_INVALID_POINTER(ppszDestFolder);
    BAIL_ON_INVALID_POINTER(pbPolicyExists);
  
    dwError =  ADUCrackFileSysPath(
                    pszgGpSysVolPath,
                    &pszDomainName,
                    &pszSourcePath,
                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszDestGPFolderPath,
                    "%s/%s",
                    IsNullOrEmptyString(pszDestFolderRootPath) ? LWDS_ADMIN_CACHE_DIR : pszDestFolderRootPath,
                    pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    if ( pszgCseIdentifier != NULL ) {
        dwError = LWAllocateStringPrintf(
                &pszDestFolderPath,
                "%s/%s",
                pszDestGPFolderPath,
                pszgCseIdentifier);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LWCheckDirectoryExists(
                    pszDestFolderPath,
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
    
    BAIL_ON_INVALID_POINTER(pdwVersion);
    BAIL_ON_INVALID_POINTER(pdwFileVersion);
    BAIL_ON_INVALID_POINTER(pbCurrent);

    dwError = ADUGetGPTFileAndVersionNumber(
                    pGPO->pszgPCFileSysPath,
                    &pszGPTFilePath,
                    &dwFileGPTVersion);
    if (dwError)
    {
        MAC_AD_LOG_WARNING("GPO GPT.INI file not found on DC, maybe this GPO no longer exists");
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
        BAIL_ON_MAC_ERROR(dwError);
    }

    MAC_AD_LOG_INFO("The GPT.INI in sysvol for GPO is at version %d", dwFileGPTVersion);

    dwError =  ADUCrackFileSysPath(
                    pGPO->pszgPCFileSysPath,
                    &pszDomainName,
                    &pszSourcePath,
                    &pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszCachePath,
                    "%s/%s",
                    LWDS_ADMIN_CACHE_DIR,
                    pszPolicyIdentifier);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszGPOVersionFile,
                    "%s/GPT.INI",
                    pszCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCheckDirectoryExists(
                    pszCachePath, 
                    &bDirExists);
    BAIL_ON_MAC_ERROR(dwError);
    
    if (bDirExists)
    {
        dwError = LWCheckFileExists(
                        pszGPOVersionFile,
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

            LWRemoveFile(pszGPOVersionFile);
        }
    }
    else
    {
        mode_t perms = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
        
        dwError = LWCreateDirectory(
                        pszCachePath,
                        perms);
        BAIL_ON_MAC_ERROR(dwError);
    } 

    dwError = LWMoveFileAcrossDevices(
                    pszGPTFilePath,
                    pszGPOVersionFile);
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

