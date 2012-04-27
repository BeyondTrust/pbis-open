/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
DWORD
DNSBuildTKeyQueryRequest(
    PCSTR pszKeyName,
    PBYTE pKeyData,
    DWORD dwKeyLen,
    PDNS_REQUEST * ppDNSRequest
    );

static
DWORD
DNSVerifyResponseMessage_GSSSuccess(
    PCtxtHandle    pGSSContext,
    PDNS_RR_RECORD pClientTKeyRecord,
    PDNS_RESPONSE  pDNSResponse
    );

static
DWORD
DNSVerifyResponseMessage_GSSContinue(
    PCtxtHandle    pGSSContext,
    PDNS_RR_RECORD pClientTKeyRecord,
    PDNS_RESPONSE  pDNSResponse,
    PBYTE *        ppServerKeyData,
    PWORD          pwServerKeyDataSize
    );

static
DWORD
DNSResponseGetRCode(
    PDNS_RESPONSE pDNSResponse,
    WORD * pwRCode
    );

static
DWORD
DNSResponseGetTKeyRecord(
    PDNS_RESPONSE pDNSResponse,
    PDNS_RR_RECORD * ppTKeyRecord
    );

static
DWORD
DNSResponseGetTSIGRecord(
    PDNS_RESPONSE pDNSResponse,
    PDNS_RR_RECORD * ppTSIGRecord
    );

static
DWORD
DNSCompareTKeyRecord(
    PDNS_RR_RECORD pClientTKeyRecord,
    PDNS_RR_RECORD pTKeyRecord
    );

static
DWORD
DNSGetTKeyData(
    PDNS_RR_RECORD pTKeyRecord,
    PBYTE * ppKeyData,
    PWORD pwKeyDataSize
    );

static
VOID
DNSRecordGenerateOffsets(
    PDNS_RR_RECORD pDNSRecord
    );

static
DWORD
DNSGetDomainNameOffset(
    PBYTE pBuffer
    );

