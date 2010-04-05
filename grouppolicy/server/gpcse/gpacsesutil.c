#include "includes.h"

CENTERROR
GPACrackFileSysPath(
    PSTR pszFileSysPath,
    PSTR * ppszDomainName,
    PSTR * ppszSourcePath,
    PSTR * ppszPolicyIdentifier
    )
{
    CENTERROR ceError = 0;
    PSTR pTemp = NULL;
    char szDomainName[256];
    char szPolicyId[256];
    PSTR pSysVol = NULL;
    PSTR pPolicy = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;

    memset( szDomainName, 
            0, 
            256);
    memset( szPolicyId, 
            0, 
            256);

    pTemp = pszFileSysPath + 2;
    pSysVol = strchr(pTemp, '\\');

    /* TODO: make sure pSysVol points to \\sysvol\\ */
    memset( szDomainName, 
            0, 
            sizeof(szDomainName));
    strncpy( szDomainName, 
             pTemp, 
             pSysVol - pTemp);

    ceError = LwAllocateString( szDomainName, 
                                &pszDomainName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString( pSysVol + sizeof("\\sysvol\\") - 1, 
                                &pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pTemp = pSysVol + sizeof("\\sysvol\\");
    pPolicy = strchr( pTemp, 
                      '{');

    /* TODO: make sure pPolicy points to end of \\sysvol\\ */
    if ( pPolicy && strlen(pPolicy) >= 38 ) {
        strcpy( szPolicyId, 
                &pPolicy[1]);
        szPolicyId[36] = 0;
    } else {
        ceError = CENTERROR_BAD_GUID;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString( szPolicyId, 
                                &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszDomainName = pszDomainName;
    *ppszSourcePath = pszSourcePath;
    *ppszPolicyIdentifier = pszPolicyIdentifier;

    return(ceError);

error:

    if (pszDomainName) {
        LwFreeString(pszDomainName);
    }

    if (pszSourcePath) {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier) {
        LwFreeString(pszPolicyIdentifier);
    }

    *ppszDomainName = NULL;
    *ppszSourcePath = NULL;
    *ppszPolicyIdentifier = NULL;

    return(ceError);
}

CENTERROR
GPARemovePolicy(
    PGROUP_POLICY_OBJECT pGPO,
    PSTR pszCSEType
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szNewFilePath[STATIC_PATH_BUFFER_SIZE];
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;
    PSTR pszgPCFileSysPath = NULL;
    PSTR pszDisplayName = NULL;

    if ( !pGPO ) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszgPCFileSysPath = pGPO->pszgPCFileSysPath;
    pszDisplayName = pGPO->pszDisplayName;

    ceError =  GPACrackFileSysPath( pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf( szNewFilePath,
             "%s%s_likewise_%s",
             CENTERIS_GP_DIRECTORY_,
             pszPolicyIdentifier,
             pszCSEType);

    GPA_LOG_VERBOSE( "Need to remove %s policy file: %s [%s]",
                     pszCSEType,
                     pszDisplayName,
                     szNewFilePath);

    LwRemoveFile(szNewFilePath);

error:

    if (pszDomainName) {
        LwFreeString(pszDomainName);
    }

    if (pszSourcePath) {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier) {
        LwFreeString(pszPolicyIdentifier);
    }

    return(ceError);
}

CENTERROR
GPADeleteFormerPolicies(
    PSTR pszCSEType
    )
{
   CENTERROR ceError = CENTERROR_SUCCESS;

   char szFilePath[STATIC_PATH_BUFFER_SIZE];

   sprintf( szFilePath,
            "%s*_centeris_%s",
            CENTERIS_GP_DIRECTORY_,
            pszCSEType);

   ceError = GPARemoveFiles( szFilePath, 
                            FALSE, 
                            FALSE );
   BAIL_ON_CENTERIS_ERROR(ceError);

error:

   return ceError;

}

CENTERROR
IsSuSeLinux(
    PBOOLEAN pbValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;

    ceError = GPACheckFileExists( SUSE_RELEASE_FILE, 
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pbValue = bExists;

error:

    return ceError;
}

BOOLEAN
GPAIsAbsolutePath(
    PCSTR pszPath
    )
{
    PCSTR pszTmp = pszPath;

    if (IsNullOrEmptyString(pszTmp))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int)*pszTmp))
        pszTmp++;

    return (*pszTmp == '/' || *pszTmp == '\0');
}

BOOLEAN
GPAIsComment(
    PCSTR pszPath
    )
{
    PCSTR pszTmp = pszPath;

    if (IsNullOrEmptyString(pszTmp))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int)*pszTmp))
        pszTmp++;

    return (*pszTmp == '#' || *pszTmp == '\0');
}

