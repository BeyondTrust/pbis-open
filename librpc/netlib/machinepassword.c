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
 * Abstract: Machine trust password handling (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


static
VOID
GenerateRandomString(
    PWSTR   pwszBuffer,
    size_t  sBufferLen
    );


static
DWORD
CharacterClassesInPassword(
    const wchar16_t *password,
    size_t len);


static WINERR SavePrincipalKey(const wchar16_t *name, const wchar16_t *pass,
                               UINT32 pass_len, const wchar16_t *realm,
                               const wchar16_t *salt, const wchar16_t *dc_name,
                               UINT32 kvno)
{
    UINT32 ktstatus = 0;
    WINERR err = ERROR_SUCCESS;
    wchar16_t *principal = NULL;

    BAIL_ON_INVALID_PTR(name, err);
    BAIL_ON_INVALID_PTR(pass, err);
    BAIL_ON_INVALID_PTR(dc_name, err);

    ktstatus = KtKrb5FormatPrincipalW(name, realm, &principal);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto cleanup;
    }

    ktstatus = KtKrb5AddKeyW(principal, (void*)pass, pass_len,
                             NULL, salt, dc_name, kvno);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto cleanup;
    }

cleanup:
    if (principal)
    {
        KtFreeMemory(principal);
    }

    return err;

error:
    goto cleanup;
}


