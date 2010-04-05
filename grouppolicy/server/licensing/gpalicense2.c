#include "gpagent.h"

CENTERROR
GPAGetAvailableLicenses(
    HANDLE hDirectory,
    PSTR  pszLicenseRoot,
    PSTR** pppLicenses,
    PDWORD pdwCount,
    BOOLEAN bSiteLicense,
    PSTR pszGUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LDAPMessage * pMessage = NULL;
    LDAPMessage * pEntry = NULL;
    PSTR pszLicenseDN = NULL;
    PSTR pszString = NULL;
    PSTR szAttributeList[] = {"distinguishedName", NULL};
    PSTR *ppLicenses = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    char buf[LICENSE_BUFSIZE];
    BOOLEAN bNeedServerLicense = FALSE;

    if(requireServerOrWorkstationLicense() & LICENSE_SERVER)
        bNeedServerLicense = TRUE;

    memset(buf, 0, LICENSE_BUFSIZE);
    sprintf(buf, "(& (description=assignedTo=%s)(description=siteLicense=%s)(description=serverLicense=%s))",
            (pszGUID != NULL) ? pszGUID : "",
            (bSiteLicense == TRUE) ? "True" : "False",
            (bNeedServerLicense == TRUE) ? "True" : "False");

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = GPODirectorySearch(hDirectory,
                                 pszLicenseRoot,
                                 LDAP_SCOPE_ONELEVEL,
                                 buf,
                                 szAttributeList,
                                 &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(pDirectory->ld,
                                 pMessage);
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (dwCount) {

        ceError = CTAllocateMemory(sizeof(PSTR) * dwCount, (PVOID*)&ppLicenses);
        BAIL_ON_CENTERIS_ERROR(ceError);

        i = 0;
        pEntry = ldap_first_entry(pDirectory->ld,
                                  pMessage);

        pszLicenseDN = ldap_get_dn(pDirectory->ld,
                                   pEntry);

        ceError = CTAllocateString(pszLicenseDN, &pszString);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pszLicenseDN) {
            ber_memfree(pszLicenseDN);
            pszLicenseDN = NULL;
        }

        *(ppLicenses + i) = pszString;
        pszString = NULL;

        i++;
        while(i < dwCount) {

            pEntry = ldap_next_entry(pDirectory->ld, pEntry);

            pszLicenseDN = ldap_get_dn(pDirectory->ld, pEntry);

            ceError = CTAllocateString(pszLicenseDN, &pszString);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (pszLicenseDN) {
                ber_memfree(pszLicenseDN);
                pszLicenseDN = NULL;
            }

            *(ppLicenses + i) = pszString;
            pszString = NULL;

            i++;
        }
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    *pppLicenses = ppLicenses;
    *pdwCount = dwCount;

    return ceError;

error:

    if (pszLicenseDN) {
        ber_memfree(pszLicenseDN);
    }

    if (pszString) {
        CTFreeString(pszString);
    }

    if (ppLicenses) {

        for(i = 0; i < dwCount; i++) {

            CTFreeString(ppLicenses[i]);

        }
        CTFreeMemory(ppLicenses);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    *pppLicenses = NULL;
    *pdwCount = 0;

    return(ceError);
}

CENTERROR
GPAGetAssignedLicense(
    HANDLE hDirectory,
    PSTR  pszLicenseRoot,
    PSTR** pppLicenses,
    PDWORD pdwCount,
    BOOLEAN bSiteLicense,
    PSTR pszGUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LDAPMessage * pMessage = NULL;
    LDAPMessage * pEntry = NULL;
    PSTR pszLicenseDN = NULL;
    PSTR pszString = NULL;
    PSTR szAttributeList[] = {"distinguishedName", NULL};
    PSTR *ppLicenses = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    char buf[LICENSE_BUFSIZE];
    BOOLEAN bNeedServerLicense = FALSE;

    if(requireServerOrWorkstationLicense() & LICENSE_SERVER)
        bNeedServerLicense = TRUE;

    memset(buf, 0, LICENSE_BUFSIZE);
    sprintf(buf, "(& (description=assignedTo=%s)(description=siteLicense=%s)(description=serverLicense=%s))",
            (pszGUID != NULL) ? pszGUID : "",
            (bSiteLicense == TRUE) ? "True" : "False",
            (bNeedServerLicense == TRUE) ? "True" : "False");

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = GPODirectorySearch(hDirectory,
                                 pszLicenseRoot,
                                 LDAP_SCOPE_BASE,
                                 buf,
                                 szAttributeList,
                                 &pMessage);
    if(ceError == LDAP_NO_SUCH_OBJECT)
    {
        dwCount = 0;
        ceError = CENTERROR_SUCCESS;
    }
    else
        BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(pDirectory->ld,
                                 pMessage);
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (dwCount) {

        ceError = CTAllocateMemory(sizeof(PSTR) * dwCount, (PVOID*)&ppLicenses);
        BAIL_ON_CENTERIS_ERROR(ceError);

        i = 0;
        pEntry = ldap_first_entry(pDirectory->ld,
                                  pMessage);

        pszLicenseDN = ldap_get_dn(pDirectory->ld,
                                   pEntry);

        ceError = CTAllocateString(pszLicenseDN, &pszString);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pszLicenseDN) {
            ber_memfree(pszLicenseDN);
            pszLicenseDN = NULL;
        }

        *(ppLicenses + i) = pszString;
        pszString = NULL;

        i++;
        while(i < dwCount) {

            pEntry = ldap_next_entry(pDirectory->ld, pEntry);

            pszLicenseDN = ldap_get_dn(pDirectory->ld, pEntry);

            ceError = CTAllocateString(pszLicenseDN, &pszString);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (pszLicenseDN) {
                ber_memfree(pszLicenseDN);
                pszLicenseDN = NULL;
            }

            *(ppLicenses + i) = pszString;
            pszString = NULL;

            i++;
        }
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    *pppLicenses = ppLicenses;
    *pdwCount = dwCount;

    return ceError;

error:

    if (pszLicenseDN) {
        ber_memfree(pszLicenseDN);
    }

    if (pszString) {
        CTFreeString(pszString);
    }

    if (ppLicenses) {

        for(i = 0; i < dwCount; i++) {

            CTFreeString(ppLicenses[i]);

        }
        CTFreeMemory(ppLicenses);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    *pppLicenses = NULL;
    *pdwCount = 0;

    return(ceError);
}

CENTERROR
GPAAssignLicense(
    HANDLE hDirectory,
    PSTR   pszLicenseDN,
    PSTR   pszGUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR szAttributeList[] = {"description", NULL};
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    LDAPMessage * pMessage = NULL;
    CHAR szBuffer[256];
    LDAPMod mod0;
    LDAPMod *mods[2];
    PSTR* ppValues = NULL;
    PSTR* ppNewValues = NULL;
    PSTR pszValue = NULL;
    DWORD dwCount = 0;
    DWORD dwValues = 0;
    DWORD i  = 0;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    memset(szBuffer, 0, 256);
    ceError = GPODirectorySearch(hDirectory,
                                 pszLicenseDN,
                                 LDAP_SCOPE_BASE,
                                 "(objectClass=*)",
                                 szAttributeList,
                                 &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(pDirectory->ld,
                                 pMessage);
    if (dwCount < 0) {
        ceError = CENTERROR_GP_LDAP_ERROR;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (dwCount) {

        ceError = GPAGetLDAPStrings(hDirectory,
                                    pMessage,
                                    "description",
                                    &ppValues,
                                    &dwValues);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (dwValues) {

            ceError = CTAllocateMemory((dwValues + 1) * sizeof(PSTR), (PVOID*)&ppNewValues);
            BAIL_ON_CENTERIS_ERROR(ceError);

            for (i = 0; i < dwValues; i++) {

                if (!strcmp(ppValues[i], "assignedTo=")) {

                    sprintf(szBuffer, "assignedTo=%s", pszGUID);

                    ceError = CTAllocateString(szBuffer, &pszValue);
                    ppNewValues[i] = pszValue;
                    pszValue = NULL;

                    if (ppValues[i]) {
                        CTFreeString(ppValues[i]);
                        ppValues[i] = NULL;
                    }

                    continue;
                }
                ppNewValues[i]= ppValues[i];
                ppValues[i] = NULL;
            }

            CTFreeMemory(ppValues);
            ppValues = NULL;
        }

        mod0.mod_op = LDAP_MOD_REPLACE;
        mod0.mod_type = "description";
        mod0.mod_values = ppNewValues;
        mods[0] = &mod0;
        mods[1] = NULL;

        ceError = ldap_modify_s(pDirectory->ld,
                                pszLicenseDN,
                                mods);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pszValue) {
        CTFreeString(pszValue);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    if (ppValues) {
        for (i = 0; i < dwValues; i++) {
            CTFreeString(ppValues[i]);
        }
        CTFreeMemory(ppValues);
    }

    if (ppNewValues) {
        for (i = 0; i < dwValues; i++) {
            CTFreeString(ppNewValues[i]);
        }
        CTFreeMemory(ppNewValues);
    }

    return(ceError);
}


