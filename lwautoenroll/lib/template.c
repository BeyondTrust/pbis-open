#include <bail.h>
#include <lwautoenroll/lwautoenroll.h>

#include <lsa/lsa.h>

#include <ldap.h>
#include <lwldap.h>

#include <lwmem.h>
#include <lwstr.h>

#define LDAP_QUERY  "(objectClass=pKICertificateTemplate)"
#define LDAP_BASE   "cn=Public Key Services,cn=Services,cn=Configuration"

DWORD
LwAutoEnrollGetTemplateList(
        OUT PLW_AUTOENROLL_TEMPLATE *ppTemplateList,
        OUT PDWORD pNumTemplates
        )
{
    HANDLE lsaConnection = (HANDLE) NULL;
    HANDLE ldapConnection = (HANDLE) NULL;
    PLSASTATUS pLsaStatus = NULL;
    LDAPMessage *pLdapResults = NULL;
    int provider;
    PSTR domainDN;
    PSTR searchDN;
    PLW_AUTOENROLL_TEMPLATE pTemplateList = NULL;
    DWORD numTemplates = 0;
    DWORD error;
    static PSTR attributeList[] =
    {
        "name",
        "pKIKeyUsage",
        "pKIExtendedKeyUsage",
        NULL
    };

    error = LsaOpenServer(&lsaConnection);
    BAIL_ON_LW_ERROR(error);
    error = LsaGetStatus(lsaConnection, &pLsaStatus);
    BAIL_ON_LW_ERROR(error);

    for (provider = 0; provider < pLsaStatus->dwCount; ++provider)
    {
        PLSA_AUTH_PROVIDER_STATUS pProviderStatus =
            &pLsaStatus->pAuthProviderStatusList[provider];

        if (pProviderStatus->mode == LSA_PROVIDER_MODE_LOCAL_SYSTEM)
        {
            continue;
        }

        if (pProviderStatus->pTrustedDomainInfoArray)
        {
            DWORD domain;

            for (domain = 0; domain < pProviderStatus->dwNumTrustedDomains;
                    ++domain)
            {
                PLSA_TRUSTED_DOMAIN_INFO pDomainInfo =
                    &pProviderStatus->pTrustedDomainInfoArray[domain];

                if (pDomainInfo->pDCInfo &&
                        !strcasecmp(pDomainInfo->pszDnsDomain,
                            pProviderStatus->pszDomain))
                {
                    PLSA_DC_INFO pDCInfo = pDomainInfo->pDCInfo;
                    LDAP *pLdap;
                    LDAPMessage *pLdapResult;
                    DWORD template = 0;
                    PSTR name = NULL;
                    EXTENDED_KEY_USAGE *extendedKeyUsage = NULL;

                    LW_LOG_DEBUG(
                        "Getting certificate templates for domain %s via DC %s",
                            pDomainInfo->pszDnsDomain,
                            pDCInfo->pszName);

                    error = LwLdapOpenDirectoryServer(pDCInfo->pszAddress,
                            pDCInfo->pszName, 0, &ldapConnection);
                    if (error)
                    {
                        char szErrorString[256];

                        LwGetErrorString(error, szErrorString,
                                sizeof(szErrorString));
                        LW_LOG_ERROR(
                            "Could not get certificate templates for domain %s"
                            " from DC %s [%s]: %s (error %d)",
                            pDomainInfo->pszDnsDomain,
                            pDCInfo->pszName,
                            pDCInfo->pszAddress,
                            szErrorString,
                            error);
                        continue;
                    }

                    pLdap = LwLdapGetSession(ldapConnection);

                    error = LwLdapConvertDomainToDN(pDomainInfo->pszDnsDomain,
                            &domainDN);
                    BAIL_ON_LW_ERROR(error);

                    error = LwAllocateStringPrintf(&searchDN, "%s,%s",
                            LDAP_BASE, domainDN);
                    BAIL_ON_LW_ERROR(error);

                    error = LwLdapDirectorySearch(ldapConnection,
                            searchDN, LDAP_SCOPE_SUBTREE,
                            LDAP_QUERY, attributeList,
                            &pLdapResults);
                    BAIL_ON_LW_ERROR(error);

                    LW_SAFE_FREE_STRING(searchDN);
                    error = LwLdapCountEntries(
                                ldapConnection,
                                pLdapResults,
                                &numTemplates);
                    BAIL_ON_LW_ERROR(error);

                    error = LwAllocateMemory(
                                numTemplates * sizeof(*pTemplateList),
                                (LW_PVOID*) &pTemplateList);
                    BAIL_ON_LW_ERROR(error);

                    for (pLdapResult = LwLdapFirstEntry(
                                            ldapConnection,
                                            pLdapResults);
                            pLdapResult != NULL;
                            pLdapResult = LwLdapNextEntry(
                                            ldapConnection,
                                            pLdapResult)
                        )
                    {
                        struct berval **ppValues;
                        int value;
                        int numValues;

                        LW_SAFE_FREE_STRING(name);

                        if (extendedKeyUsage)
                        {
                            sk_ASN1_OBJECT_free(extendedKeyUsage);
                            extendedKeyUsage = NULL;
                        }

                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "pKIKeyUsage");
                        if (ppValues == NULL ||
                                ldap_count_values_len(ppValues) != 1)
                        {
                            continue;
                        }

                        pTemplateList[template].keyUsage =
                            *((uint16_t *) (ppValues[0]->bv_val));

                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "name");
                        if (ppValues == NULL ||
                                ldap_count_values_len(ppValues) != 1)
                        {
                            continue;
                        }

                        error = LwAllocateString(
                                    ppValues[0]->bv_val,
                                    &name);
                        BAIL_ON_LW_ERROR(error);

                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "pKIExtendedKeyUsage");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        extendedKeyUsage = sk_ASN1_OBJECT_new_null();
                        numValues = ldap_count_values_len(ppValues);

                        for (value = 0; value < numValues; ++value)
                        {
                            ASN1_OBJECT *obj = NULL;
                            const unsigned char *data;

                            data = (unsigned char *) (ppValues[value]->bv_val);
                            obj = c2i_ASN1_OBJECT(NULL, &data,
                                    ppValues[value]->bv_len);
                            if (obj != NULL)
                            {
                                sk_ASN1_OBJECT_push(
                                        extendedKeyUsage,
                                        obj);
                                obj = NULL;
                                continue;
                            }
                        }

                        pTemplateList[template].name = name;
                        pTemplateList[template].extendedKeyUsage =
                            extendedKeyUsage;
                        name = NULL;
                        extendedKeyUsage = NULL;
                        ++template;
                    }

                    if (pLdapResults)
                    {
                        ldap_msgfree(pLdapResults);
                        pLdapResults = NULL;
                    }

                    LwLdapCloseDirectory(ldapConnection);
                    ldapConnection = (HANDLE) NULL;
                }
            }
        }
    }

cleanup:
    if (error)
    {
        if (pTemplateList)
        {
            LwAutoEnrollFreeTemplateList(pTemplateList, numTemplates);
            pTemplateList = NULL;
            numTemplates = 0;
        }
    }

    *ppTemplateList = pTemplateList;
    *pNumTemplates = numTemplates;

    return error;
}

VOID
LwAutoEnrollFreeTemplateList(
        IN PLW_AUTOENROLL_TEMPLATE pTemplateList,
        IN DWORD NumTemplates
        )
{
    DWORD template;

    for (template = 0; template < NumTemplates; ++template)
    {
        LW_SAFE_FREE_STRING(pTemplateList[template].name);

        if (pTemplateList[template].extendedKeyUsage)
        {
            sk_ASN1_OBJECT_free(pTemplateList[template].extendedKeyUsage);
        }
    }

    LwFreeMemory(pTemplateList);
}
