/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsakrb5.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Kerberos 5 API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rszczesniak@likewisesoftware.com)
 *
 */

#include "includes.h"


DWORD
KtKrb5GetDefaultRealm(
    PSTR* ppszRealm
    )
{
    DWORD dwError = 0;
    krb5_context ctx = NULL;
    PSTR pszKrb5Realm = NULL;
    PSTR pszRealm = NULL;

    krb5_init_context(&ctx);
    krb5_get_default_realm(ctx, &pszKrb5Realm);

    if (IsNullOrEmptyString(pszKrb5Realm)) {
        dwError = KT_STATUS_KRB5_NO_DEFAULT_REALM;
        BAIL_ON_KT_ERROR(dwError);
    }

    dwError = KtAllocateString(pszKrb5Realm, &pszRealm);
    BAIL_ON_KT_ERROR(dwError);

    *ppszRealm = pszRealm;
    
cleanup:

    if (pszKrb5Realm){
        krb5_free_default_realm(ctx,pszKrb5Realm);
    }

    krb5_free_context(ctx);

    return(dwError);

error:

    *ppszRealm = NULL;
    
    KT_SAFE_FREE_STRING(pszRealm);

    goto cleanup;
}


DWORD
KtKrb5GetSystemCachePath(
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    
    switch (cacheType)
    {
        case KRB5_InMemory_Cache:
            
            dwError = KtAllocateString(
                        "MEMORY:krb5cc_lsass",
                        &pszCachePath);
            BAIL_ON_KT_ERROR(dwError);
            
            break;
            
        case KRB5_File_Cache:
            
            dwError = KtAllocateString(
                        "FILE:/tmp/krb5cc_0",
                        &pszCachePath);
            BAIL_ON_KT_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = KT_STATUS_INVALID_PARAMETER;
            BAIL_ON_KT_ERROR(dwError);
            
            break;
    }
    
    *ppszCachePath = pszCachePath;
    
cleanup:

    return dwError;
    
error:

    *ppszCachePath = NULL;
    
    goto cleanup;
}


DWORD
KtKrb5GetUserCachePath(
    uid_t         uid,
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    
    switch (cacheType)
    {
        case KRB5_InMemory_Cache:
            
            dwError = KtAllocateStringPrintf(
                        &pszCachePath,
                        "MEMORY:krb5cc_%ld",
                        (long)uid);
            BAIL_ON_KT_ERROR(dwError);
            
            break;
            
        case KRB5_File_Cache:
            
            dwError = KtAllocateStringPrintf(
                        &pszCachePath,
                        "FILE:/tmp/krb5cc_%ld",
                        (long)uid);
            BAIL_ON_KT_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = KT_STATUS_INVALID_PARAMETER;
            BAIL_ON_KT_ERROR(dwError);
            
            break;
    }
    
    *ppszCachePath = pszCachePath;
    
cleanup:

    return dwError;
    
error:

    *ppszCachePath = NULL;
    
    goto cleanup;
}


DWORD
KtKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    )
{
    DWORD dwError       = 0;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;
    PSTR  pszOrigCachePath = NULL;

    dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            pszCachePath,
                            (ppszOrigCachePath) ? (const char**)&pszOrigCachePath : NULL);
    BAIL_ON_SEC_ERROR(dwMajorStatus);
    
    if (ppszOrigCachePath) {
        if (!IsNullOrEmptyString(pszOrigCachePath)) {
            dwError = KtAllocateString(pszOrigCachePath, ppszOrigCachePath);
            BAIL_ON_KT_ERROR(dwError);
        } else {
            *ppszOrigCachePath = NULL;
        }
    }
    
cleanup:

    return dwError;
    
error:

    if (ppszOrigCachePath) {
        *ppszOrigCachePath = NULL;
    }

    goto cleanup;
}