WINERR
SaveMachinePassword(
    const wchar16_t *machine,
    const wchar16_t *machacct_name,
    const wchar16_t *mach_dns_domain,
    const wchar16_t *domain_name,
    const wchar16_t *dns_domain_name,
    const wchar16_t *dc_name,
    const wchar16_t *sid_str,
    const wchar16_t *password
    )
{
    WINERR err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    UINT32 ktstatus = 0;
    wchar16_t *account = NULL;
    wchar16_t *dom_name = NULL;
    wchar16_t *ad_dns_dom_name_lc = NULL;
    wchar16_t *ad_dns_dom_name_uc = NULL;
    wchar16_t *mach_dns_dom_name_uc = NULL;
    wchar16_t *mach_dns_dom_name_lc = NULL;
    wchar16_t *sid = NULL;
    wchar16_t *hostname_uc = NULL;
    wchar16_t *hostname_lc = NULL;
    wchar16_t *pass = NULL;
    LWPS_PASSWORD_INFO pi = {0};
    size_t pass_len = 0;
    UINT32 kvno = 0;
    wchar16_t *base_dn = NULL;
    wchar16_t *salt = NULL;
    /* various forms of principal name for keytab */
    wchar16_t *host_machine_uc = NULL;
    wchar16_t *host_machine_lc = NULL;
    wchar16_t *host_machine_fqdn = NULL;
    size_t host_machine_fqdn_size = 0;
    wchar16_t *cifs_machine_fqdn = NULL;
    size_t cifs_machine_fqdn_size = 0;
    wchar16_t *principal = NULL;

    account = wc16sdup(machacct_name);
    BAIL_ON_NO_MEMORY(account, err);

    dom_name = wc16sdup(domain_name);
    BAIL_ON_NO_MEMORY(dom_name, err);

    ad_dns_dom_name_lc = wc16sdup(dns_domain_name);
    BAIL_ON_NO_MEMORY(ad_dns_dom_name_lc, err);

    wc16slower(ad_dns_dom_name_lc);

    ad_dns_dom_name_uc = wc16sdup(dns_domain_name);
    BAIL_ON_NO_MEMORY(ad_dns_dom_name_uc, err);

    wc16supper(ad_dns_dom_name_uc);

    mach_dns_dom_name_uc = wc16sdup(mach_dns_domain);
    BAIL_ON_NO_MEMORY(mach_dns_dom_name_uc, err);

    wc16supper(mach_dns_dom_name_uc);

    mach_dns_dom_name_lc = wc16sdup(mach_dns_domain);
    BAIL_ON_NO_MEMORY(mach_dns_dom_name_lc, err);

    wc16slower(mach_dns_dom_name_lc);

    sid = wc16sdup(sid_str);
    BAIL_ON_NO_MEMORY(sid, err);

    hostname_uc = wc16sdup(machine);
    BAIL_ON_NO_MEMORY(hostname_uc, err);

    wc16supper(hostname_uc);

    hostname_lc = wc16sdup(machine);
    BAIL_ON_NO_MEMORY(hostname_lc, err);

    wc16slower(hostname_lc);

    pass = wc16sdup(password);
    BAIL_ON_NO_MEMORY(pass, err);

    /*
     * Store the machine password first
     */

    pi.pwszDomainName      = dom_name;
    pi.pwszDnsDomainName   = ad_dns_dom_name_lc;
    pi.pwszSID             = sid;
    pi.pwszHostname        = hostname_uc;
    pi.pwszHostDnsDomain   = mach_dns_dom_name_lc;
    pi.pwszMachineAccount  = account;
    pi.pwszMachinePassword = pass;
    pi.last_change_time    = time(NULL);
    pi.dwSchannelType      = SCHANNEL_WKSTA;

    status = LwpsWritePasswordToAllStores(&pi);
    if (status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
        goto error;
    }

    pass_len = wc16slen(pass);

    /* TODO: sort out error code propagation from libkeytab functions */


    /*
     * Find the current key version number for machine account
     */

    ktstatus = KtKrb5FormatPrincipalW(account, ad_dns_dom_name_uc, &principal);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;
    }

    /* Get the directory base naming context first */
    ktstatus = KtLdapGetBaseDnW(dc_name, &base_dn);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;
    }

    ktstatus = KtLdapGetKeyVersionW(dc_name, base_dn, principal, &kvno);
    if (ktstatus == KT_STATUS_LDAP_NO_KVNO_FOUND) {
        /* This is probably win2k DC we're talking to, because it doesn't
           store kvno in directory. In such case return default key version */
        kvno = 0;

    } else if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;
    }

    ktstatus = KtGetSaltingPrincipalW(machine, account, mach_dns_domain, ad_dns_dom_name_uc,
                                      dc_name, base_dn, &salt);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;

    } else if (ktstatus == 0 && salt == NULL) {
        salt = wc16sdup(principal);
    }

    /*
     * Update keytab records with various forms of machine principal
     */

    /* MACHINE$@DOMAIN.NET */
    err = SavePrincipalKey(account, pass, pass_len, ad_dns_dom_name_uc, salt, dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    /* host/MACHINE@DOMAIN.NET */

    host_machine_uc = asw16printfw(L"host/%ws", hostname_uc);
    if (host_machine_uc == NULL)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_uc, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    /* host/machine.domain.net@DOMAIN.NET */
    host_machine_fqdn_size = wc16slen(hostname_lc) +
                             wc16slen(mach_dns_dom_name_lc) +
                             8;
    host_machine_fqdn = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                            host_machine_fqdn_size);
    BAIL_ON_NO_MEMORY(host_machine_fqdn, err);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_lc,
                mach_dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_uc,
                mach_dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_uc,
                mach_dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_lc,
                mach_dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    /* host/machine@DOMAIN.NET */
    host_machine_lc = asw16printfw(L"host/%ws", hostname_lc);
    if (host_machine_lc == NULL)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_lc, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    /* cifs/machine.domain.net@DOMAIN.NET */
    cifs_machine_fqdn_size = wc16slen(hostname_lc) +
                             wc16slen(mach_dns_dom_name_lc) +
                             8;
    cifs_machine_fqdn = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                            cifs_machine_fqdn_size);
    BAIL_ON_NO_MEMORY(cifs_machine_fqdn, err);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_lc,
                mach_dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_uc,
                mach_dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, ad_dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_uc,
                mach_dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, ad_dns_dom_name_uc, salt,
                               dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_lc,
                mach_dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WIN_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, mach_dns_dom_name_uc, salt,
                               dc_name, kvno);
    BAIL_ON_WIN_ERROR(err);

