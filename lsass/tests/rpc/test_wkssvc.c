/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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


static
BOOLEAN
CallNetrWkstaGetInfo(
    WKSS_BINDING       hBinding,
    PWSTR              pwszServerName,
    PDWORD             pdwLevels,
    DWORD              dwNumLevels,
    PNETR_WKSTA_INFO  *ppInfo
    )
{
    BOOLEAN bRet = TRUE;
    WINERROR winError = ERROR_SUCCESS;
    DWORD i = 0;
    DWORD dwLevel = 0;
    PNETR_WKSTA_INFO pInfo = NULL;

    winError = LwAllocateMemory(sizeof(*pInfo) * dwNumLevels,
                                OUT_PPVOID(&pInfo));
    BAIL_ON_WIN_ERROR(winError);

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        winError = NetrWkstaGetInfo(hBinding,
                                    pwszServerName,
                                    dwLevel,
                                    &(pInfo[i]));
        BAIL_ON_WIN_ERROR(winError);
    }

    *ppInfo = pInfo;

cleanup:
    return bRet;

error:
    bRet = FALSE;
    goto cleanup;
}


WKSS_BINDING
CreateWkssBinding(
    PWKSS_BINDING     phBinding,
    const wchar16_t  *hostname
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_CREDS creds = NULL;

    if (phBinding == NULL) return NULL;

    if (LwIoGetActiveCreds(NULL, &creds) != STATUS_SUCCESS)
    {
        return NULL;
    }

    ntStatus = WkssInitBindingDefault(phBinding, hostname, creds);
    if (ntStatus != STATUS_SUCCESS)
    {
        *phBinding = NULL;
        goto done;
    }

done:
    if (creds)
    {
        LwIoDeleteCreds(creds);
    }

    return *phBinding;
}


