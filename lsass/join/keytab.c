/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        keytab.c
 *
 * Abstract:
 *
 *        Kerberos 5 keytab functions
 *
 *        Public libkeytab API
 * 
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 */

#include "includes.h"

#define RDONLY_FILE  "FILE"
#define RDWR_FILE  "WRFILE"

#define BAIL_ON_KRB5_ERROR(ctx, krb5_err, winerr)         \
    if ((krb5_err)) {                                     \
        switch ((krb5_err))                               \
        {                                                 \
        case ENOENT:                                      \
            winerr = krb5_err;                            \
            break;                                        \
                                                          \
        case KRB5_LIBOS_BADPWDMATCH:                      \
            winerr = ERROR_WRONG_PASSWORD;                \
            break;                                        \
                                                          \
        case KRB5KDC_ERR_KEY_EXP:                         \
            winerr = ERROR_PASSWORD_EXPIRED;              \
            break;                                        \
                                                          \
        case KRB5KRB_AP_ERR_SKEW:                         \
            winerr = ERROR_TIME_SKEW;                     \
            break;                                        \
                                                          \
        default:                                          \
            winerr = LwTranslateKrb5Error(                \
                        (ctx),                            \
                        (krb5_err),                       \
                        __FUNCTION__,                     \
                        __FILE__,                         \
                        __LINE__);                        \
            break;                                        \
        }                                                 \
        goto error;                                       \
    }

static
VOID
KtKrb5FreeKeytabEntries(
    krb5_context       ctx,
    krb5_keytab_entry *pEntries,
    DWORD              dwCount
    );

static
DWORD
KtKrb5GetDefaultKeytab(
    PSTR* ppszKtFile
    )
{
    PSTR pszDefName = NULL;
    PSTR pszDefNameNew = NULL;
    const DWORD dwMaxSize = 1024;
    // Do not free
    PSTR pszKtFilename = NULL;
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    DWORD dwSize = 32;
    krb5_context ctx = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    do
    {
        dwSize += dwSize;

        dwError = LwReallocMemory(
                      (PVOID)pszDefName,
                      (PVOID*)&pszDefNameNew,
                      dwSize);
        BAIL_ON_LSA_ERROR(dwError);

        pszDefName = pszDefNameNew;
        pszDefNameNew = NULL;

        ret = krb5_kt_default_name(ctx, pszDefName, dwSize);
        if (ret == 0)
        {
            LwStrChr(pszDefName, ':', &pszKtFilename);

            if (!pszKtFilename)
            {
                pszKtFilename = pszDefName;
            }
            else
            {
                pszKtFilename++;
            }
        }
        else if (ret != KRB5_CONFIG_NOTENUFSPACE)
        {
            BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
        }
    }
    while (ret == KRB5_CONFIG_NOTENUFSPACE &&
           dwSize < dwMaxSize);

    dwError = LwAllocateString(pszKtFilename,
                               ppszKtFile);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (ctx)
    {
        krb5_free_context(ctx);
    }
    LW_SAFE_FREE_STRING(pszDefName);
    LW_SAFE_FREE_STRING(pszDefNameNew);
    return dwError;

error:
    *ppszKtFile = NULL;
    goto cleanup;
}

static
DWORD
KtKrb5KeytabOpen(
    PCSTR         pszPrefix,
    PCSTR         pszKtFile,
    krb5_context *pCtx,
    krb5_keytab  *pId
    )
{
    DWORD dwError = ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_keytab id = 0;
    PSTR pszKtName = NULL;
    PSTR pszKtFilename = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    if (!pszKtFile)
    {
        dwError = KtKrb5GetDefaultKeytab(&pszKtFilename);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                  &pszKtName,
                  "%s:%s",
                  pszPrefix,
                  pszKtFile ? pszKtFile : pszKtFilename);
    BAIL_ON_LSA_ERROR(dwError);

    ret = krb5_kt_resolve(ctx, pszKtName, &id);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    *pId  = id;
    *pCtx = ctx;

error:

    if (dwError && ctx)
    {
        krb5_free_context(ctx);
        ctx = NULL;
    }

    LW_SAFE_FREE_MEMORY(pszKtFilename);
    LW_SAFE_FREE_MEMORY(pszKtName);

    return dwError;
}


