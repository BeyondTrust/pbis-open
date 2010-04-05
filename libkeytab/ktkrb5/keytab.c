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
 *        Kerberos 5 keytab management library
 *
 *        Public libkeytab API
 * 
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 */

#include "includes.h"

#define RDONLY_FILE  "FILE"
#define RDWR_FILE  "WRFILE"

static
VOID
KtKrb5FreeKeytabEntries(
    krb5_context ctx,
    krb5_keytab_entry *pEntries,
    INT count
    );

static
DWORD
KtKrb5KeytabOpen(
    PCSTR pszPrefix,
    PCSTR pszKtFile,
    krb5_context *pCtx,
    krb5_keytab  *pId
    )
{
    const DWORD dwMaxSize = 1024;
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_keytab id = 0;
    PSTR pszKtName = NULL;
    PSTR pszDefName = NULL;
    PSTR pszKtFilename = NULL;
    DWORD dwSize = 32;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    if (!pszKtFile) {

        do {
            dwSize += dwSize;

            dwError = KtReallocMemory((PVOID)pszDefName,
                                      (PVOID*)&pszDefName, dwSize);
            BAIL_ON_KT_ERROR(dwError);

            ret = krb5_kt_default_name(ctx, pszDefName, dwSize);
            if (ret == 0) {
                KtStrChr(pszDefName, ':', &pszKtFilename);
                pszKtFilename++;

            } else if (ret != KRB5_CONFIG_NOTENUFSPACE) {
                BAIL_ON_KRB5_ERROR(ctx, ret);
            }

        } while (ret == KRB5_CONFIG_NOTENUFSPACE &&
                 dwSize < dwMaxSize);
    }

    BAIL_ON_KRB5_ERROR(ctx, ret);

    dwError = KtAllocateStringPrintf(
                  &pszKtName,
                  "%s:%s",
                  pszPrefix,
                  pszKtFile ? pszKtFile : pszKtFilename);
    BAIL_ON_KT_ERROR(dwError);

    ret = krb5_kt_resolve(ctx, pszKtName, &id);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    *pId  = id;
    *pCtx = ctx;

error:
    if (ret && ctx)
    {
        krb5_free_context(ctx);
        ctx = NULL;
    }

    KT_SAFE_FREE_STRING(pszDefName);
    KT_SAFE_FREE_STRING(pszKtName);

    return dwError;
}

static
DWORD
KtKrb5SearchKeys(
    krb5_context ctx,
    krb5_keytab ktid,
    PCSTR pszSrvPrincipal,
    krb5_keytab_entry **ppEntries,
    INT *pCount
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_principal server = NULL;
    krb5_kt_cursor ktcursor = NULL;
    krb5_keytab_entry entry = {0};
    krb5_keytab_entry *entries = NULL;
    int num = 0;

    ret = krb5_parse_name(ctx, pszSrvPrincipal, &server);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    ret = krb5_kt_start_seq_get(ctx, ktid, &ktcursor);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    do {
        ret = krb5_kt_next_entry(ctx, ktid, &entry, &ktcursor);

        if (ret == 0 &&
            krb5_principal_compare(ctx, entry.principal, server)) {

            dwError = KtReallocMemory((PVOID)entries, (PVOID*)&entries,
                                       (num + 1) * sizeof(krb5_keytab_entry));
            BAIL_ON_KT_ERROR(dwError);

            memset(&entries[num], 0, sizeof(krb5_keytab_entry));
            num++;

            entries[num - 1].magic     = entry.magic;
            entries[num - 1].timestamp = entry.timestamp;
            entries[num - 1].vno       = entry.vno;

            ret = krb5_copy_principal(ctx, entry.principal,
                                      &entries[num - 1].principal);
            BAIL_ON_KRB5_ERROR(ctx, ret);

            entries[num - 1].key       = entry.key;

            ret = krb5_copy_keyblock_contents(ctx, &entry.key,
                                              &entries[num - 1].key);
            BAIL_ON_KRB5_ERROR(ctx, ret);
	}

        if (ret == 0) {
            krb5_free_keytab_entry_contents(ctx, &entry);
        }

    } while (ret != KRB5_KT_END);

    ret = krb5_kt_end_seq_get(ctx, ktid, &ktcursor);
    BAIL_ON_KRB5_ERROR(ctx, ret);

cleanup:
    if (server)
    {
        krb5_free_principal(ctx, server);
    }

    *ppEntries = entries;
    *pCount    = num;

    return dwError;

error:
    if (entries)
    {
        KtKrb5FreeKeytabEntries(ctx, entries, num);
    }

    entries = NULL;
    num     = 0;

    goto cleanup;
}