int
TestNetrJoinDomain2(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    struct parameter *options,
    int optcount
    )
{
    PCSTR pszDefAccountOu = "OU=TestingMachines";
    PCSTR pszDefAccountName = "Administrator";

    BOOLEAN bRet = TRUE;
    WINERROR winError = ERROR_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    enum param_err perr = perr_success;
    WKSS_BINDING hBinding = NULL;
    DWORD dwLevels[] = { 100 };
    PNETR_WKSTA_INFO pWkstaInfo = NULL;
    PWSTR pwszServerName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAccountOu = NULL;
    PWSTR pwszAccountName = NULL;
    PWSTR pwszPassword = NULL;
    size_t sAccountOuLen = 0;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    DWORD dwProtSeq = 0;
    DWORD iByte = 0;
    BYTE PasswordBlob[516] = {0};
    BYTE KeyInit[8] = {0};
    MD5_CTX ctx;
    BYTE DigestedSessionKey[16] = {0};
    RC4_KEY key;
    ENC_JOIN_PASSWORD_BUFFER PasswordBuffer;
    DWORD dwJoinFlags = 0;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLen = 0;
    unsigned16 sessionKeyLen = 0;

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));
    memset(&PasswordBuffer, 0, sizeof(PasswordBuffer));

    perr = fetch_value(options, optcount, "domainname", pt_w16string,
                       &pwszDomainName, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "ou", pt_w16string,
                       &pwszAccountOu, &pszDefAccountOu);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "account", pt_w16string,
                       &pwszAccountName, &pszDefAccountName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &pwszPassword, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(t, hostname, user, pass);

    CreateWkssBinding(&hBinding, hostname);
    if (hBinding == NULL)
    {
        bRet = FALSE;
        goto cleanup;
    }

    if (hostname)
    {
        winError = LwAllocateWc16String(&pwszServerName, hostname);
        BAIL_ON_WIN_ERROR(winError);
    }

    winError = LwWc16sLen(pwszAccountOu, &sAccountOuLen);
    BAIL_ON_WIN_ERROR(winError);

    if (sAccountOuLen == 0)
    {
        LW_SAFE_FREE_MEMORY(pwszAccountOu);
        pwszAccountOu = NULL;
    }

    /* This is only to create a connection and get a valid session key */
    bRet &= CallNetrWkstaGetInfo(hBinding,
                                 pwszServerName,
                                 dwLevels,
                                 (sizeof(dwLevels)/sizeof(dwLevels[0])),
                                 &pWkstaInfo);

    rpc_binding_inq_transport_info(hBinding,
                                   &hTransportInfo,
                                   &rpcStatus);

    if (hTransportInfo)
    {
        rpc_binding_inq_prot_seq(hBinding,
                                 (unsigned32*)&dwProtSeq,
                                 &rpcStatus);

        switch (dwProtSeq)
        {
        case rpc_c_protseq_id_ncacn_np:
            rpc_smb_transport_info_inq_session_key(
                                       hTransportInfo,
                                       (unsigned char**)&pSessionKey,
                                       &sessionKeyLen);
            break;

        case rpc_c_protseq_id_ncalrpc:
            rpc_lrpc_transport_info_inq_session_key(
                                       hTransportInfo,
                                       (unsigned char**)&pSessionKey,
                                       &sessionKeyLen);
            break;
        }
        dwSessionKeyLen = (DWORD)sessionKeyLen;
    }
    else
    {
        bRet = FALSE;
        goto cleanup;
    }

    
    winError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_WIN_ERROR(winError);

    sPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    winError = LwAllocateMemory(sPasswordSize + sizeof(pwszPassword[0]),
                                OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_WIN_ERROR(winError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    iByte = sizeof(PasswordBlob);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 24) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 16) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 8) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize) & 0xff);

    iByte -= sPasswordSize;
    memcpy(&(PasswordBlob[iByte]),
           pwszPasswordLE,
           sPasswordSize);

    if (!RAND_bytes(KeyInit, sizeof(KeyInit)))
    {
        bRet = FALSE;
        goto cleanup;
    }

    MD5_Init(&ctx);
    MD5_Update(&ctx, pSessionKey, dwSessionKeyLen);
    MD5_Update(&ctx, KeyInit, sizeof(KeyInit));
    MD5_Final(DigestedSessionKey, &ctx);

    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);

    RC4(&key,
        sizeof(PasswordBlob),
        (const unsigned char*)PasswordBlob,
        (unsigned char*)PasswordBlob);

    iByte = 0;
    memcpy(&PasswordBuffer.data[iByte], KeyInit, sizeof(KeyInit));
    iByte += sizeof(KeyInit);
    memcpy(&PasswordBuffer.data[iByte], PasswordBlob, sizeof(PasswordBlob));

    winError = NetrJoinDomain2(hBinding,
                               pwszServerName,
                               pwszDomainName,
                               pwszAccountOu,
                               pwszAccountName,
                               &PasswordBuffer,
                               dwJoinFlags);
    if (winError)
    {
        bRet = FALSE;
    }