static
DWORD
KtKrb5SearchKeys(
    krb5_context        ctx,
    krb5_keytab         ktid,
    PCSTR               pszSrvPrincipal,
    krb5_keytab_entry **ppEntries,
    PDWORD              pdwCount
    )
{
    DWORD dwError = ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_principal server = NULL;
    krb5_kt_cursor ktcursor = NULL;
    krb5_keytab_entry entry = {0};
    krb5_keytab_entry *entries = NULL;
    DWORD dwCount = 0;

    ret = krb5_parse_name(ctx, pszSrvPrincipal, &server);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    ret = krb5_kt_start_seq_get(ctx, ktid, &ktcursor);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    do
    {
        ret = krb5_kt_next_entry(ctx, ktid, &entry, &ktcursor);

        if (ret == 0 &&
            krb5_principal_compare(ctx, entry.principal, server))
        {

            dwError = LwReallocMemory((PVOID)entries,
                                      (PVOID*)&entries,
                                      (dwCount + 1) * sizeof(krb5_keytab_entry));
            BAIL_ON_LSA_ERROR(dwError);

            memset(&entries[dwCount], 0, sizeof(krb5_keytab_entry));
            dwCount++;

            entries[dwCount - 1].magic     = entry.magic;
            entries[dwCount - 1].timestamp = entry.timestamp;
            entries[dwCount - 1].vno       = entry.vno;

            ret = krb5_copy_principal(ctx,
                                      entry.principal,
                                      &entries[dwCount - 1].principal);
            BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

            entries[dwCount - 1].key = entry.key;

            ret = krb5_copy_keyblock_contents(ctx,
                                              &entry.key,
                                              &entries[dwCount - 1].key);
            BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
        }

        if (ret == 0)
        {
            krb5_free_keytab_entry_contents(ctx, &entry);
        }
    }
    while (ret != KRB5_KT_END);

    ret = krb5_kt_end_seq_get(ctx, ktid, &ktcursor);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    if (dwCount == 0)
    {
        dwError = ERROR_FILE_NOT_FOUND;
    }

cleanup:
    if (server)
    {
        krb5_free_principal(ctx, server);
    }

    *ppEntries = entries;
    *pdwCount  = dwCount;

    return dwError;

error:
    if (entries)
    {
        KtKrb5FreeKeytabEntries(ctx, entries, dwCount);
    }

    entries = NULL;
    dwCount = 0;

    goto cleanup;
}