DWORD
KtKrb5AddKey(
    PCSTR pszPrincipal,
    PVOID pKey,
    DWORD dwKeyLen,
    PCSTR pszSalt,
    PCSTR pszKtPath,
    PCSTR pszDcName,
    DWORD dwKeyVer
    )

{
    const krb5_enctype enc[] = { ENCTYPE_DES_CBC_CRC,
                                 ENCTYPE_DES_CBC_MD5,
                                 ENCTYPE_ARCFOUR_HMAC };
    DWORD dwError = KT_STATUS_SUCCESS;
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
    krb5_data password = {0}, salt = {0};
    krb5_keyblock key = {0};
    krb5_encrypt_block key_encrypted = {0};
    DWORD dwKvno = 0;
    int count = 0;
    int i = 0;

    dwError = KtKrb5KeytabOpen(RDWR_FILE, pszKtPath, &ctx, &kt);
    BAIL_ON_KT_ERROR(dwError);

    if (dwKeyVer == (unsigned int)(-1)) {
        /* Try to find kvno querying ldap directory, if no kvno was passed */
        dwError = KtLdapGetBaseDn(pszDcName, &pszBaseDn);
        BAIL_ON_KT_ERROR(dwError);

        if (pszBaseDn) {
            dwError = KtLdapGetKeyVersion(pszDcName, pszBaseDn, pszPrincipal,
                                          &dwKvno);
            BAIL_ON_KT_ERROR(dwError);

            kvno = dwKvno;
        }

    } else {
        kvno = dwKeyVer;
    }

    dwError = KtKrb5SearchKeys(ctx, kt, pszPrincipal,
                               &entries, &count);
    /* Go straight to adding a key since there are no keys found */
    if (dwError == KT_STATUS_KRB5_NO_KEYS_FOUND) goto keyadd;

    /* The file doesn't exist yet, so it has no keys */
    if (dwError == ENOENT) goto keyadd;

    /* Handle other errors */
    BAIL_ON_KT_ERROR(dwError);

    /* Find the latest version of this key and remove old ones.
     * Key versions are stored as a single byte.
     */
    search_kvno = (krb5_octet)(kvno - 1);
    for (i = 0; i < count; i++) {
        if (search_kvno == entries[i].vno) {
           /* Don't remove the ones with just one version older */
           continue;
        }
        else
        {
            ret = krb5_kt_remove_entry(ctx, kt, &(entries[i]));
            BAIL_ON_KRB5_ERROR(ctx, ret);
        }
    }

keyadd:
    /* Cleanup status code */
    dwError = KT_STATUS_SUCCESS;

    ret = krb5_parse_name(ctx, pszPrincipal, &client);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    if (pszSalt) {
        ret = krb5_parse_name(ctx, pszSalt, &salt_principal);
        BAIL_ON_KRB5_ERROR(ctx, ret);

        ret = krb5_principal2salt(ctx, salt_principal, &salt);
        BAIL_ON_KRB5_ERROR(ctx, ret);
    }

    password.data   = pKey;
    password.length = dwKeyLen;

    for (i = 0; i < sizeof(enc)/sizeof(krb5_enctype); i++) {
        krb5_data *pass_salt = NULL;
        if (salt.data && salt.length) pass_salt = &salt;

        memset(&key, 0, sizeof(key));
        memset(&key_encrypted, 0, sizeof(key_encrypted));

        krb5_use_enctype(ctx, &key_encrypted, enc[i]);

        ret = krb5_string_to_key(ctx, &key_encrypted, &key, &password,
                                 pass_salt);
        BAIL_ON_KRB5_ERROR(ctx, ret);

        memset(&entry, 0, sizeof(entry));

        entry.principal = client;
        entry.vno       = kvno;
        entry.key       = key;

        ret = krb5_kt_add_entry(ctx, kt, &entry);
        BAIL_ON_KRB5_ERROR(ctx, ret);

        krb5_free_keyblock_contents(ctx, &key);
        memset(&key, 0, sizeof(key));
    }

cleanup:
    if ( ctx && entries )
    {
        KtKrb5FreeKeytabEntries(ctx, entries, count);
    }

    if (pszBaseDn)
    {
        KtFreeMemory(pszBaseDn);
    }

    KT_SAFE_FREE_MEMORY(salt.data);

    if (ctx)
    {
        krb5_free_keyblock_contents(ctx, &key);

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
    krb5_context ctx,
    krb5_keytab_entry *pEntries,
    INT count
    )
{
    int num = 0;

    for (num = 0; num < count ; num++)
    {
        krb5_free_keytab_entry_contents(ctx, &pEntries[num]);
    }

    KT_SAFE_FREE_MEMORY(pEntries);

    return;
}


DWORD
KtKrb5GetKey(
    PCSTR pszPrincipal,
    PCSTR pszKtPath,
    DWORD dwEncType,
    PVOID *pKey,
    DWORD *dwKeyLen
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_keytab ktid = 0;
    krb5_principal client;
    krb5_kvno vno = 0;
    krb5_enctype enctype = 0;
    krb5_keytab_entry entry = {0};

    dwError = KtKrb5KeytabOpen(RDONLY_FILE, pszKtPath, &ctx, &ktid);
    BAIL_ON_KT_ERROR(dwError);

    ret = krb5_parse_name(ctx, pszPrincipal, &client);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    enctype = (krb5_enctype)dwEncType;

    ret = krb5_kt_get_entry(ctx, ktid, client, vno, enctype, &entry);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    dwError = KtDuplicateMemory((PVOID)entry.key.contents,
                                (DWORD)entry.key.length, pKey);
    BAIL_ON_KT_ERROR(dwError);

    *dwKeyLen = entry.key.length;

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
    goto cleanup;
}


DWORD
KtKrb5RemoveKey(
    PSTR pszPrincipal,
    DWORD dwVer,
    PSTR pszKtPath
    )
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_keytab ktid = 0;
    krb5_keytab_entry *entries = NULL;
    int count = 0, i;

    dwError = KtKrb5KeytabOpen(RDWR_FILE, pszKtPath, &ctx, &ktid);
    BAIL_ON_KT_ERROR(dwError);

    /* Should enctypes be added to conditions ? */
    dwError = KtKrb5SearchKeys(ctx, ktid, pszPrincipal,
                               &entries, &count);
    BAIL_ON_KT_ERROR(dwError);

    for (i = 0; i < count; i++) {
        /* if dwVer is non-zero skip entries with different kvno */
        if (dwVer > 0 && dwVer != entries[i].vno) continue;

        ret = krb5_kt_remove_entry(ctx, ktid, &(entries[i]));
        BAIL_ON_KRB5_ERROR(ctx, ret);
    }

error:
    if (ctx)
    {
        if (entries)
        {
            for (i = 0; i < count; i++) {
                krb5_free_principal(ctx, entries[i].principal);
            }
            KT_SAFE_FREE_MEMORY(entries);
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
KtKrb5FormatPrincipal(
    PCSTR pszAccount,
    PCSTR pszRealm,
    PSTR *ppszPrincipal)
{
    DWORD dwError = KT_STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    char *realm = NULL;
    int i = 0;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB5_ERROR(ctx, ret);

    if (pszRealm) {
        dwError = KtAllocateString(pszRealm, &realm);
        BAIL_ON_KT_ERROR(dwError);
    } else {
        ret = krb5_get_default_realm(ctx, &realm);
        BAIL_ON_KRB5_ERROR(ctx, ret);
    }

    for (i = 0; i < strlen(realm); i++)
    {
        *(realm+i) = toupper((int)*(realm+i));
    }

    dwError = KtAllocateStringPrintf(ppszPrincipal, "%s@%s",
                                     pszAccount, realm);
    BAIL_ON_KT_ERROR(dwError);

cleanup:

    KT_SAFE_FREE_STRING(realm);

    if (ctx)
    {
       krb5_free_context(ctx);
    }

    return dwError;

error:
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
