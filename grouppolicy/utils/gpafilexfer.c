#include "includes.h"


CENTERROR
GPOGetSysDirBase(
	PSTR pszFileSysPath,	
	PSTR *ppszSysDir,
	PSTR *ppszBase )
{
	CENTERROR ceError = 0;
	PSTR pLastSep = NULL;
	PSTR pszFileSysPathCopy = NULL;
	
	ceError = LwAllocateString( pszFileSysPath, &pszFileSysPathCopy );
	BAIL_ON_CENTERIS_ERROR(ceError);
	
	pLastSep = strrchr(pszFileSysPathCopy, '\\' );
	if ( pLastSep == NULL )
		pLastSep = strrchr(pszFileSysPathCopy, '/' );
	
	if ( pLastSep )
	{
		*pLastSep = '\0';
		pLastSep++;
		
		ceError = LwAllocateString( pszFileSysPathCopy, ppszSysDir );
		BAIL_ON_CENTERIS_ERROR(ceError);
		
		ceError = LwAllocateString( pLastSep, ppszBase );
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	else
	{
		ceError = LwAllocateString( ".", ppszSysDir );
		BAIL_ON_CENTERIS_ERROR(ceError);
		
		ceError = LwAllocateString( pszFileSysPathCopy, ppszBase );
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	
error:
	
	if ( ceError != 0 )
	{
		if ( *ppszSysDir )
			LwFreeString( *ppszSysDir );
		
		if ( *ppszBase )
			LwFreeString( *ppszBase );
	}
	
	if ( pszFileSysPathCopy )
		LwFreeString( pszFileSysPathCopy );
		
	return ceError;
}

CENTERROR
GPOCrackFileSysPath(
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
    //PSTR pszTempPath = NULL;
    PSTR pSysVol = NULL;
    PSTR pPolicy = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszPolicyIdentifier = NULL;

    pTemp = pszFileSysPath + 2;
    pSysVol = strchr(pTemp, '\\');

    /* TODO: make sure pSysVol points to \\sysvol\\ */
    memset(szDomainName, 0, sizeof(szDomainName));
    strncpy(szDomainName, pTemp, pSysVol - pTemp);
    szDomainName[pSysVol - pTemp] = 0;

    ceError = LwAllocateString(szDomainName, &pszDomainName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString(pSysVol + sizeof("\\sysvol\\") - 1, &pszSourcePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pTemp = pSysVol + sizeof("\\sysvol\\");
    pPolicy = strchr(pTemp, '{');

    /* TODO: make sure pPolicy points to end of \\sysvol\\{ */
    if ( pPolicy && strlen(pPolicy) >= 38 ) {
        strcpy(szPolicyId, pPolicy);
        szPolicyId[38] = 0;
    } else {
        ceError = CENTERROR_BAD_GUID;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString(szPolicyId, &pszPolicyIdentifier);
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