DWORD
KtKrb5GetSystemKeytabPath(
    PSTR* ppszKeytabPath
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    PSTR pszPath = NULL;
    size_t size = 64;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    do {
        KT_SAFE_FREE_STRING(pszPath);

        size *= 2;
        dwError = KtAllocateMemory(size, (PVOID*)&pszPath);
        BAIL_ON_KT_ERROR(dwError);

        ret = krb5_kt_default_name(ctx, pszPath, size);
    } while (ret == KRB5_CONFIG_NOTENUFSPACE);
    
    BAIL_ON_KRB5_ERROR(ctx, ret);
    *ppszKeytabPath = pszPath;

cleanup:
    if (ctx) {
        krb5_free_context(ctx);
    }

    return dwError;

error:
    KT_SAFE_FREE_STRING(pszPath);
    *ppszKeytabPath = NULL;
    
    goto cleanup;
}


#if 0
static
BOOLEAN
Krb5TicketHasExpired()
{
    if (gdwKrbTicketExpiryTime == 0) {

        LSA_LOG_VERBOSE("Acquiring new Krb5 ticket...");
        return TRUE;

    } else if (difftime(gdwKrbTicketExpiryTime,time(NULL)) < gdwExpiryGraceSeconds) {

        LSA_LOG_VERBOSE("Renewing Krb5 Ticket...");
        return TRUE;
    }

    return FALSE;
}
#endif


DWORD
KtKrb5Init(
    PCSTR pszMachname,
    PCSTR pszPassword
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;

    return (dwError);
}


DWORD
KtSetupMachineSession(
    PCSTR pszMachname,
    PCSTR pszPassword,
    PCSTR pszRealm,
    PCSTR pszDomain
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;
    PSTR pszHostKeytabFile = NULL;
    PSTR pszKrb5CcPath = NULL;
    PSTR pszHostname = NULL;
    PSTR pszDomname = NULL;
    PSTR pszRealmCpy = NULL;
    PSTR pszMachPrincipal = NULL;
    PSTR pszSrvPrincipal = NULL;
    PSTR pszHostPrincipal = NULL;
    PSTR pszDomainControllerName = NULL;
    /*
    dwError = LWNetGetDomainController(pszDomain, &pszDomainControllerName);
    BAIL_ON_KT_ERROR(dwError);
    */
    dwError = KtKrb5GetSystemKeytabPath(&pszHostKeytabFile);
    BAIL_ON_KT_ERROR(dwError);

    dwError = KtKrb5GetSystemCachePath(KRB5_File_Cache, &pszKrb5CcPath);
    BAIL_ON_KT_ERROR(dwError);

    dwError = KtAllocateString(pszRealm, &pszRealmCpy);
    BAIL_ON_KT_ERROR(dwError);
    KtStrToUpper(pszRealmCpy);

    dwError = KtAllocateStringPrintf(&pszMachPrincipal, "%s$@%s",
                                      pszMachname, pszRealm);
    BAIL_ON_KT_ERROR(dwError);

    dwError = KtAllocateStringPrintf(&pszSrvPrincipal, "krbtgt/%s@%s",
                                      pszDomainControllerName, pszRealm);
    BAIL_ON_KT_ERROR(dwError);

    dwError = KtAllocateString(pszMachname, &pszHostname);
    BAIL_ON_KT_ERROR(dwError);
    KtStrToLower(pszHostname);

    dwError = KtAllocateString(pszDomain, &pszDomname);
    BAIL_ON_KT_ERROR(dwError);
    KtStrToLower(pszDomname);

    dwError = KtAllocateStringPrintf(&pszHostPrincipal, "host/%s.%s@%s",
                                      pszHostname, pszDomname, pszRealm);
    BAIL_ON_KT_ERROR(dwError);

    dwError = KtKrb5GetTgt(pszMachPrincipal,
            pszPassword,
            pszKrb5CcPath);
    BAIL_ON_KT_ERROR(dwError);

    dwError = KtKrb5GetTgs(pszMachPrincipal, pszHostPrincipal,
                           pszKrb5CcPath);
    BAIL_ON_KT_ERROR(dwError);

error:
    KT_SAFE_FREE_STRING(pszMachPrincipal);
    KT_SAFE_FREE_STRING(pszSrvPrincipal);
    KT_SAFE_FREE_STRING(pszHostPrincipal);
    KT_SAFE_FREE_STRING(pszHostname);
    KT_SAFE_FREE_STRING(pszDomname);
    KT_SAFE_FREE_STRING(pszRealmCpy);
    KT_SAFE_FREE_STRING(pszKrb5CcPath);
    
    return (dwError);
}


