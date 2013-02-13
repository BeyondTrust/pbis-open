/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include "config.h"
#include <lwautoenroll/lwautoenroll.h>

#include <bail.h>
#include <ldap_util.h>
#include <mspki.h>

#include <krb5/krb5.h>

#include <lsa/lsa.h>

#include <lwerror.h>
#include <lwkrb5.h>
#include <lwldap.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lw/swab.h>

#include <ldap.h>
#include <string.h>

#define LDAP_BASE   "cn=Public Key Services,cn=Services,cn=Configuration"

static DWORD
LwAutoEnrollGetUrls(
        IN HANDLE ldapConnection,
        IN LDAP *pLdap,
        IN LDAPMessage *pServiceLdapResults,
        IN OUT PLW_AUTOENROLL_TEMPLATE pTemplate
        )
{
    LDAPMessage *pLdapResult = NULL;
    struct berval **ppValues = NULL;
    int numValues = 0;
    int value = 0;
    PLW_AUTOENROLL_SERVICE pService = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    for (pLdapResult = LwLdapFirstEntry(
                            ldapConnection,
                            pServiceLdapResults);
            pLdapResult != NULL;
            pLdapResult = LwLdapNextEntry(
                            ldapConnection,
                            pLdapResult)
        )
    {
        ldap_value_free_len(ppValues);
        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "certificateTemplates");
        numValues = ldap_count_values_len(ppValues);

        for (value = 0; value < numValues; ++value)
        {
            if (!strncmp(
                    pTemplate->name,
                    ppValues[value]->bv_val,
                    ppValues[value]->bv_len))
            {
                ldap_value_free_len(ppValues);
                ppValues = ldap_get_values_len(
                                pLdap,
                                pLdapResult,
                                "msPKI-Enrollment-Servers");
                if (ppValues != NULL)
                {
                    int i;
                    int numValues;

                    numValues = ldap_count_values_len(ppValues);
                    for (i = 0; i < numValues; ++i)
                    {
                        PSTR serverInfo = ppValues[i]->bv_val;
                        int priority;
                        int authType;
                        int renewOnly;
                        PLW_AUTOENROLL_SERVICE *ppService = NULL;

                        priority = strtoul(serverInfo, &serverInfo, 10);
                        if (*serverInfo != '\n')
                        {
                            continue;
                        }

                        ++serverInfo;
                        authType = strtoul(serverInfo, &serverInfo, 10);
                        if (*serverInfo != '\n')
                        {
                            continue;
                        }

                        if (authType != 2) // kerberos
                        {
                            continue;
                        }

                        ++serverInfo;
                        renewOnly = strtoul(serverInfo, &serverInfo, 10);
                        if (*serverInfo != '\n')
                        {
                            continue;
                        }

                        ++serverInfo;

                        error = LwAllocateMemory(
                                    sizeof(*pService),
                                    (PVOID*) &pService);
                        BAIL_ON_LW_ERROR(error);

                        error = LwAllocateString(
                                    serverInfo,
                                    &pService->url);
                        BAIL_ON_LW_ERROR(error);

                        pService->priority = priority;
                        pService->renewOnly = renewOnly;

                        for (ppService = &pTemplate->pEnrollmentServices;
                                *ppService != NULL;
                                ppService = &((*ppService)->next))
                        {
                            if ((*ppService)->priority > priority)
                            {
                                break;
                            }
                        }

                        pService->next = *ppService;
                        *ppService = pService;
                    }
                }

                break;
            }
        }
    }

