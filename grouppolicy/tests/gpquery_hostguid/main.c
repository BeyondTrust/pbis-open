#include "gpagent.h"

int
main(
    int argc,
    char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszHostGUID = NULL;
    PSTR pszMachineName = NULL;
    PSTR pszADDomain = NULL;
    HANDLE hDirectory = (HANDLE)NULL;

    GPOSetServerDefaults();

    ceError = GPAInitKrb5();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOOpenDirectory2(&hDirectory, &pszMachineName, &pszADDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetHostGUID(hDirectory, pszMachineName, pszADDomain, &pszHostGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    printf("Host GUID:%s\n", pszHostGUID ? pszHostGUID : "(null)");

    return 0;

  error:

    printf("Failed to find host GUID. Error code: %.8x\n", ceError);

    return(ceError);
}