DWORD
KtKrb5SetConfFileSearchPath()
{
    DWORD dwError = 0;
    
    PCSTR pszKrb5ConfName = "KRB5_CONFIG";
    PCSTR pszKrb5ConfValueDefault = "/etc/krb5.conf";
    PCSTR pszKrb5ConfValueAdditional = "/var/lib/netlogon/krb5.conf";
    BOOLEAN bPathAlreadyPresent = FALSE;
    PSTR pszKrb5ConfValue = NULL;
    PSTR pszKrb5ConfValueOriginal = NULL;
    char* pszKrb5ConfValueOriginalPtr = NULL;
    PSTR pszKrb5ConfPutenvStr = NULL;
    PCSTR pszDelimiter = ":";
    PSTR pszSavePtr = NULL;
    PSTR pszToken = NULL;
    
    pszKrb5ConfValueOriginalPtr = getenv(pszKrb5ConfName);
    if (!IsNullOrEmptyString(pszKrb5ConfValueOriginalPtr))
    {
        dwError = KtAllocateString(
                    pszKrb5ConfValueOriginalPtr,
                    &pszKrb5ConfValueOriginal
                    );
        BAIL_ON_KT_ERROR(dwError);      
    }
    
    
    //If the previous value was empty, the behavior is to open /etc/krb5.conf.
    //To preserve this behavior, /etc/krb5.conf must be added expicitly.
    if (IsNullOrEmptyString(pszKrb5ConfValueOriginal))
    {
        dwError = KtAllocateStringPrintf(
                    &pszKrb5ConfValue,
                    "%s:%s",
                    pszKrb5ConfValueAdditional,
                    pszKrb5ConfValueDefault
                    );
        BAIL_ON_KT_ERROR(dwError);
    }
    
    //if a path is found in the original path string, 
    //do not add it or /etc/krb5.conf again
    else
    {
        pszToken = strtok_r(pszKrb5ConfValueOriginal, pszDelimiter, &pszSavePtr);
        
        while(!IsNullOrEmptyString(pszToken))
        {
            if(strcmp(pszToken, pszKrb5ConfValueAdditional) == 0)
            {
                bPathAlreadyPresent = TRUE;
            }
            pszToken = strtok_r(NULL, pszDelimiter, &pszSavePtr);
        }
        
        strcpy(pszKrb5ConfValueOriginal, pszKrb5ConfValueOriginalPtr);
        
        if(bPathAlreadyPresent)
        {
            dwError = KtAllocateString(
                    pszKrb5ConfValueOriginal,
                    &pszKrb5ConfValue
                    );
            BAIL_ON_KT_ERROR(dwError);
        }
        else
        {
            dwError = KtAllocateStringPrintf(
                        &pszKrb5ConfValue,
                        "%s:%s",
                        pszKrb5ConfValueAdditional,
                        pszKrb5ConfValueOriginal
                        );
            BAIL_ON_KT_ERROR(dwError);
        }
    }
    
    dwError = KtAllocateStringPrintf(
                &pszKrb5ConfPutenvStr,
                "%s=%s",
                pszKrb5ConfName,
                pszKrb5ConfValue
                );
    BAIL_ON_KT_ERROR(dwError);
    
    dwError = putenv(pszKrb5ConfPutenvStr);
    if(dwError)
    {
        dwError = errno;
        BAIL_ON_KT_ERROR(dwError);
    }
    
cleanup:

    KT_SAFE_FREE_STRING(pszKrb5ConfValueOriginal);
    KT_SAFE_FREE_STRING(pszKrb5ConfValue);

    return dwError;

error:

    KT_SAFE_FREE_STRING(pszKrb5ConfPutenvStr);

    goto cleanup;
    
}


