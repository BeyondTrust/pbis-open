#include "gpagent.h"
#include "gpagss.h"

int
main(
    int argc,
    char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    /*krb5_error_code ret = 0;*/
    HANDLE hDirectory = (HANDLE) NULL;
    CHAR * pszADDomain = NULL;
    CHAR * pszDomainDN = NULL;
    CHAR * pszDomainControllerName = NULL;
    LDAPMessage * pResult = NULL;
    /*char * szAttributeList[] = {"objectClass", NULL};*/

    GPOSetServerDefaults();

    gpa_init_logging_to_file(LOG_LEVEL_VERBOSE, "");

    ceError = GPOGetADDomain( &pszADDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GetDomainController( pszADDomain, &pszDomainControllerName );
    BAIL_ON_CENTERIS_ERROR(ceError);

    printf("\nDC: %s\n", pszDomainControllerName );

    ceError = GPOOpenDirectory(pszADDomain, pszDomainControllerName, &hDirectory);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ConvertDomainToDN( pszADDomain, &pszDomainDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOReadObject(
				hDirectory,
				pszDomainDN,
				NULL,
				&pResult
			);
    BAIL_ON_CENTERIS_ERROR(ceError);
						
error:

    if ( pszADDomain )
    {
       LwFreeMemory(pszADDomain);
    }

    if ( pszDomainDN )
    {
       LwFreeMemory(pszDomainDN);
    }

    if ( pszDomainControllerName )
    {
       LwFreeMemory(pszDomainControllerName);
    }

    return(ceError);
}


