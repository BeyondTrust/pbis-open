#include "gpagent.h"

int
main(
    int argc,
    char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*PSTR pszHostGUID = NULL;*/
    PSTR pszMachineName = NULL;
    PSTR pszComputerDN = NULL;
    PSTR pszADDomain = NULL;
    PSTR pszDomainControllerName = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    int i = 0;

    GPOSetServerDefaults();

    ceError = GPAInitKrb5();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOGetADDomain( &pszADDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GetDomainController( pszADDomain, &pszDomainControllerName );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GetDnsSystemNames(NULL, &pszMachineName, NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOOpenDirectory(pszADDomain, pszDomainControllerName, &hDirectory);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (i = 0; i < 525600; i++) {
        
	    ceError = GPAFindComputerDN(hDirectory,
                                	pszMachineName,
                                	pszADDomain,
                                	&pszComputerDN);
   		BAIL_ON_CENTERIS_ERROR(ceError);

        printf("Iteration:%d;Computer DN: %s\n", i+1, pszComputerDN);

        if (pszComputerDN) {
           LwFreeString(pszComputerDN);
           pszComputerDN = NULL;
        }
    }

        if (pszMachineName) {
           LwFreeString(pszMachineName);
        }

        if (pszADDomain) {
           LwFreeString(pszADDomain);
        }

        if (pszComputerDN) {
           LwFreeString(pszComputerDN);
        }

        if (pszDomainControllerName) {
           LwFreeString(pszDomainControllerName);
        }

    if (hDirectory) {
       GPOCloseDirectory(hDirectory);
    }

    return 0;

  error:

    printf("Error code: %.8x\n", ceError);

    return(ceError);
}