cleanup:
    if (base_dn) KtFreeMemory(base_dn);
    if (salt) KtFreeMemory(salt);

    SAFE_FREE(dom_name);
    SAFE_FREE(ad_dns_dom_name_lc);
    SAFE_FREE(ad_dns_dom_name_uc);
    SAFE_FREE(mach_dns_dom_name_lc);
    SAFE_FREE(mach_dns_dom_name_uc);
    SAFE_FREE(sid);
    SAFE_FREE(hostname_lc);
    SAFE_FREE(hostname_uc);
    SAFE_FREE(pass);
    SAFE_FREE(account);
    SAFE_FREE(host_machine_uc);
    SAFE_FREE(host_machine_lc);
    SAFE_FREE(host_machine_fqdn);
    SAFE_FREE(cifs_machine_fqdn);
    SAFE_FREE(principal);

    return err;

error:
    goto cleanup;
}

VOID
GenerateMachinePassword(
    wchar16_t *password,
    size_t len)
{
    const DWORD dwMaxGenerationAttempts = 1000;
    DWORD dwGenerationAttempts = 0;

    password[0] = '\0';
    do
    {
        GenerateRandomString(password, len);

        dwGenerationAttempts++;

    } while (dwGenerationAttempts <= dwMaxGenerationAttempts &&
             CharacterClassesInPassword(password, len) < 3);

    if (!(dwGenerationAttempts <= dwMaxGenerationAttempts))
    {
        abort();
    }
}


static
const CHAR
RandomCharsSet[] = "abcdefghijklmnoprstuvwxyz"
                   "ABCDEFGHIJKLMNOPRSTUVWXYZ"
                   "-+/*,.;:!<=>%'&()0123456789";

static
VOID
GenerateRandomString(
    PWSTR   pwszBuffer,
    size_t  sBufferLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pBuffer = NULL;
    DWORD i = 0;

    dwError = LwAllocateMemory(sizeof(pBuffer[0]) * sBufferLen,
                               OUT_PPVOID(&pBuffer));
    BAIL_ON_WIN_ERROR(dwError);

    if (!RAND_bytes((unsigned char*)pBuffer, (int)sBufferLen))
    {
        goto error;
    }

    for (i = 0; i < sBufferLen - 1; i++)
    {
        DWORD iChar = pBuffer[i] % (sizeof(RandomCharsSet) - 1);
        pwszBuffer[i] = (WCHAR)RandomCharsSet[iChar];
    }

    pwszBuffer[sBufferLen - 1] = (WCHAR)'\0';

cleanup:
    LW_SAFE_FREE_MEMORY(pBuffer);

    return;

error:
    memset(pwszBuffer, 0, sizeof(pwszBuffer[0] * sBufferLen));

    goto cleanup;
}

static
DWORD
CharacterClassesInPassword(
    const wchar16_t* password,
    size_t len)
{
    DWORD dwClassesSeen = 0;
    BOOLEAN bHasUpperCase = FALSE;
    BOOLEAN bHasLowerCase = FALSE;
    BOOLEAN bHasDigit = FALSE;
    BOOLEAN bHasNonAlphaNumeric = FALSE;
    size_t i = 0;

    for (i = 0; i < len; i++)
    {
        if ('A' <= password[i] && password[i] <= 'Z')
        {
            bHasUpperCase = TRUE;
        }
        else if ('a' <= password[i] && password[i] <= 'z')
        {
            bHasLowerCase = TRUE;
        }
        else if ('0' <= password[i] && password[i] <= '9')
        {
            bHasDigit = TRUE;
        }
        else if (strchr( "-+/*,.;:!<=>%'&()", password[i]) != NULL)
        {
            // This may be a better list to check against:
            //       `~!@#$%^&*()_+-={}|[]\:";'<>?,./
            bHasNonAlphaNumeric = TRUE;
        }
    }
    if (bHasUpperCase)
        dwClassesSeen++;
    if (bHasLowerCase)
        dwClassesSeen++;
    if (bHasDigit)
        dwClassesSeen++;
    if (bHasNonAlphaNumeric)
        dwClassesSeen++;

    return dwClassesSeen;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