static
DWORD
KtKrb5AddKeyA(
    PCSTR  pszPrincipal,
    PVOID  pKey,
    DWORD  dwKeyLen,
    PCSTR  pszSalt,
    PCSTR  pszKtPath,
    PCSTR  pszDcName,
    DWORD  dwKeyVer
    )
{
    const krb5_enctype enc[] = { ENCTYPE_DES_CBC_CRC,
                                 ENCTYPE_DES_CBC_MD5,
                                 ENCTYPE_ARCFOUR_HMAC };
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszBaseDn = NULL;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_keytab kt = NULL;
    krb5_principal client = NULL;
    krb5_principal salt_principal = NULL;
    krb5_keytab_entry entry = {0};
    krb5_keytab_entry *entries = NULL;
    krb5_kvno kvno = 0;
    krb5_octet search_kvno = 0;
    krb5_data password = {0};
    krb5_data salt = {0};
    krb5_keyblock key = {0};
    krb5_encrypt_block key_encrypted = {0};
    DWORD dwKvno = 0;
    DWORD dwCount = 0;
    DWORD i = 0;
    struct stat statbuf = { 0 };
    PSTR pszKtPathLocal = NULL;
    int dwKeytabFd = -1;

    memset(&statbuf, 0, sizeof(struct stat));

    if (!pszKtPath)
    {
        dwError = KtKrb5GetDefaultKeytab(&pszKtPathLocal);
        BAIL_ON_LSA_ERROR(dwError);

        pszKtPath = pszKtPathLocal;
    }

    if (stat(pszKtPath, &statbuf) == 0 && statbuf.st_size == 0)
    {
        // The file is empty. Add the version number to the file so kerberos
        // will accept it.
        char byte = 0x05;

        dwKeytabFd = open(pszKtPath, O_WRONLY, 0);
        if (dwKeytabFd < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        while (write(dwKeytabFd, &byte, 1) < 0)
        {
            if (errno != EINTR)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        byte = 0x02;
        while (write(dwKeytabFd, &byte, 1) < 0)
        {
            if (errno != EINTR)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        while (close(dwKeytabFd) < 0)
        {
            if (errno != EINTR)
            {
                dwKeytabFd = -1;
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        dwKeytabFd = -1;
    }

    dwError = KtKrb5KeytabOpen(RDWR_FILE, pszKtPath, &ctx, &kt);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Try to find kvno by querying ldap directory, if no kvno was passed
     */
    if (dwKeyVer == (unsigned int)(-1))
    {
        dwError = KtLdapGetBaseDnA(pszDcName, &pszBaseDn);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszBaseDn)
        {
            dwError = KtLdapGetKeyVersionA(pszDcName,
                                           pszBaseDn,
                                           pszPrincipal,
                                           &dwKvno);
            BAIL_ON_LSA_ERROR(dwError);

            kvno = dwKvno;
        }
    }
    else
    {
        kvno = dwKeyVer;
    }

    /*
     * Search for existing versions of this principal's keys
     */
    dwError = KtKrb5SearchKeys(ctx,
                               kt,
                               pszPrincipal,
                               &entries,
                               &dwCount);
    if (dwError == ERROR_SUCCESS)
    {
        /*
         * Find the latest version of this key and remove old ones.
         * Key versions are stored as a single byte.
         */
        search_kvno = (krb5_octet)(kvno - 1);
        for (i = 0; i < dwCount; i++)
        {
            if (search_kvno == entries[i].vno)
            {
                /* Don't remove the keys with just one version number older */
                continue;
            }
            else
            {
                ret = krb5_kt_remove_entry(ctx, kt, &(entries[i]));
                BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
            }
        }
    }
    else if (dwError == ERROR_FILE_NOT_FOUND ||
             dwError == ENOENT)
    {
        /*
         * No key has been found or the file doesn't exist so
         * ignore the error and start adding the new keys
         */
        dwError = ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*
     * Prepare new key for this principal
     */
    ret = krb5_parse_name(ctx, pszPrincipal, &client);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    if (pszSalt)
    {
        ret = krb5_parse_name(ctx, pszSalt, &salt_principal);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

        ret = krb5_principal2salt(ctx, salt_principal, &salt);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
    }

    password.data   = pKey;
    password.length = dwKeyLen;

    /*
     * Add key entry for each encryption type
     */
    for (i = 0; i < sizeof(enc)/sizeof(krb5_enctype); i++)
    {
        krb5_data *pass_salt = NULL;

        memset(&key, 0, sizeof(key));
        memset(&key_encrypted, 0, sizeof(key_encrypted));
        memset(&entry, 0, sizeof(entry));

        if (salt.data && salt.length)
        {
            pass_salt = &salt;
        }

        krb5_use_enctype(ctx, &key_encrypted, enc[i]);

        ret = krb5_string_to_key(ctx,
                                 &key_encrypted,
                                 &key,
                                 &password,
                                 pass_salt);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

        entry.principal = client;
        entry.vno       = kvno;
        entry.key       = key;

        ret = krb5_kt_add_entry(ctx, kt, &entry);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

        krb5_free_keyblock_contents(ctx, &key);
    }

cleanup:
    if (ctx && entries)
    {
        KtKrb5FreeKeytabEntries(ctx, entries, dwCount);
    }

    LW_SAFE_FREE_MEMORY(pszBaseDn);
    LW_SAFE_FREE_MEMORY(salt.data);
    LW_SAFE_FREE_STRING(pszKtPathLocal);
    if (dwKeytabFd != -1)
    {
        close(dwKeytabFd);
    }

    if (ctx)
    {
        if (client)
        {
            krb5_free_principal(ctx, client);
        }

        if (salt_principal)
        {
            krb5_free_principal(ctx, salt_principal);
        }

        if (kt)
        {
            krb5_kt_close(ctx, kt);
        }

        if (key.length)
        {
            krb5_free_keyblock_contents(ctx, &key);
        }

        krb5_free_context(ctx);
    }

    return dwError;
    
error:
    goto cleanup;
}


static
VOID
KtKrb5FreeKeytabEntries(
    krb5_context       ctx,
    krb5_keytab_entry *pEntries,
    DWORD              dwCount
    )
{
    DWORD i = 0;

    for (i = 0; i < dwCount; i++)
    {
        krb5_free_keytab_entry_contents(ctx, &pEntries[i]);
    }

    LW_SAFE_FREE_MEMORY(pEntries);

    return;
}


DWORD
KtKrb5AddKeyW(
    PCWSTR   pwszPrincipal,
    PVOID    pKey,
    DWORD    dwKeyLen,
    PCWSTR   pwszKtPath,
    PCWSTR   pwszSalt,
    PCWSTR   pwszDcName,
    DWORD    dwKeyVersion)
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszPrincipal = NULL;
    PSTR pszKey = NULL;
    PSTR pszKtPath = NULL;
    PSTR pszSalt = NULL;
    PSTR pszDcName = NULL;

    dwError = LwWc16sToMbs(pwszPrincipal, &pszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16snToMbs((PWSTR)pKey,
                            &pszKey,
                            dwKeyLen + 1);
    BAIL_ON_LSA_ERROR(dwError)

    if (pwszKtPath)
    {
        dwError = LwWc16sToMbs(pwszKtPath, &pszKtPath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(pwszSalt, &pszSalt);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszDcName, &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtKrb5AddKeyA(pszPrincipal,
                            (PVOID)pszKey,
                            dwKeyLen,
                            pszSalt,
                            pszKtPath,
                            pszDcName,
                            dwKeyVersion);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pszPrincipal);
    LW_SAFE_FREE_MEMORY(pszKey);
    LW_SAFE_FREE_MEMORY(pszKtPath);
    LW_SAFE_FREE_MEMORY(pszSalt);
    LW_SAFE_FREE_MEMORY(pszDcName);

    return dwError;

error:
    goto cleanup;
}


DWORD
KtKrb5GetKey(
    PCSTR    pszPrincipal,
    PCSTR    pszKtPath,
    DWORD    dwEncType,
    PVOID   *ppKey,
    PDWORD   pdwKeyLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_keytab ktid = 0;
    krb5_principal client = NULL;
    krb5_kvno vno = 0;
    krb5_enctype enctype = 0;
    krb5_keytab_entry entry = {0};
    PVOID pKey = NULL;

    dwError = KtKrb5KeytabOpen(RDONLY_FILE,
                               pszKtPath,
                               &ctx,
                               &ktid);
    BAIL_ON_LSA_ERROR(dwError);

    ret = krb5_parse_name(ctx, pszPrincipal, &client);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    enctype = (krb5_enctype)dwEncType;

    ret = krb5_kt_get_entry(ctx, ktid, client, vno, enctype, &entry);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    dwError = LwAllocateMemory((DWORD)entry.key.length,
                               OUT_PPVOID(&pKey));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pKey, entry.key.contents, entry.key.length);

    *ppKey     = pKey;
    *pdwKeyLen = entry.key.length;

cleanup:
    if (ctx)
    {
        if (client)
        {
            krb5_free_principal(ctx, client);
        }

        if (ktid)
        {
           krb5_kt_close(ctx, ktid);
        }

        krb5_free_context(ctx);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pKey);

    *ppKey     = NULL;
    *pdwKeyLen = 0;

    goto cleanup;
}


DWORD
KtKrb5RemoveKey(
    PSTR   pszPrincipal,
    DWORD  dwVer,
    PSTR   pszKtPath
    )
{
    DWORD dwError = ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_keytab ktid = 0;
    krb5_keytab_entry *entries = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwError = KtKrb5KeytabOpen(RDWR_FILE,
                               pszKtPath,
                               &ctx,
                               &ktid);
    BAIL_ON_LSA_ERROR(dwError);

    /* Should enctypes be added to conditions ? */
    dwError = KtKrb5SearchKeys(ctx,
                               ktid,
                               pszPrincipal,
                               &entries,
                               &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwCount; i++)
    {
        /* if dwVer is non-zero skip entries with different kvno */
        if (dwVer > 0 && dwVer != entries[i].vno)
        {
            continue;
        }

        ret = krb5_kt_remove_entry(ctx, ktid, &(entries[i]));
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
    }

error:
    if (ctx)
    {
        if (entries)
        {
            for (i = 0; i < dwCount; i++)
            {
                krb5_free_principal(ctx, entries[i].principal);
            }
            LW_SAFE_FREE_MEMORY(entries);
        }

        if (ktid)
        {
            krb5_kt_close(ctx, ktid);
        }

        krb5_free_context(ctx);
    }

    return dwError;
}


DWORD
KtKrb5FormatPrincipalA(
    PCSTR  pszAccount,
    PCSTR  pszRealmName,
    PSTR  *ppszPrincipal
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszRealm = NULL;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    PSTR pszPrincipal = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

    if (pszRealmName)
    {
        dwError = LwAllocateString(pszRealmName,
                                   &pszRealm);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        ret = krb5_get_default_realm(ctx, &pszRealm);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
    }

    LwStrToUpper(pszRealm);

    dwError = LwAllocateStringPrintf(&pszPrincipal,
                                     "%s@%s",
                                     pszAccount,
                                     pszRealm);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPrincipal = pszPrincipal;

cleanup:
    if (pszRealmName)
    {
        LW_SAFE_FREE_MEMORY(pszRealm);
    }

    if (ctx)
    {
        if (pszRealm && !pszRealmName)
        {
            krb5_free_default_realm(ctx, pszRealm);
        }

        krb5_free_context(ctx);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszPrincipal);

    *ppszPrincipal = NULL;

    goto cleanup;
}


DWORD
KtKrb5FormatPrincipalW(
    PCWSTR   pwszAccount,
    PCWSTR   pwszRealm,
    PWSTR   *ppwszPrincipal
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszAccount = NULL;
    PSTR pszRealm = NULL;
    PSTR pszPrincipal = NULL;
    PWSTR pwszPrincipal = NULL;

    dwError = LwWc16sToMbs(pwszAccount, &pszAccount);
    BAIL_ON_LSA_ERROR(dwError);

    /* NULL realm is a valid argument of KtKrb5FormatPrincpal */
    if (pwszRealm)
    {
        dwError = LwWc16sToMbs(pwszRealm, &pszRealm);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = KtKrb5FormatPrincipalA(pszAccount,
                                     pszRealm,
                                     &pszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pszPrincipal, &pwszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    *ppwszPrincipal = pwszPrincipal;

cleanup:
    LW_SAFE_FREE_MEMORY(pszAccount);
    LW_SAFE_FREE_MEMORY(pszRealm);
    LW_SAFE_FREE_MEMORY(pszPrincipal);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszPrincipal);

    *ppwszPrincipal = NULL;

    goto cleanup;
}


DWORD
KtKrb5GetSaltingPrincipalA(
    PCSTR pszMachineName,
    PCSTR pszMachAcctName,
    PCSTR pszDnsDomainName,
    PCSTR pszRealmName,
    PCSTR pszDcName,
    PCSTR pszBaseDn,
    PSTR *pszSalt)
{
    DWORD dwError = ERROR_SUCCESS;
    krb5_error_code ret = 0;
    PSTR pszSaltOut = NULL;
    PSTR pszRealm = NULL;
    PSTR pszMachine = NULL;
    krb5_context ctx = NULL;

    /* Try to query for userPrincipalName attribute first */
    dwError = KtLdapGetSaltingPrincipalA(pszDcName,
                                         pszBaseDn,
                                         pszMachAcctName,
                                         &pszSaltOut);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszSaltOut)
    {
        *pszSalt = pszSaltOut;
        goto cleanup;
    }

    if (pszRealmName)
    {
        /* Use passed realm name */
        dwError = LwAllocateString(pszRealmName, &pszRealm);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* No realm name was passed so get the default */
        ret = krb5_init_context(&ctx);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);

        ret = krb5_get_default_realm(ctx, &pszRealm);
        BAIL_ON_KRB5_ERROR(ctx, ret, dwError);
    }

    /* Ensure realm name uppercased */
    LwStrToUpper(pszRealm);

    /* Ensure host name lowercased */
    dwError = LwAllocateString(pszMachineName, &pszMachine);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToLower(pszMachine);

    dwError = LwAllocateStringPrintf(&pszSaltOut,
                                     "host/%s.%s@%s",
                                     pszMachine,
                                     pszDnsDomainName,
                                     pszRealm);
    BAIL_ON_LSA_ERROR(dwError);

    *pszSalt = pszSaltOut;

cleanup:
    if (ctx)
    {
        krb5_free_context(ctx);
    }

    LW_SAFE_FREE_MEMORY(pszRealm);
    LW_SAFE_FREE_MEMORY(pszMachine);

    return dwError;

error:
    *pszSalt = NULL;
    goto cleanup;
}


DWORD
KtKrb5GetSaltingPrincipalW(
    PCWSTR  pwszMachineName,
    PCWSTR  pwszMachAcctName,
    PCWSTR  pwszDnsDomainName,
    PCWSTR  pwszRealmName,
    PCWSTR  pwszDcName,
    PCWSTR  pwszBaseDn,
    PWSTR  *ppwszSalt
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszMachineName = NULL;
    PSTR pszMachAcctName = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszRealmName = NULL;
    PSTR pszDcName = NULL;
    PSTR pszBaseDn = NULL;
    PSTR pszSalt = NULL;
    PWSTR pwszSalt = NULL;

    dwError = LwWc16sToMbs(pwszMachineName,
                           &pszMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszMachAcctName,
                           &pszMachAcctName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszDnsDomainName,
                           &pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszDcName,
                           &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszBaseDn,
                           &pszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszRealmName)
    {
        dwError = LwWc16sToMbs(pwszRealmName,
                               &pszRealmName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = KtKrb5GetSaltingPrincipalA(pszMachineName,
                                         pszMachAcctName,
                                         pszDnsDomainName,
                                         pszRealmName,
                                         pszDcName,
                                         pszBaseDn,
                                         &pszSalt);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszSalt)
    {
        dwError = LwMbsToWc16s(pszSalt, &pwszSalt);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppwszSalt = pwszSalt;

cleanup:
    LW_SAFE_FREE_MEMORY(pszMachineName);
    LW_SAFE_FREE_MEMORY(pszMachAcctName);
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pszRealmName);
    LW_SAFE_FREE_MEMORY(pszDcName);
    LW_SAFE_FREE_MEMORY(pszBaseDn);
    LW_SAFE_FREE_MEMORY(pszSalt);

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