DWORD
KtGetSaltingPrincipal(
    PCSTR pszMachineName,
    PCSTR pszMachAcctName,
    PCSTR pszDnsDomainName,
    PCSTR pszRealmName,
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PSTR *pszSalt)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    PSTR pszSaltOut = NULL;
    PSTR pszRealm = NULL;
    PSTR pszMachine = NULL;
    krb5_context ctx = NULL;

    /* Try to query for userPrincipalName attribute first */
    dwError = KtLdapGetSaltingPrincipal(pszDcName, pszBaseDn, pszMachAcctName,
                                         &pszSaltOut);
    BAIL_ON_KT_ERROR(dwError);

    if (pszSaltOut) {
        *pszSalt = pszSaltOut;
        goto cleanup;
    }

    if (pszRealmName) {
        /* Use passed realm name */
        dwError = KtAllocateString(pszRealmName, &pszRealm);
        BAIL_ON_KT_ERROR(dwError);

    } else {
        /* No realm name was passed so get the default */
        ret = krb5_init_context(&ctx);
        BAIL_ON_KRB5_ERROR(ctx, ret);

        ret = krb5_get_default_realm(ctx, &pszRealm);
        BAIL_ON_KRB5_ERROR(ctx, ret);
    }

    /* Ensure realm name uppercased */
    KtStrToUpper(pszRealm);

    /* Ensure host name lowercased */
    dwError = KtAllocateString(pszMachineName, &pszMachine);
    BAIL_ON_KT_ERROR(dwError);

    KtStrToLower(pszMachine);

    dwError = KtAllocateStringPrintf(&pszSaltOut, "host/%s.%s@%s",
                                     pszMachine, pszDnsDomainName,
                                     pszRealm);
    BAIL_ON_KT_ERROR(dwError);

    *pszSalt = pszSaltOut;

cleanup:
    if (ctx) {
        krb5_free_context(ctx);
    }

    KT_SAFE_FREE_STRING(pszRealm);
    KT_SAFE_FREE_STRING(pszMachine);

    return dwError;

error:
    *pszSalt = NULL;
    goto cleanup;
}


DWORD
KtGetSaltingPrincipalW(
    PCWSTR pwszMachineName,
    PCWSTR pwszMachAcctName,
    PCWSTR pwszDnsDomainName,
    PCWSTR pwszRealmName,
    PCWSTR pwszDcName,
    PCWSTR pwszBaseDn,
    PWSTR *pwszSalt)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    PSTR pszMachineName = NULL;
    PSTR pszMachAcctName = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszRealmName = NULL;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszSalt = NULL;

    pszMachineName = awc16stombs(pwszMachineName);
    BAIL_IF_NO_MEMORY(pszMachineName);

    pszMachAcctName = awc16stombs(pwszMachAcctName);
    BAIL_IF_NO_MEMORY(pszMachAcctName);

    pszDnsDomainName = awc16stombs(pwszDnsDomainName);
    BAIL_IF_NO_MEMORY(pszDnsDomainName);

    pszDcName = awc16stombs(pwszDcName);
    BAIL_IF_NO_MEMORY(pszDcName);

    pszBaseDn = awc16stombs(pwszBaseDn);
    BAIL_IF_NO_MEMORY(pszBaseDn);

    if (pwszRealmName) {
        pszRealmName = awc16stombs(pwszRealmName);
        BAIL_IF_NO_MEMORY(pszRealmName);
    }

    dwError = KtGetSaltingPrincipal(pszMachineName, pszMachAcctName,
                                    pszDnsDomainName,
                                    pszRealmName, pszDcName, pszBaseDn,
                                    &pszSalt);
    BAIL_ON_KT_ERROR(dwError);

    if (pszSalt) {
        *pwszSalt = ambstowc16s(pszSalt);
        BAIL_IF_NO_MEMORY(*pwszSalt);
    }

cleanup:
    KT_SAFE_FREE_STRING(pszMachineName);
    KT_SAFE_FREE_STRING(pszMachAcctName);
    KT_SAFE_FREE_STRING(pszDnsDomainName);
    KT_SAFE_FREE_STRING(pszRealmName);
    KT_SAFE_FREE_STRING(pszDcName);
    KT_SAFE_FREE_STRING(pszBaseDn);
    KT_SAFE_FREE_STRING(pszSalt);

    return dwError;

error:
    pwszSalt = NULL;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