DWORD
DNSNegotiateSecureContext(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszKeyName,
    PCtxtHandle pGSSContext
    )
{
    DWORD dwError = 0;
    DWORD dwMajorStatus = 0;
    char szTargetName[256];

    gss_buffer_desc input_name = {0};
    gss_buffer_desc input_desc = {0};
    gss_buffer_desc* input_ptr = NULL;
    gss_buffer_desc output_desc = {0};
    PDNS_REQUEST pDNSRequest = NULL;
    PDNS_RESPONSE pDNSResponse = NULL;
    PDNS_RR_RECORD pClientTKeyRecord = NULL;
    //PDNS_RR_RECORD pTKeyRecord = NULL;
    HANDLE hDNSTcpServer = (HANDLE)NULL;

    PBYTE pServerKeyData = NULL;
    WORD wServerKeyDataSize = 0;

    //PCtxtHandle pContextHandle = NULL;
    OM_uint32 ret_flags = 0;

    DWORD dwMinorStatus = 0;
    gss_name_t targ_name = {0};

    gss_OID_desc nt_host_oid_desc =
        {10,(void*)"\052\206\110\206\367\022\001\002\002\002"};
    gss_OID_desc krb5_oid_desc =
        {9,(void*)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x02" };

    krb5_error_code ret = 0;
    krb5_principal host_principal = NULL;
    krb5_context ctx = NULL;

    input_desc.length = 0;
    input_desc.value = NULL;

    dwError = DNSOpen(
                    pszServerName,
                    DNS_TCP,
                    &hDNSTcpServer
                    );
    BAIL_ON_LWDNS_ERROR(dwError);

    sprintf(szTargetName,"dns/%s@",pszServerName);

    ret = krb5_init_context(&ctx);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, szTargetName, &host_principal);
    BAIL_ON_LWDNS_KRB_ERROR(ctx, ret);

    input_name.value = &host_principal;
    input_name.length = sizeof(host_principal);

    dwMajorStatus = gss_import_name(
                        (OM_uint32 *)&dwMinorStatus,
                        &input_name,
                        &nt_host_oid_desc,
                        &targ_name);
    BAIL_ON_SEC_ERROR(dwMajorStatus);

    memset(pGSSContext, 0, sizeof(CtxtHandle));
    *pGSSContext = GSS_C_NO_CONTEXT;

    do  {

        dwMajorStatus = gss_init_sec_context(
                            (OM_uint32 *)&dwMinorStatus,
                            NULL,
                            pGSSContext,
                            targ_name,
                            &krb5_oid_desc,
                            GSS_C_REPLAY_FLAG | GSS_C_MUTUAL_FLAG |
                            GSS_C_SEQUENCE_FLAG | GSS_C_CONF_FLAG |
                            GSS_C_INTEG_FLAG | GSS_C_DELEG_FLAG,
                            0,
                            NULL,
                            input_ptr,
                            NULL,
                            &output_desc,
                            &ret_flags,
                            NULL);
        
        lwdns_display_status(
                "gss_init_context",
                dwMajorStatus,
                dwMinorStatus);
        
        BAIL_ON_SEC_ERROR(dwMajorStatus);

        switch (dwMajorStatus) {

            case GSS_S_COMPLETE:
                
                    if (output_desc.length != 0) {

                        dwError = DNSBuildTKeyQueryRequest(
                                            pszKeyName,
                                            output_desc.value,
                                            output_desc.length,
                                            &pDNSRequest);
                        BAIL_ON_LWDNS_ERROR(dwError);

                        dwError = DNSStdSendStdRequest2(
                                    hDNSTcpServer, 
                                    pDNSRequest);
                        BAIL_ON_LWDNS_ERROR(dwError);

                        dwError = DNSStdReceiveStdResponse(
                                    hDNSTcpServer,
                                    &pDNSResponse);
                          BAIL_ON_LWDNS_ERROR(dwError);

                        dwError = DNSVerifyResponseMessage_GSSSuccess(
                                        pGSSContext,
                                        pClientTKeyRecord,
                                        pDNSResponse);
                        BAIL_ON_LWDNS_ERROR(dwError);
                    }
                    
                    break;


            case GSS_S_CONTINUE_NEEDED:
                
                    if (output_desc.length != 0) {
                        
                        if (pDNSRequest)
                        {
                            DNSStdFreeRequest(pDNSRequest);
                            pDNSRequest = NULL;
                        }

                        dwError = DNSBuildTKeyQueryRequest(
                                        pszKeyName,
                                        output_desc.value,
                                        output_desc.length,
                                        &pDNSRequest);
                        BAIL_ON_LWDNS_ERROR(dwError);
                        
                        gss_release_buffer(
                            (OM_uint32 *)&dwMinorStatus,
                            &output_desc);
                        output_desc.length = 0;

                        dwError = DNSStdSendStdRequest2(
                                    hDNSTcpServer, 
                                    pDNSRequest);
                        BAIL_ON_LWDNS_ERROR(dwError);
                        
                        if (pDNSResponse)
                        {
                            DNSStdFreeResponse(pDNSResponse);
                            pDNSResponse = NULL;
                        }

                        dwError = DNSStdReceiveStdResponse(
                                    hDNSTcpServer, 
                                    &pDNSResponse);
                        BAIL_ON_LWDNS_ERROR(dwError);
                          
                        if (pServerKeyData)
                        {
                            DNSFreeMemory(pServerKeyData);
                            pServerKeyData = NULL;
                        }

                        dwError = DNSVerifyResponseMessage_GSSContinue(
                                        pGSSContext,
                                        pClientTKeyRecord,
                                        pDNSResponse,
                                        &pServerKeyData,
                                        &wServerKeyDataSize);
                        BAIL_ON_LWDNS_ERROR(dwError);

                        input_desc.value = pServerKeyData;
                        input_desc.length = wServerKeyDataSize;

                        input_ptr = &input_desc;
                    }
                    
                break;

            default:
                
                BAIL_ON_LWDNS_ERROR(dwError);
        }

    } while (dwMajorStatus == GSS_S_CONTINUE_NEEDED);

    //
    // If we arrive here, we have a valid security context
    //
    
cleanup:

    if (pDNSRequest)
    {
        DNSStdFreeRequest(pDNSRequest);
        pDNSRequest = NULL;
    }

    if (pDNSResponse)
    {
        DNSStdFreeResponse(pDNSResponse);
        pDNSResponse = NULL;
    }

    if (pServerKeyData)
    {
        DNSFreeMemory(pServerKeyData);
    }
    
    if (output_desc.length)
    {
        gss_release_buffer(
            (OM_uint32 *)&dwMinorStatus,
            &output_desc);
    }
    
    gss_release_name(&dwMinorStatus, &targ_name);
    
    if (ctx) {

        if (host_principal) {
            krb5_free_principal(ctx, host_principal);
        }

        krb5_free_context(ctx);
    }
    
    if (hDNSTcpServer != (HANDLE)NULL)
    {
        DNSClose(hDNSTcpServer);
    }

    return dwError;

sec_error:
error:

    goto cleanup;
}

