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

#include "includes.h"


#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN (256)
#endif

NET_API_STATUS NetUnjoinDomainLocal(const wchar16_t *machine, 
                                    const wchar16_t *domain,
                                    const wchar16_t *account,
                                    const wchar16_t *password,
                                    UINT32 options)
{
    const UINT32 domain_access  = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                  DOMAIN_ACCESS_CREATE_USER;
    WINERR err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    ACCOUNT_HANDLE hAccount = NULL;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pi = NULL;
    PNET_CONN pConn = NULL;
    char *localname = NULL;
    wchar16_t *domain_controller_name = NULL;
    wchar16_t *machine_name = NULL;   
    PIO_CREDS creds = NULL;

    BAIL_ON_INVALID_PTR(machine, err);
    BAIL_ON_INVALID_PTR(domain, err);

    machine_name = wc16sdup(machine);
    BAIL_ON_NO_MEMORY(machine_name, err);

    err = NetGetHostInfo(&localname);
    BAIL_ON_WIN_ERROR(err);

    status = NetpGetRwDcName(domain, FALSE, &domain_controller_name);
    BAIL_ON_NT_STATUS(status);

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    BAIL_ON_NT_STATUS(status);

    status = LwpsGetPasswordByHostName(hStore, localname, &pi);
    BAIL_ON_NT_STATUS(status);

    /* zero the machine password */
    memset((void*)pi->pwszMachinePassword, 0,
           wc16slen(pi->pwszMachinePassword));
    pi->last_change_time = time(NULL);

    status = LwpsWritePasswordToAllStores(pi);
    BAIL_ON_NT_STATUS(status);

    /* disable the account only if requested */
    if (options & NETSETUP_ACCT_DELETE) {
        if (account && password)
        {
            status = LwIoCreatePlainCredsW(account, domain, password, &creds);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            status = LwIoGetActiveCreds(NULL, &creds);
            BAIL_ON_NT_STATUS(status);
        }

        status = NetConnectSamr(&pConn, domain_controller_name, domain_access, 0, creds);
        BAIL_ON_NT_STATUS(status);

        status = DisableWksAccount(pConn, pi->pwszMachineAccount, &hAccount);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    NetDisconnectSamr(&pConn);

    if (localname)
    {
        NetFreeMemory(localname);
    }

    SAFE_FREE(domain_controller_name);
    SAFE_FREE(machine_name);

    if (pi)
    {
        LwpsFreePasswordInfo(hStore, pi);
    }

    if (hStore != (HANDLE)NULL)
    {
       LwpsClosePasswordStore(hStore);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (creds)
    {
        LwIoDeleteCreds(creds);
    }

    SAFE_FREE(machine_name);

    goto cleanup;
}


NET_API_STATUS NetUnjoinDomain(const wchar16_t *hostname,
                               const wchar16_t *account,
                               const wchar16_t *password,
                               UINT32 options)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *domain = NULL;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pi = NULL;
    char *localname = NULL;

    /* at the moment we support only locally triggered unjoin */
    if (hostname) {
        status = STATUS_NOT_IMPLEMENTED;

    } else {
        wchar16_t host[MAXHOSTNAMELEN];

        err = NetGetHostInfo(&localname);
        BAIL_ON_WIN_ERROR(err);

        mbstowc16s(host, localname, sizeof(wchar16_t)*MAXHOSTNAMELEN);

        status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
        BAIL_ON_NT_STATUS(status);

        status = LwpsGetPasswordByHostName(hStore, localname, &pi);
        BAIL_ON_NT_STATUS(status);

        domain = pi->pwszDnsDomainName;
        err = NetUnjoinDomainLocal(host, domain, account, password, options);
        BAIL_ON_WIN_ERROR(err);
    }

    if (hStore != (HANDLE)NULL) {
       status = LwpsClosePasswordStore(hStore);
       BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (localname)
    {
        NetFreeMemory(localname);
    }    

    if (pi) {
       LwpsFreePasswordInfo(hStore, pi);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

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