cleanup:
error:
    WkssFreeBinding(&hBinding);

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszAccountOu);
    LW_SAFE_FREE_MEMORY(pwszAccountName);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pwszPasswordLE);

    switch (dwProtSeq)
    {
    case rpc_c_protseq_id_ncacn_np:
        rpc_smb_transport_info_free(hTransportInfo);
        break;

    case rpc_c_protseq_id_ncalrpc:
        rpc_lrpc_transport_info_free(hTransportInfo);
        break;
    }

    if (winError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


int
TestNetrUnjoinDomain2(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    struct parameter *options,
    int optcount
    )
{
    PCSTR pszDefAccountName = "Administrator";

    BOOLEAN bRet = TRUE;
    WINERROR winError = ERROR_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    enum param_err perr = perr_success;
    WKSS_BINDING hBinding = NULL;
    DWORD dwLevels[] = { 100 };
    PNETR_WKSTA_INFO pWkstaInfo = NULL;
    PWSTR pwszServerName = NULL;
    PWSTR pwszAccountName = NULL;
    PWSTR pwszPassword = NULL;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    DWORD dwProtSeq = 0;
    DWORD iByte = 0;
    BYTE PasswordBlob[516] = {0};
    BYTE KeyInit[8] = {0};
    MD5_CTX ctx;
    BYTE DigestedSessionKey[16] = {0};
    RC4_KEY key;
    ENC_JOIN_PASSWORD_BUFFER PasswordBuffer;
    DWORD dwUnjoinFlags = 0;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLen = 0;

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));
    memset(&PasswordBuffer, 0, sizeof(PasswordBuffer));

    perr = fetch_value(options, optcount, "account", pt_w16string,
                       &pwszAccountName, &pszDefAccountName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &pwszPassword, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(t, hostname, user, pass);

    CreateWkssBinding(&hBinding, hostname);
    if (hBinding == NULL)
    {
        bRet = FALSE;
        goto cleanup;
    }

    if (hostname)
    {
        winError = LwAllocateWc16String(&pwszServerName, hostname);
        BAIL_ON_WIN_ERROR(winError);
    }

    /* This is only to create a connection and get a valid session key */
    bRet &= CallNetrWkstaGetInfo(hBinding,
                                 pwszServerName,
                                 dwLevels,
                                 (sizeof(dwLevels)/sizeof(dwLevels[0])),
                                 &pWkstaInfo);

    rpc_binding_inq_transport_info(hBinding,
                                   &hTransportInfo,
                                   &rpcStatus);

    if (hTransportInfo)
    {
        rpc_binding_inq_prot_seq(hBinding,
                                 (unsigned32*)&dwProtSeq,
                                 &rpcStatus);

        switch (dwProtSeq)
        {
        case rpc_c_protseq_id_ncacn_np:
            rpc_smb_transport_info_inq_session_key(
                                       hTransportInfo,
                                       (unsigned char**)&pSessionKey,
                                       (unsigned16*)&dwSessionKeyLen);
            break;

        case rpc_c_protseq_id_ncalrpc:
            rpc_lrpc_transport_info_inq_session_key(
                                       hTransportInfo,
                                       (unsigned char**)&pSessionKey,
                                       (unsigned16*)&dwSessionKeyLen);
            break;
        }
    }
    else
    {
        bRet = FALSE;
        goto cleanup;
    }

    
    winError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_WIN_ERROR(winError);

    sPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    winError = LwAllocateMemory(sPasswordSize + sizeof(pwszPassword[0]),
                                OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_WIN_ERROR(winError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    iByte = sizeof(PasswordBlob);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 24) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 16) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize >> 8) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((sPasswordSize) & 0xff);

    iByte -= sPasswordSize;
    memcpy(&(PasswordBlob[iByte]),
           pwszPasswordLE,
           sPasswordSize);

    if (!RAND_bytes(KeyInit, sizeof(KeyInit)))
    {
        bRet = FALSE;
        goto cleanup;
    }

    MD5_Init(&ctx);
    MD5_Update(&ctx, pSessionKey, dwSessionKeyLen);
    MD5_Update(&ctx, KeyInit, sizeof(KeyInit));
    MD5_Final(DigestedSessionKey, &ctx);

    RC4_set_key(&key,
                sizeof(DigestedSessionKey),
                (unsigned char*)DigestedSessionKey);

    RC4(&key,
        sizeof(PasswordBlob),
        (const unsigned char*)PasswordBlob,
        (unsigned char*)PasswordBlob);

    iByte = 0;
    memcpy(&PasswordBuffer.data[iByte], KeyInit, sizeof(KeyInit));
    iByte += sizeof(KeyInit);
    memcpy(&PasswordBuffer.data[iByte], PasswordBlob, sizeof(PasswordBlob));

    winError = NetrUnjoinDomain2(hBinding,
                                 pwszServerName,
                                 pwszAccountName,
                                 &PasswordBuffer,
                                 dwUnjoinFlags);
    if (winError)
    {
        bRet = FALSE;
    }

cleanup:
error:
    WkssFreeBinding(&hBinding);

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszAccountName);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pwszPasswordLE);

    switch (dwProtSeq)
    {
    case rpc_c_protseq_id_ncacn_np:
        rpc_smb_transport_info_free(hTransportInfo);
        break;

    case rpc_c_protseq_id_ncalrpc:
        rpc_lrpc_transport_info_free(hTransportInfo);
        break;
    }

    if (winError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


void SetupWkssvcTests(struct test *t)
{
    AddTest(t, "NETR-JOIN-DOMAIN2", TestNetrJoinDomain2);
    AddTest(t, "NETR-UNJOIN-DOMAIN2", TestNetrUnjoinDomain2);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
