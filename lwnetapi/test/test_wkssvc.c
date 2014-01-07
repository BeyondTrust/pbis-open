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
    );

static
DWORD
TestNetrWkstaGetInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestNetrJoinDomain2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestNetrUnjoinDomain2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );


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


static
DWORD
TestNetrWkstaGetInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefLevel = -1;

    BOOLEAN bRet = TRUE;
    WINERROR winError = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    WKSS_BINDING hWkss = NULL;
    PWSTR pwszServerName = NULL;
    DWORD dwSelectedLevels[] = {0};
    DWORD dwAvailableLevels[] = {100};
    DWORD dwNumLevels = 0;
    PDWORD pdwLevels = NULL;
    DWORD iLevel = 0;
    DWORD dwLevel = 0;
    PNETR_WKSTA_INFO pWkstaInfo = NULL;

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hWkss),
                             RPC_WKSSVC_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    if (pwszHostname)
    {
        winError = LwAllocateWc16String(&pwszServerName, pwszHostname);
        BAIL_ON_WIN_ERROR(winError);
    }

    if (dwLevel == (DWORD)(-1))
    {
        pdwLevels   = dwAvailableLevels;
        dwNumLevels = sizeof(dwAvailableLevels)/sizeof(dwAvailableLevels[0]);
    }
    else
    {
        dwSelectedLevels[0] = dwLevel;
        pdwLevels   = dwSelectedLevels;
        dwNumLevels = sizeof(dwSelectedLevels)/sizeof(dwSelectedLevels[0]);
    }

    for (iLevel = 0; iLevel < dwNumLevels; iLevel++)
    {
        dwLevel = pdwLevels[iLevel];

        bRet &= CallNetrWkstaGetInfo(hWkss,
                                     pwszServerName,
                                     pdwLevels,
                                     dwNumLevels,
                                     &pWkstaInfo);
    }

    WkssFreeBinding(&hWkss);

error:
    for (iLevel = 0; iLevel < dwNumLevels; iLevel++)
    {
        if (pWkstaInfo[iLevel].pInfo100)
        {
            WkssFreeMemory(pWkstaInfo[iLevel].pInfo100);
        }
    }

    if (winError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
DWORD
TestNetrJoinDomain2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefAccountOu = "";
    PCSTR pszDefAccountName = "Administrator";

    BOOLEAN bRet = TRUE;
    WINERROR winError = ERROR_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    enum param_err perr = perr_success;
    WKSS_BINDING hWkss = NULL;
    DWORD dwLevels[] = {100};
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

    perr = fetch_value(pOptions, dwOptcount, "domainname", pt_w16string,
                       &pwszDomainName, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "ou", pt_w16string,
                       &pwszAccountOu, &pszDefAccountOu);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "account", pt_w16string,
                       &pwszAccountName, &pszDefAccountName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "password", pt_w16string,
                       &pwszPassword, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hWkss),
                             RPC_WKSSVC_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    if (pwszHostname)
    {
        winError = LwAllocateWc16String(&pwszServerName, pwszHostname);
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
    bRet &= CallNetrWkstaGetInfo(hWkss,
                                 pwszServerName,
                                 dwLevels,
                                 (sizeof(dwLevels)/sizeof(dwLevels[0])),
                                 &pWkstaInfo);

    rpc_binding_inq_transport_info(hWkss,
                                   &hTransportInfo,
                                   &rpcStatus);

    if (hTransportInfo)
    {
        rpc_binding_inq_prot_seq(hWkss,
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
        goto error;
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
        goto error;
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

    winError = NetrJoinDomain2(hWkss,
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

    WkssFreeBinding(&hWkss);

error:

    if (pWkstaInfo[0].pInfo100)
    {
        WkssFreeMemory(pWkstaInfo[0].pInfo100);
    }

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszAccountOu);
    LW_SAFE_FREE_MEMORY(pwszAccountName);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pwszPasswordLE);

    if (winError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
DWORD
TestNetrUnjoinDomain2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefAccountName = "Administrator";

    BOOLEAN bRet = TRUE;
    WINERROR winError = ERROR_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    enum param_err perr = perr_success;
    WKSS_BINDING hWkss = NULL;
    DWORD dwLevels[] = {100};
    PNETR_WKSTA_INFO pWkstaInfo = NULL;
    PWSTR pwszServerName = NULL;
    PWSTR pwszAccountName = NULL;
    PWSTR pwszPassword = NULL;
    size_t sPasswordLen = 0;
    size_t sPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    unsigned32 ProtSeq = 0;
    DWORD iByte = 0;
    BYTE PasswordBlob[516] = {0};
    BYTE KeyInit[8] = {0};
    MD5_CTX ctx;
    BYTE DigestedSessionKey[16] = {0};
    RC4_KEY key;
    ENC_JOIN_PASSWORD_BUFFER PasswordBuffer;
    DWORD dwUnjoinFlags = 0;
    unsigned char *SessionKey = NULL;
    unsigned16 SessionKeyLen = 0;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLen = 0;

    memset(&ctx, 0, sizeof(ctx));
    memset(&key, 0, sizeof(key));
    memset(&PasswordBuffer, 0, sizeof(PasswordBuffer));

    perr = fetch_value(pOptions, dwOptcount, "account", pt_w16string,
                       &pwszAccountName, &pszDefAccountName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "password", pt_w16string,
                       &pwszPassword, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hWkss),
                             RPC_WKSSVC_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    if (pwszHostname)
    {
        winError = LwAllocateWc16String(&pwszServerName, pwszHostname);
        BAIL_ON_WIN_ERROR(winError);
    }

    /* This is only to create a connection and get a valid session key */
    bRet &= CallNetrWkstaGetInfo(hWkss,
                                 pwszServerName,
                                 dwLevels,
                                 (sizeof(dwLevels)/sizeof(dwLevels[0])),
                                 &pWkstaInfo);

    rpc_binding_inq_transport_info(hWkss,
                                   &hTransportInfo,
                                   &rpcStatus);

    if (hTransportInfo)
    {
        rpc_binding_inq_prot_seq(hWkss,
                                 &ProtSeq,
                                 &rpcStatus);

        switch (ProtSeq)
        {
        case rpc_c_protseq_id_ncacn_np:
            rpc_smb_transport_info_inq_session_key(
                                       hTransportInfo,
                                       &SessionKey,
                                       &SessionKeyLen);
            break;

        case rpc_c_protseq_id_ncalrpc:
            rpc_lrpc_transport_info_inq_session_key(
                                       hTransportInfo,
                                       &SessionKey,
                                       &SessionKeyLen);
            break;
        }

        dwSessionKeyLen = SessionKeyLen;
        pSessionKey     = SessionKey;
    }
    else
    {
        bRet = FALSE;
        goto error;
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
        goto error;
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

    winError = NetrUnjoinDomain2(hWkss,
                                 pwszServerName,
                                 pwszAccountName,
                                 &PasswordBuffer,
                                 dwUnjoinFlags);
    if (winError)
    {
        bRet = FALSE;
    }

    WkssFreeBinding(&hWkss);

error:
    if (pWkstaInfo[0].pInfo100)
    {
        WkssFreeMemory(pWkstaInfo[0].pInfo100);
    }

    LW_SAFE_FREE_MEMORY(pwszServerName);
    LW_SAFE_FREE_MEMORY(pwszAccountName);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pwszPasswordLE);

    if (winError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


VOID
SetupWkssvcTests(PTEST t)
{
    AddTest(t, "NETR-WKSTA-GET-INFO", TestNetrWkstaGetInfo);
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
