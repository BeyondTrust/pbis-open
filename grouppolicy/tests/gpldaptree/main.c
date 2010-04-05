#include "gpagent.h"
#include "gpagss.h"

#define LDAP_DN "distinguishedName"

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
    CHAR * pszUsername = NULL;
    CHAR * pszPassword = NULL;
    CHAR * pszServerName = NULL;
    LDAPMessage * pResult = NULL;
    char * szAttributeList[] = {"objectClass", LDAP_DN, NULL};
    PSTR pszValue = NULL;

    if ( argc < 4 )
    {	
	printf("Usage: ldap <domain name> <username@domain name> <password>\n" );
	exit(1);
    }

    pszADDomain = argv[1];
    pszUsername = argv[2];
    pszPassword = argv[3];

    /*GPOSetServerDefaults();*/

    gpa_init_logging_to_file(LOG_LEVEL_VERBOSE, "");

    /* ceError = GPOGetADDomain( &pszADDomain); */
    /* BAIL_ON_CENTERIS_ERROR(ceError); */

    ceError = GetDomainController( pszADDomain, &pszDomainControllerName );
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszServerName = (CHAR*)malloc( strlen("krbtgt/") + 2 * strlen(pszADDomain) + 1 );
    strcpy( pszServerName, "krbtgt/" );
    strcat( pszServerName, pszADDomain );
    strcat( pszServerName, "@" );
    strcat( pszServerName, pszADDomain );

    ceError =
	kerb5_gettgt_from_password2(
	    pszUsername,
	    pszPassword,
	    pszServerName /* "krbtgt/CORPQA.CENTERIS.COM@CORPQA.CENTERIS.COM" */
	    );

    BAIL_ON_CENTERIS_ERROR(ceError);

    printf("\nDC: %s\n", pszDomainControllerName );

    ceError = GPOOpenDirectory(pszADDomain, pszDomainControllerName, &hDirectory);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ConvertDomainToDN( pszADDomain, &pszDomainDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPODirectorySearch(
	                          hDirectory,
				  pszDomainDN,
				  LDAP_SCOPE_ONELEVEL,
				  "(objectClass=organizationalUnit)",			       
				  szAttributeList,
				  &pResult
	);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pResult = GPOFirstLDAPEntry(hDirectory, pResult);

    while( pResult != NULL )
    {
	ceError = GPAGetLDAPString(hDirectory,
				  pResult,
				  LDAP_DN,
				  &pszValue);

	BAIL_ON_CENTERIS_ERROR(ceError);

	if ( pszValue != NULL )
	{
	    printf( "%s\n", pszValue );
	    LwFreeString( pszValue );
	    pszValue = NULL;
	}

	pResult = GPONextLDAPEntry(hDirectory,pResult);	
    }
						
error:

    /*
    if ( pszADDomain )
    {
       LwFreeMemory(pszADDomain);
    }
    */

    if ( pszServerName)
    {
	LwFreeMemory(pszServerName);
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


