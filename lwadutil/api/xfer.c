#include "includes.h"

#define STATIC_PATH_BUFFER_SIZE 256


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
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
    char szSourceFolderPath[STATIC_PATH_BUFFER_SIZE];
    char szDestFolderPath[STATIC_PATH_BUFFER_SIZE];
    char szDestGPFolderPath[STATIC_PATH_BUFFER_SIZE];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;

    memset(szSourceFolderPath, 0, sizeof(szSourceFolderPath));
    memset(szDestFolderPath, 0, sizeof(szDestFolderPath));
    memset(szDestGPFolderPath, 0, sizeof(szDestGPFolderPath));

    dwError =  ADUCrackFileSysPath(pszgGpSysVolPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_LWUTIL_ERROR(dwError);

    sprintf(szDestGPFolderPath,
            "%s/%s",
            IsNullOrEmptyString(pszDestFolderRootPath) ? LWDS_ADMIN_CACHE_DIR : pszDestFolderRootPath,
            pszPolicyIdentifier);

    sprintf(szSourceFolderPath, "%s\\%s\\Centeris\\Identity\\%s",
            pszSourcePath,
            (dwPolicyType == MACHINE_GROUP_POLICY ? "Machine" : "User"),
            pszgCseIdentifier );

    sprintf(szDestFolderPath, "%s/%s", szDestGPFolderPath, pszgCseIdentifier);

    dwError = ADUSMBGetFolder(pszDomainName,
                              szSourceFolderPath,
                              szDestFolderPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWCheckDirectoryExists(szDestFolderPath, pbPolicyExists);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if ( *pbPolicyExists )
    {
        dwError = LWAllocateString( szDestFolderPath, ppszDestFolder );
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

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
    DWORD dwError = LWUTIL_ERROR_SUCCESS;
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
    BAIL_ON_LWUTIL_ERROR(dwError);

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
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    if (pszDomainName) {
        LWFreeString(pszDomainName);
    }

    if (pszDestPath) {
        LWFreeString(pszDestPath);
    }

    if (pszPolicyIdentifier) {
        LWFreeString(pszPolicyIdentifier);
    }

    return dwError;

error:

    goto cleanup;
}
