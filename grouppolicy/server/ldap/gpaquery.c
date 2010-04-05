
#include "includes.h"

CENTERROR
GPAGetHostGUID(
    HANDLE hDirectory,
    PSTR   pszHostname,
    PSTR   pszADDomain,
    PSTR*  ppszGUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR szAttributeList[] = { GPA_OBJECTGUID_ATTR, NULL};
    LDAPMessage* pMessage = NULL;
    int dwCount = 0;
    PSTR  pszValue = NULL;
    PSTR pszComputerDN = NULL;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = GPAFindComputerDN(hDirectory,
                                pszHostname,
                                pszADDomain,
                                &pszComputerDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPODirectorySearch(hDirectory,
                                 pszComputerDN,
                                 LDAP_SCOPE_BASE,
                                 "(objectclass=*)",
                                 szAttributeList,
                                 &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(pDirectory->ld,
                                 pMessage);
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;

    } else if (dwCount == 0) {

        ceError = CENTERROR_GP_LDAP_NO_SUCH_GUID;

    } else if (dwCount > 1) {

        ceError = CENTERROR_GP_LDAP_FOUND_MULTIPLE_GUIDS;

    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPGUID(hDirectory,
                             pMessage,
                             GPA_OBJECTGUID_ATTR,
                             &pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszGUID = pszValue;
    pszValue = NULL;

error:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    if (pszComputerDN) {
        LwFreeString(pszComputerDN);
    }

    return ceError;
}