cleanup:
    if (ppValues)
    {
        ldap_value_free_len(ppValues);
    }

    return error;
}

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
    PSTR domainDnsName = NULL;
    LDAP *pLdap = NULL;
    LDAPMessage *pLdapResults = NULL;
    LDAPMessage *pServiceLdapResults = NULL;
    LDAPMessage *pLdapResult = NULL;
    PCSTR domainDn = NULL;
    PLW_AUTOENROLL_TEMPLATE pTemplateList = NULL;
    DWORD numTemplates = 0;
    struct berval **ppValues = NULL;
    PSTR name = NULL;
    PSTR displayName = NULL;
    PSTR csp = NULL;
    EXTENDED_KEY_USAGE *extendedKeyUsage = NULL;
    STACK_OF(ASN1_OBJECT) *criticalExtensions = NULL;
    DWORD error = LW_ERROR_SUCCESS;
    static PSTR templateAttributeList[] =
    {
        "name",
        "displayName",
        "pKIKeyUsage",
        "pKIExtendedKeyUsage",
        "pKICriticalExtensions",
        "pKIDefaultCSPs",
        "msPKI-Enrollment-Flag",
        "msPKI-Certificate-Name-Flag",
        "msPKI-Minimal-Key-Size",
        NULL
    };
    static PSTR serviceAttributeList[] =
    {
        "certificateTemplates",
        "msPKI-Enrollment-Servers",
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

        error = LwKrb5SetThreadDefaultCachePath(credentialsCache, NULL);
        BAIL_ON_LW_ERROR(error);
    }

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
                LSA_OBJECT_TYPE_USER,
                LSA_QUERY_TYPE_BY_UPN,
                1,
                QueryList,
                &ppObjects);
    BAIL_ON_LW_ERROR(error);

    if (ppObjects == NULL || ppObjects[0] == NULL)
    {
        BAIL_WITH_LW_ERROR(LW_ERROR_NO_SUCH_USER);
    }

    error = LwAutoEnrollFindDomainDn(
                ppObjects[0]->pszDN,
                &domainDn);
    BAIL_ON_LW_ERROR(error);

    error = LwLdapConvertDNToDomain(
                domainDn,
                &domainDnsName);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapConnect(
                domainDnsName,
                &ldapConnection);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapSearch(
                ldapConnection,
                domainDn,
                LDAP_BASE,
                LDAP_SCOPE_SUBTREE,
                "(objectClass=pKIEnrollmentService)",
                serviceAttributeList,
                &pServiceLdapResults);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapSearch(
                ldapConnection,
                domainDn,
                LDAP_BASE,
                LDAP_SCOPE_SUBTREE,
                "(objectClass=pKICertificateTemplate)",
                templateAttributeList,
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

    pLdap = LwLdapGetSession(ldapConnection);

    numTemplates = 0;

    for (pLdapResult = LwLdapFirstEntry(
                            ldapConnection,
                            pLdapResults);
            pLdapResult != NULL;
            pLdapResult = LwLdapNextEntry(
                            ldapConnection,
                            pLdapResult)
        )
    {
        int value = 0;
        int numValues = 0;
        DWORD enrollmentFlags = 0;
        DWORD nameFlags = 0;
        DWORD keyUsage = 0;
        DWORD keySize = 0;

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
            sk_ASN1_OBJECT_pop_free(criticalExtensions, ASN1_OBJECT_free);
            criticalExtensions = NULL;
        }

        if (ppValues)
        {
            ldap_value_free_len(ppValues);
            ppValues = NULL;
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

        memcpy(&keyUsage, ppValues[0]->bv_val, ppValues[0]->bv_len);
        keyUsage = LW_LTOH32(keyUsage);

        ldap_value_free_len(ppValues);
        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "displayName");
        if (ppValues == NULL)
        {
            error = LwAllocateString(
                        name,
                        &displayName);
            BAIL_ON_LW_ERROR(error);
        }
        else
        {
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
            ppValues = NULL;
        }

        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "pKIExtendedKeyUsage");
        if (ppValues != NULL)
        {
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
            ppValues = NULL;
        }

        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "pKICriticalExtensions");
        if (ppValues != NULL)
        {
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
            ppValues = NULL;
        }

        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "msPKI-Minimal-Key-Size");
        if (ppValues != NULL)
        {
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
            ppValues = NULL;
        }

        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "msPKI-Certificate-Name-Flag");
        if (ppValues != NULL)
        {
            if (ldap_count_values_len(ppValues) != 1)
            {
                ldap_value_free_len(ppValues);
                ppValues = NULL;
                continue;
            }

            nameFlags = strtoul(ppValues[0]->bv_val,
                                NULL,
                                0);

            ldap_value_free_len(ppValues);
            ppValues = NULL;
        }

        ppValues = ldap_get_values_len(
                        pLdap,
                        pLdapResult,
                        "pKIDefaultCSPs");
        if (ppValues == NULL)
        {
            // The CA will accept any CSP; supply a reasonable default.
            error = LwAllocateString(
                        "1,Microsoft Enhanced Cryptographic Provider v1.0",
                        &csp);
            BAIL_ON_LW_ERROR(error);
        }
        else
        {
            // Pick the preferred CSP from the list.
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

            ldap_value_free_len(ppValues);
            ppValues = NULL;
        }

        pTemplateList[numTemplates].name = name;
        pTemplateList[numTemplates].displayName = displayName;
        pTemplateList[numTemplates].csp = csp;
        pTemplateList[numTemplates].keyUsage = keyUsage;
        pTemplateList[numTemplates].keySize = keySize;
        pTemplateList[numTemplates].enrollmentFlags = enrollmentFlags;
        pTemplateList[numTemplates].nameFlags = nameFlags;
        pTemplateList[numTemplates].extendedKeyUsage = extendedKeyUsage;
        pTemplateList[numTemplates].criticalExtensions = criticalExtensions;

        error = LwAutoEnrollGetUrls(
                    ldapConnection,
                    pLdap,
                    pServiceLdapResults,
                    &pTemplateList[numTemplates]);
        BAIL_ON_LW_ERROR(error);

        if (pTemplateList[numTemplates].pEnrollmentServices == NULL)
        {
            /* No enrollment services for this template, so skip it. */
            continue;
        }

        name = NULL;
        displayName = NULL;
        extendedKeyUsage = NULL;
        criticalExtensions = NULL;
        csp = NULL;
        ++numTemplates;
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

    LW_SAFE_FREE_STRING(name);
    LW_SAFE_FREE_STRING(displayName);

    if (krbContext)
    {
        if (krbUpn)
        {
            krb5_free_unparsed_name(krbContext, krbUpn);
        }

        if (krbPrincipal)
        {
            krb5_free_principal(krbContext, krbPrincipal);
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

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }

    if (ldapConnection)
    {
        LwLdapCloseDirectory(ldapConnection);
    }

    if (pLdapResults)
    {
        ldap_msgfree(pLdapResults);
    }

    if (pServiceLdapResults)
    {
        ldap_msgfree(pServiceLdapResults);
    }

    if (extendedKeyUsage)
    {
        EXTENDED_KEY_USAGE_free(extendedKeyUsage);
    }

    if (criticalExtensions)
    {
        sk_ASN1_OBJECT_free(criticalExtensions);
    }

    if (ppValues)
    {
        ldap_value_free_len(ppValues);
    }

    LW_SAFE_FREE_STRING(domainDnsName);

    *ppTemplateList = pTemplateList;
    *pNumTemplates = numTemplates;

    return error;
}

static VOID
LwAutoEnrollFreeServiceList(
        IN PLW_AUTOENROLL_SERVICE pService
        )
{
    while (pService)
    {
        PLW_AUTOENROLL_SERVICE next = pService->next;

        LW_SAFE_FREE_STRING(pService->url);
        LwFreeMemory(pService);
        pService = next;
    }
}

VOID
LwAutoEnrollFreeTemplateList(
        IN PLW_AUTOENROLL_TEMPLATE pTemplateList,
        IN DWORD numTemplates
        )
{
    DWORD template;

    for (template = 0; template < numTemplates; ++template)
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
            sk_ASN1_OBJECT_pop_free(
                    pTemplateList[template].criticalExtensions,
                    ASN1_OBJECT_free);
        }

        if (pTemplateList[template].extensions)
        {
            sk_X509_EXTENSION_free(pTemplateList[template].extensions);
        }

        if (pTemplateList[template].attributes)
        {
            sk_X509_ATTRIBUTE_free(pTemplateList[template].attributes);
        }

        LwAutoEnrollFreeServiceList(
            pTemplateList[template].pEnrollmentServices);
    }

    LW_SAFE_FREE_MEMORY(pTemplateList);
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