static
DWORD
DNSBuildTKeyQueryRequest(
    PCSTR pszKeyName,
    PBYTE pKeyData,
    DWORD dwKeyLen,
    PDNS_REQUEST * ppDNSRequest
    )
{
    DWORD dwError = 0;
    PDNS_REQUEST pDNSRequest = NULL;
    PDNS_RR_RECORD pDNSTKeyRecord = NULL;
    PDNS_QUESTION_RECORD pDNSQuestionRecord = NULL;

    dwError = DNSStdCreateStdRequest(
                    &pDNSRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCreateQuestionRecord(
                    pszKeyName,
                    QTYPE_TKEY,
                    DNS_CLASS_IN,
                    &pDNSQuestionRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSStdAddQuestionSection(
                    pDNSRequest,
                    pDNSQuestionRecord);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDNSQuestionRecord = NULL;

    dwError = DNSCreateTKeyRecord(
                    pszKeyName,
                    pKeyData,
                    (WORD)dwKeyLen,
                    &pDNSTKeyRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSStdAddAdditionalSection(
                        pDNSRequest,
                        pDNSTKeyRecord);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDNSTKeyRecord = NULL;

    *ppDNSRequest = pDNSRequest;

cleanup:
    
    return dwError;

error:

    *ppDNSRequest = NULL;
    
    if (pDNSRequest)
    {
        DNSStdFreeRequest(pDNSRequest);
    }
    
    if (pDNSTKeyRecord)
    {
        DNSFreeRecord(pDNSTKeyRecord);
    }
    
    if (pDNSQuestionRecord)
    {
        DNSFreeQuestionRecord(pDNSQuestionRecord);
    }

    goto cleanup;
}

static
DWORD
DNSVerifyResponseMessage_GSSSuccess(
    PCtxtHandle    pGSSContext,
    PDNS_RR_RECORD pClientTKeyRecord,
    PDNS_RESPONSE  pDNSResponse
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pTKeyRecord = NULL;
    PDNS_RR_RECORD pTSIGRecord = NULL;
    WORD wRCode = 0;

    dwError = DNSResponseGetRCode(
                        pDNSResponse,
                        &wRCode);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (wRCode != 0) {
        dwError = ERROR_BAD_NET_RESP;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSResponseGetTKeyRecord(
                    pDNSResponse,
                    &pTKeyRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCompareTKeyRecord(
                    pClientTKeyRecord,
                    pTKeyRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSResponseGetTSIGRecord(
                    pDNSResponse,
                    &pTSIGRecord);
    BAIL_ON_LWDNS_ERROR(dwError);
    
/*
    dwMajorStatus = GSS_VerifyMIC(
                        pDNSResponse->pDNSResponseBuffer,
                        pDNSResponse->dwNumBytes,
                        pDNSRRRecord->RData.TSIGRData.pMAC,
                        pDNSRRRecord->RData.TSIGRData.wMaxSize
                        )
    BAIL_ON_LWDNS_ERROR(dwMajorStatus);
*/
    
cleanup:

    if (pTKeyRecord)
    {
        DNSFreeRecord(pTKeyRecord);
    }
    
    if (pTSIGRecord)
    {
        DNSFreeRecord(pTSIGRecord);
    }
    
    return dwError;
    
error:

    goto cleanup;
}

static
DWORD
DNSVerifyResponseMessage_GSSContinue(
    PCtxtHandle    pGSSContext,
    PDNS_RR_RECORD pClientTKeyRecord,
    PDNS_RESPONSE  pDNSResponse,
    PBYTE *        ppServerKeyData,
    PWORD          pwServerKeyDataSize
    )
{
    DWORD dwError = 0;
    WORD  wRCode = 0;
    PBYTE pServerKeyData = NULL;
    WORD  wServerKeyDataSize = 0;
    // Do not Free
    PDNS_RR_RECORD pTKeyRecord = NULL;

    dwError = DNSResponseGetRCode(
                        pDNSResponse,
                        &wRCode);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    if (wRCode != 0)
    {
        dwError = ERROR_BAD_NET_RESP;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSResponseGetTKeyRecord(
                        pDNSResponse,
                        &pTKeyRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCompareTKeyRecord(
                        pClientTKeyRecord,
                        pTKeyRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSGetTKeyData(
                        pTKeyRecord,
                        &pServerKeyData,
                        &wServerKeyDataSize);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppServerKeyData = pServerKeyData;
    *pwServerKeyDataSize = wServerKeyDataSize;
    
cleanup:

    return(dwError);

error:

    *ppServerKeyData = NULL;
    *pwServerKeyDataSize = 0;
    
    if (pServerKeyData)
    {
        DNSFreeMemory(pServerKeyData);
    }
    
    goto cleanup;
}

static
DWORD
DNSResponseGetRCode(
    PDNS_RESPONSE pDNSResponse,
    WORD *pwRCode
    )
{
    DWORD dwError = 0;
    WORD wnParameter = 0;
    BYTE uChar = 0;

    wnParameter = htons(pDNSResponse->wParameter);

    //
    // Byte 0 is the most significate byte
    // Bit 12, 13, 14, 15 or Bit 4, 5, 6, 7 represent the RCode
    //
    memcpy(&uChar, (PBYTE)&wnParameter+1, 1);
    uChar >>=4;
    *pwRCode = (WORD)uChar;

    return dwError;
}

static
DWORD
DNSResponseGetTKeyRecord(
    PDNS_RESPONSE pDNSResponse,
    PDNS_RR_RECORD * ppTKeyRecord
    )
{
    DWORD dwError = 0;
    WORD wAnswers = 0;
    PDNS_RR_RECORD pDNSRecord = NULL;
    DWORD i = 0;

    wAnswers = pDNSResponse->wAnswers ;
    if (!wAnswers) {
        dwError = EINVAL;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    for (i = 0; i < wAnswers; i++)
    {
        pDNSRecord = *(pDNSResponse->ppAnswerRRSet + i);
        
        if (pDNSRecord->RRHeader.wType == QTYPE_TKEY)
        {
            *ppTKeyRecord = pDNSRecord;
            goto cleanup;
        }
    }
    
    dwError = DNS_ERROR_RECORD_DOES_NOT_EXIST;
    BAIL_ON_LWDNS_ERROR(dwError);
    
cleanup:

    return dwError;

error:

    *ppTKeyRecord = NULL;
    
    goto cleanup;
}

static
DWORD
DNSResponseGetTSIGRecord(
    PDNS_RESPONSE pDNSResponse,
    PDNS_RR_RECORD * ppTSIGRecord
    )
{
    DWORD dwError = 0;
    WORD wAdditionals = 0;
    PDNS_RR_RECORD pDNSRecord = NULL;
    DWORD i = 0;

    wAdditionals = pDNSResponse->wAdditionals ;
    if (!wAdditionals) {
        dwError = EINVAL;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    for (i = 0; i < wAdditionals; i++)
    {
        pDNSRecord = *(pDNSResponse->ppAdditionalRRSet + i);
        
        if (pDNSRecord->RRHeader.wType == QTYPE_TSIG) {
            *ppTSIGRecord = pDNSRecord;
            goto cleanup;
        }
    }
    
    dwError = DNS_ERROR_RECORD_DOES_NOT_EXIST;
    BAIL_ON_LWDNS_ERROR(dwError);
    
cleanup:

    return dwError;

error:

    *ppTSIGRecord = NULL;
    
    goto cleanup;
}

void
lwdns_display_status(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    )
{
     lwdns_display_status_1(pszId, maj_stat, GSS_C_GSS_CODE);
     lwdns_display_status_1(pszId, min_stat, GSS_C_MECH_CODE);
}

void
lwdns_display_status_1(
    PCSTR     pszId,
    OM_uint32 code,
    int       type
    )
{
    OM_uint32 /*maj_stat ATTRIBUTE_UNUSED,*/ min_stat;
    gss_buffer_desc msg;
    OM_uint32 msg_ctx;

    if ( code == 0 )
    {
        return;
    }

    msg_ctx = 0;
    while (1)
    {
        /*maj_stat = */gss_display_status(&min_stat, code,
                                      type, GSS_C_NULL_OID,
                                      &msg_ctx, &msg);

        switch(code)
        {
#ifdef WIN32
            case SEC_E_OK:
            case SEC_I_CONTINUE_NEEDED:
#else
            case GSS_S_COMPLETE:
            case GSS_S_CONTINUE_NEEDED:
#endif
                LWDNS_LOG_VERBOSE("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
                break;

            default:
                
                LWDNS_LOG_ERROR("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
        }

        (void) gss_release_buffer(&min_stat, &msg);

        if (!msg_ctx)
            break;
    }
}

static
DWORD
DNSCompareTKeyRecord(
    PDNS_RR_RECORD pClientTKeyRecord,
    PDNS_RR_RECORD pTKeyRecord
    )
{
    DWORD dwError = 0;
    
    // TODO

    return(dwError);
}

static
DWORD
DNSGetTKeyData(
    PDNS_RR_RECORD pTKeyRecord,
    PBYTE * ppKeyData,
    PWORD pwKeyDataSize
    )
{
    DWORD dwError = 0;
    WORD wKeyDataSize = 0;
    WORD wnKeyDataSize = 0;
    DWORD dwKeyDataSizeOffset = 0;
    DWORD dwKeyDataOffset = 0;
    PBYTE pKeyData = NULL;

    DNSRecordGenerateOffsets(pTKeyRecord);
    
    dwKeyDataSizeOffset = pTKeyRecord->Offsets.TKey.wKeySizeOffset;
    dwKeyDataOffset = pTKeyRecord->Offsets.TKey.wKeyDataOffset;
    memcpy(&wnKeyDataSize,
           pTKeyRecord->pRData + dwKeyDataSizeOffset,
           sizeof(WORD));
    wKeyDataSize = ntohs(wnKeyDataSize);

    dwError = DNSAllocateMemory(wKeyDataSize, (PVOID *)&pKeyData);
    BAIL_ON_LWDNS_ERROR(dwError);

    memcpy(pKeyData, pTKeyRecord->pRData + dwKeyDataOffset, wKeyDataSize);

    *ppKeyData = pKeyData;
    *pwKeyDataSize = wKeyDataSize;

    return(dwError);

error:

    *ppKeyData  = NULL;
    *pwKeyDataSize = 0;
    
    return(dwError);
}

static
VOID
DNSRecordGenerateOffsets(
    PDNS_RR_RECORD pDNSRecord
    )
{
    DWORD dwOffset = 0;
    PBYTE pRData = NULL;
    WORD  wKeySize, wnKeySize = 0;

    pRData = pDNSRecord->pRData;
    
    switch(pDNSRecord->RRHeader.wType) 
    {
        case QTYPE_TKEY:
            
                pDNSRecord->Offsets.TKey.wAlgorithmOffset = (WORD)dwOffset;
                dwOffset += DNSGetDomainNameOffset(pRData);
                
                pDNSRecord->Offsets.TKey.wInceptionOffset = (WORD)dwOffset;
                dwOffset += sizeof(DWORD);
                
                pDNSRecord->Offsets.TKey.wExpirationOffset = (WORD)dwOffset;
                dwOffset += sizeof(DWORD);
                
                pDNSRecord->Offsets.TKey.wModeOffset = (WORD)dwOffset;
                dwOffset += sizeof(WORD);
                
                pDNSRecord->Offsets.TKey.wErrorOffset = (WORD)dwOffset;
                dwOffset += sizeof(WORD);
                
                pDNSRecord->Offsets.TKey.wKeySizeOffset = (WORD)dwOffset;
                dwOffset += sizeof(WORD);
                
                pDNSRecord->Offsets.TKey.wKeyDataOffset = (WORD)dwOffset;

                memcpy(&wnKeySize,
                       pRData + pDNSRecord->Offsets.TKey.wKeySizeOffset,
                       sizeof(WORD));
                
                wKeySize = ntohs(wnKeySize);
                dwOffset += wKeySize;
                
                pDNSRecord->Offsets.TKey.wOtherSizeOffset = (WORD)dwOffset;
                dwOffset += sizeof(WORD);
                
                pDNSRecord->Offsets.TKey.wOtherDataOffset = (WORD)dwOffset;
                
                break;

        case QTYPE_TSIG:
                break;
    }
    
    return;
}

static
DWORD
DNSGetDomainNameOffset(
    PBYTE pBuffer
    )
{
    BYTE uLen1 = 0;
    DWORD dwOffset = 0;

    uLen1 = *pBuffer;
    if (uLen1 & 0xC0)
    {
        dwOffset += 2;
    }
    else
    {
        BYTE uLen = 0;
        
        while(1)
        {
            uLen = *pBuffer;
            pBuffer++;
            dwOffset++;
            if (uLen == 0)
            {
                break;
            }
            dwOffset += uLen;
            pBuffer += uLen;
        }
    }
    
    return(dwOffset);
}

