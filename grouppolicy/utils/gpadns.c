#include "includes.h"

CENTERROR
GetDnsSystemNames(
    char ** hostname,
    char ** machinename,
    char ** domain )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char buffer[256];
    char * szLocal = NULL;
    char * szDot = NULL;
    int len = 0;
    PSTR pszHostName = NULL;
    PSTR pszMachineName = NULL;
    PSTR pszDomain = NULL;
    PLWPS_PASSWORD_INFO pMachineAcctInfo = NULL;
    HANDLE hPasswordStore = (HANDLE)NULL;

    if ( gethostname(buffer, sizeof(buffer)) != 0 )
    {
        ceError = CENTERROR_GP_GETHOSTNAME_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    len = strlen(buffer);
    if ( len > strlen(".local") )
    {
        szLocal = &buffer[len - strlen(".local")];
        if ( !strcasecmp( szLocal, ".local" ) )
        {
            szLocal[0] = '\0';
        }
    }

    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    szDot = strchr(buffer, '.');
    if ( szDot )
    {
        szDot[0] = '\0';
    }

    ceError = LwpsOpenPasswordStore(
                  LWPS_PASSWORD_STORE_DEFAULT,
                  &hPasswordStore);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwpsGetPasswordByHostName(
                  hPasswordStore,
                  buffer,
                  &pMachineAcctInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszHostName = awc16stombs(pMachineAcctInfo->pwszHostname);
    if ( !pszHostName )
    {
        ceError = CENTERROR_GP_STRING_CONVERSION_FAILED;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszMachineName = awc16stombs(pMachineAcctInfo->pwszMachineAccount);
    if ( !pszMachineName )
    {
        ceError = CENTERROR_GP_STRING_CONVERSION_FAILED;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszDomain = awc16stombs(pMachineAcctInfo->pwszDnsDomainName);
    if ( !pszDomain )
    {
        ceError = CENTERROR_GP_STRING_CONVERSION_FAILED;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( hostname ) {
        *hostname = pszHostName;
        pszHostName = NULL;
    }

    if ( machinename ) {
        *machinename = pszMachineName;
        pszMachineName = NULL;
    }

    if ( domain ) {
        *domain = pszDomain;
        pszDomain = NULL;
    }

    // return below so that we free the unrequested out parameters.

error:

    if ( pszHostName )
    {
        LwFreeString(pszHostName);
    }

    if ( pszMachineName )
    {
        LwFreeString(pszMachineName);
    }

    if ( pszDomain )
    {
        LwFreeString(pszDomain);
    }

    if (pMachineAcctInfo)
    {
        LwpsFreePasswordInfo(hPasswordStore, pMachineAcctInfo);
    }

    if (hPasswordStore) 
    {
       LwpsClosePasswordStore(hPasswordStore);
    }

    return(ceError);
}

