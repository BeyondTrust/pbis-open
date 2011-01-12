#include <lwautoenroll/lwautoenroll.h>

#include <bail.h>
#include <mspki.h>

#include <krb5/krb5.h>

#include <lsa/lsa.h>

#include <ldap.h>
#include <lwldap.h>

#include <lwerror.h>
#include <lwkrb5.h>
#include <lwmem.h>
#include <lwstr.h>

#include <string.h>

#define LDAP_QUERY  "(objectClass=pKICertificateTemplate)"
#define LDAP_BASE   "cn=Public Key Services,cn=Services,cn=Configuration"

DWORD
LwAutoEnrollGetTemplateList(
        IN OPTIONAL PCSTR credentialsCache,
        OUT PLW_AUTOENROLL_TEMPLATE *ppTemplateList,
        OUT PDWORD pNumTemplates
        )
{
    HANDLE lsaConnection = (HANDLE) NULL;
    HANDLE ldapConnection = (HANDLE) NULL;
    krb5_error_code krbResult = 0;
    krb5_context krbContext = NULL;
    krb5_ccache krbCache = NULL;
    krb5_principal krbPrincipal = NULL;
    PSTR krbUpn = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList = { 0 };
    PLSASTATUS pLsaStatus = NULL;
    LDAPMessage *pLdapResults = NULL;
    int provider;
    PSTR domainDN = NULL;
    PSTR searchDN = NULL;
    PLW_AUTOENROLL_TEMPLATE pTemplateList = NULL;
    DWORD numTemplates = 0;
    struct berval **ppValues = NULL;
    PSTR name = NULL;
    PSTR displayName = NULL;
    PSTR csp = NULL;
    LDAP *pLdap = NULL;
    LDAPMessage *pLdapResult = NULL;
    EXTENDED_KEY_USAGE *extendedKeyUsage = NULL;
    STACK_OF(ASN1_OBJECT) *criticalExtensions = NULL;
    DWORD error = LW_ERROR_SUCCESS;
    static PSTR attributeList[] =
    {
        "name",
        "displayName",
        "pKIKeyUsage",
        "pKIExtendedKeyUsage",
        "pKICriticalExtensions",
        "pKIDefaultCSPs",
        "msPKI-Enrollment-Flag",
        "msPKI-Minimal-Key-Size",
        NULL
    };

    krbResult = krb5_init_context(&krbContext);
    BAIL_ON_KRB_ERROR(krbResult, krbContext);

    if (credentialsCache == NULL)
    {
        krbResult = krb5_cc_default(krbContext, &krbCache);
        BAIL_ON_KRB_ERROR(krbResult, krbContext);
    }
    else
    {
        krbResult = krb5_cc_resolve(krbContext, credentialsCache, &krbCache);
        BAIL_ON_KRB_ERROR(krbResult, krbContext);

        error = LwKrb5SetDefaultCachePath(credentialsCache, NULL);
        BAIL_ON_LW_ERROR(error);
    }

    error = LsaOpenServer(&lsaConnection);
    BAIL_ON_LW_ERROR(error);

    krbResult = krb5_cc_get_principal(krbContext, krbCache, &krbPrincipal);
    BAIL_ON_KRB_ERROR(krbResult, krbContext);

    krbResult = krb5_unparse_name(krbContext, krbPrincipal, &krbUpn);
    BAIL_ON_KRB_ERROR(krbResult, krbContext);

    error = LsaOpenServer(&lsaConnection);
    BAIL_ON_LW_ERROR(error);

    QueryList.ppszStrings = (PCSTR*) &krbUpn;

    error = LsaFindObjects(
                lsaConnection,
                NULL,
                0,
                LSA_OBJECT_TYPE_UNDEFINED,
                LSA_QUERY_TYPE_BY_UPN,
                1,
                QueryList,
                &ppObjects);
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
                        !strcasecmp(pDomainInfo->pszNetbiosDomain,
                            ppObjects[0]->pszNetbiosDomainName))
                {
                    PLSA_DC_INFO pDCInfo = pDomainInfo->pDCInfo;
                    DWORD template = 0;

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
                        int value;
                        int numValues;
                        DWORD enrollmentFlags;
                        DWORD keyUsage;
                        DWORD keySize;

                        LW_SAFE_FREE_STRING(name);
                        LW_SAFE_FREE_STRING(displayName);
                        LW_SAFE_FREE_STRING(csp);

                        if (extendedKeyUsage)
                        {
                            EXTENDED_KEY_USAGE_free(extendedKeyUsage);
                            extendedKeyUsage = NULL;
                        }

                        if (criticalExtensions)
                        {
                            sk_ASN1_OBJECT_free(criticalExtensions);
                            criticalExtensions = NULL;
                        }

                        if (ppValues)
                        {
                            ldap_value_free_len(ppValues);
                        }

                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "msPKI-Enrollment-Flag");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        if (ldap_count_values_len(ppValues) != 1)
                        {
                            ldap_value_free_len(ppValues);
                            ppValues = NULL;
                            continue;
                        }

                        enrollmentFlags = strtoul(
                                            ppValues[0]->bv_val,
                                            NULL,
                                            16);
                        if (!(enrollmentFlags & CT_FLAG_AUTO_ENROLLMENT))
                        {
                            ldap_value_free_len(ppValues);
                            ppValues = NULL;
                            continue;
                        }

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "name");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        if (ldap_count_values_len(ppValues) != 1)
                        {
                            ldap_value_free_len(ppValues);
                            ppValues = NULL;
                            continue;
                        }

                        error = LwAllocateString(
                                    (PCSTR) ppValues[0]->bv_val,
                                    &name);
                        BAIL_ON_LW_ERROR(error);

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "displayName");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        if (ldap_count_values_len(ppValues) != 1)
                        {
                            ldap_value_free_len(ppValues);
                            ppValues = NULL;
                            continue;
                        }

                        error = LwAllocateString(
                                    (PCSTR) ppValues[0]->bv_val,
                                    &displayName);
                        BAIL_ON_LW_ERROR(error);

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "pKIKeyUsage");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        if (ldap_count_values_len(ppValues) != 1)
                        {
                            ldap_value_free_len(ppValues);
                            ppValues = NULL;
                            continue;
                        }

                        keyUsage = *((PWORD) (ppValues[0]->bv_val));

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "displayName");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        if (ldap_count_values_len(ppValues) != 1)
                        {
                            ldap_value_free_len(ppValues);
                            ppValues = NULL;
                            continue;
                        }

                        error = LwAllocateString(
                                    (PCSTR) ppValues[0]->bv_val,
                                    &displayName);
                        BAIL_ON_LW_ERROR(error);

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "pKIExtendedKeyUsage");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        extendedKeyUsage = EXTENDED_KEY_USAGE_new();
                        numValues = ldap_count_values_len(ppValues);

                        for (value = 0; value < numValues; ++value)
                        {
                            ASN1_OBJECT *obj = NULL;
                            const unsigned char *data;

                            data = (unsigned char *) (ppValues[value]->bv_val);
                            obj = OBJ_txt2obj(
                                        ppValues[value]->bv_val,
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

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "pKICriticalExtensions");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        criticalExtensions = sk_ASN1_OBJECT_new_null();
                        numValues = ldap_count_values_len(ppValues);

                        for (value = 0; value < numValues; ++value)
                        {
                            ASN1_OBJECT *obj = NULL;
                            const unsigned char *data;

                            data = (unsigned char *) (ppValues[value]->bv_val);
                            obj = OBJ_txt2obj(
                                        ppValues[value]->bv_val,
                                        ppValues[value]->bv_len);
                            if (obj != NULL)
                            {
                                sk_ASN1_OBJECT_push(
                                        criticalExtensions,
                                        obj);
                                obj = NULL;
                                continue;
                            }
                        }

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "msPKI-Minimal-Key-Size");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        if (ldap_count_values_len(ppValues) != 1)
                        {
                            ldap_value_free_len(ppValues);
                            ppValues = NULL;
                            continue;
                        }

                        keySize = strtoul(ppValues[0]->bv_val,
                                            NULL,
                                            0);

                        ldap_value_free_len(ppValues);
                        ppValues = ldap_get_values_len(
                                        pLdap,
                                        pLdapResult,
                                        "pKIDefaultCSPs");
                        if (ppValues == NULL)
                        {
                            continue;
                        }

                        numValues = ldap_count_values_len(ppValues);

                        for (value = 0; value < numValues; ++value)
                        {
                            if (ppValues[value]->bv_val[0] != '1')
                            {
                                continue;
                            }

                            error = LwAllocateMemory(
                                        ppValues[value]->bv_len + 1,
                                        (PVOID*) &csp);
                            BAIL_ON_LW_ERROR(error);
                            strncpy(
                                csp,
                                ppValues[value]->bv_val,
                                ppValues[value]->bv_len);
                            csp[ppValues[value]->bv_len] = '\0';

                            break;
                        }

                        pTemplateList[template].name = name;
                        pTemplateList[template].displayName = displayName;
                        pTemplateList[template].csp = csp;
                        pTemplateList[template].keyUsage = keyUsage;
                        pTemplateList[template].keySize = keySize;
                        pTemplateList[template].enrollmentFlags =
                            enrollmentFlags;
                        pTemplateList[template].extendedKeyUsage =
                            extendedKeyUsage;
                        pTemplateList[template].criticalExtensions =
                            criticalExtensions;

                        name = NULL;
                        displayName = NULL;
                        extendedKeyUsage = NULL;
                        criticalExtensions = NULL;
                        csp = NULL;
                        ++template;
                    }
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

    LW_SAFE_FREE_STRING(domainDN);
    LW_SAFE_FREE_STRING(searchDN);
    LW_SAFE_FREE_STRING(name);
    LW_SAFE_FREE_STRING(displayName);

    if (krbContext)
    {
        if (krbUpn)
        {
            krb5_free_unparsed_name(krbContext, krbUpn);
        }

        if (krbCache)
        {
            krb5_cc_close(krbContext, krbCache);
        }

        krb5_free_context(krbContext);
    }

    if (lsaConnection)
    {
        LsaCloseServer(lsaConnection);
    }

    if (ldapConnection)
    {
        LwLdapCloseDirectory(ldapConnection);
    }

    if (pLdapResults)
    {
        ldap_msgfree(pLdapResults);
        pLdapResults = NULL;
    }

    if (extendedKeyUsage)
    {
        EXTENDED_KEY_USAGE_free(extendedKeyUsage);
        extendedKeyUsage = NULL;
    }

    if (criticalExtensions)
    {
        sk_ASN1_OBJECT_free(criticalExtensions);
        criticalExtensions = NULL;
    }

    if (ppValues)
    {
        ldap_value_free_len(ppValues);
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
        if (pTemplateList[template].name)
        {
            LwFreeString((PSTR) pTemplateList[template].name);
        }

        if (pTemplateList[template].displayName)
        {
            LwFreeString((PSTR) pTemplateList[template].displayName);
        }

        if (pTemplateList[template].csp)
        {
            LwFreeString((PSTR) pTemplateList[template].csp);
        }

        if (pTemplateList[template].extendedKeyUsage)
        {
            EXTENDED_KEY_USAGE_free(pTemplateList[template].extendedKeyUsage);
        }

        if (pTemplateList[template].criticalExtensions)
        {
            sk_ASN1_OBJECT_free(pTemplateList[template].criticalExtensions);
        }

        if (pTemplateList[template].extensions)
        {
            sk_X509_EXTENSION_free(pTemplateList[template].extensions);
        }

        if (pTemplateList[template].attributes)
        {
            sk_X509_ATTRIBUTE_free(pTemplateList[template].attributes);
        }
    }

    LW_SAFE_FREE_MEMORY(pTemplateList);
}
