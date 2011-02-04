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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dnsrequest.c
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
DNSStdMarshallQuestionSection(
    HANDLE hSendBuffer,
    PDNS_QUESTION_RECORD * ppDNSQuestionRecords,
    WORD wQuestions
    );

static
DWORD
DNSStdMarshallAnswerSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAnswerRRRecords,
    WORD wAnswers
    );

static
DWORD
DNSStdMarshallAuthoritySection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAuthorityRRRecords,
    WORD wAuthoritys
    );

static
DWORD
DNSStdMarshallAdditionalSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAdditionalsRRRecords,
    WORD wAdditionals
    );

DWORD
DNSMakeQuestion(
    HANDLE         hDNSServer,
    PCSTR          pszQuestionName,
    WORD           wQClass,
    WORD           wQType,
    PDNS_RESPONSE* ppDNSResponse
    )
{
    DWORD dwError = 0;
    PDNS_REQUEST pDNSRequest = NULL;
    PDNS_QUESTION_RECORD pDNSQuestionRecord = NULL;
    PDNS_RESPONSE pDNSResponse = NULL;

    dwError = DNSStdCreateStdRequest(&pDNSRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCreateQuestionRecord(
                    pszQuestionName,
                    wQType,
                    wQClass,
                    &pDNSQuestionRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSStdAddQuestionSection(
                    pDNSRequest,
                    pDNSQuestionRecord);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDNSQuestionRecord = NULL;

    dwError = DNSStdSendStdRequest2(
                    hDNSServer,
                    pDNSRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSStdReceiveStdResponse(
                    hDNSServer,
                    &pDNSResponse);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    *ppDNSResponse = pDNSResponse;
    
cleanup:
    
    if(pDNSRequest) {
        DNSStdFreeRequest(pDNSRequest);
    }

    return dwError;

error:

    if (pDNSResponse) {
        DNSStdFreeResponse(pDNSResponse);
    }
    
    if (pDNSQuestionRecord) {
        DNSFreeQuestionRecord(pDNSQuestionRecord);
    }

    goto cleanup;
}

DWORD
DNSStdSendStdRequest2(
    HANDLE hDNSServer,
    PDNS_REQUEST pDNSRequest
    )
{
    DWORD dwError = 0;
    DWORD dwBytesSent = 0;
    HANDLE hSendBuffer = (HANDLE)NULL;

    dwError = DNSBuildRequestMessage(pDNSRequest, &hSendBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSSendBufferContext(hDNSServer, hSendBuffer, &dwBytesSent);
    BAIL_ON_LWDNS_ERROR(dwError);

error:

    if(hSendBuffer){
        DNSFreeSendBufferContext(hSendBuffer);
    }

    return dwError;
}

DWORD
DNSStdSendRequestSynch2(
    HANDLE       hDNSServer,
    PDNS_REQUEST pDNSRequest,
    PDNS_RESPONSE * ppDNSResponse
    )
{
    DWORD dwError = 0;

    dwError = DNSStdSendStdRequest2(hDNSServer, pDNSRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSStdReceiveStdResponse(hDNSServer, ppDNSResponse);
    BAIL_ON_LWDNS_ERROR(dwError);
    
error:

    return dwError;
}

DWORD
DNSBuildRequestMessage(
    PDNS_REQUEST pDNSRequest,
    PHANDLE      phSendBuffer
    )
{
    DWORD  dwError = 0;
    CHAR   header[12];
    WORD   wnIdentification = 0;
    WORD   wnParameter = 0;
    WORD   wnQuestions = 0;
    WORD   wnAnswers = 0;
    WORD   wnAuthoritys = 0;
    WORD   wnAdditionals = 0;
    DWORD  dwRead = 0;
    HANDLE hSendBuffer = (HANDLE)NULL;

    dwError = DNSCreateSendBuffer(&hSendBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    wnIdentification = htons(pDNSRequest->wIdentification);
    memcpy(&header[0], (char *)&wnIdentification, sizeof(wnIdentification));

    wnParameter = htons(pDNSRequest->wParameter);
    memcpy(&header[2], (char *)&wnParameter, sizeof(wnParameter));

    wnQuestions = htons(pDNSRequest->wQuestions);
    memcpy(&header[4], (char *)&wnQuestions, sizeof(wnQuestions));

    wnAnswers = htons(pDNSRequest->wAnswers);
    memcpy(&header[6], (char *)&wnAnswers, sizeof(wnAnswers));

    wnAuthoritys = htons(pDNSRequest->wAuthoritys);
    memcpy(&header[8], (char *)&wnAuthoritys, sizeof(wnAuthoritys));

    wnAdditionals = htons(pDNSRequest->wAdditionals);
    memcpy(&header[10], (char *)&wnAdditionals, sizeof(wnAdditionals));

    dwError = DNSMarshallBuffer(
                hSendBuffer,
                (PBYTE)header,
                sizeof(header),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (pDNSRequest->wQuestions) {
        dwError = DNSStdMarshallQuestionSection(
                    hSendBuffer,
                    pDNSRequest->ppQuestionRRSet,
                    pDNSRequest->wQuestions);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (pDNSRequest->wAnswers) {
        dwError = DNSStdMarshallAnswerSection(
                    hSendBuffer,
                    pDNSRequest->ppAnswerRRSet,
                    pDNSRequest->wAnswers);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (pDNSRequest->wAuthoritys) {
        dwError = DNSStdMarshallAuthoritySection(
                    hSendBuffer,
                    pDNSRequest->ppAuthorityRRSet,
                    pDNSRequest->wAuthoritys);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (pDNSRequest->wAdditionals) {
        dwError = DNSStdMarshallAdditionalSection(
                    hSendBuffer,
                    pDNSRequest->ppAdditionalRRSet,
                    pDNSRequest->wAdditionals);
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    DNSDumpSendBufferContext(hSendBuffer);

    *phSendBuffer = hSendBuffer;
    
cleanup:

    return(dwError);

error:

    if(hSendBuffer) {
        DNSFreeSendBufferContext(hSendBuffer);
    }

    *phSendBuffer = (HANDLE)NULL;
    
    goto cleanup;
}

DWORD
DNSStdCreateStdRequest(
    PDNS_REQUEST * ppDNSRequest
    )
{
    DWORD dwError = 0;
    PDNS_REQUEST pDNSRequest = NULL;

    dwError = DNSAllocateMemory(
                sizeof(DNS_REQUEST),
                (PVOID *)&pDNSRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSGenerateIdentifier(
                &pDNSRequest->wIdentification);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppDNSRequest = pDNSRequest;

cleanup:

    return dwError;

error:

    if (pDNSRequest)
    {
        DNSStdFreeRequest(pDNSRequest);
    }
    
    *ppDNSRequest = NULL;
    
    goto cleanup;
}

DWORD
DNSStdAddQuestionSection(
    PDNS_REQUEST pDNSRequest,
    PDNS_QUESTION_RECORD pDNSQuestion
    )
{
    DWORD dwError = 0;
    DWORD dwNumQuestions = 0;

    dwNumQuestions = pDNSRequest->wQuestions;

    dwError = DNSReallocMemory(
                (PBYTE)pDNSRequest->ppQuestionRRSet,
                (PVOID *)&pDNSRequest->ppQuestionRRSet,
                (dwNumQuestions + 1) * sizeof(PDNS_QUESTION_RECORD));
    BAIL_ON_LWDNS_ERROR(dwError);

    *(pDNSRequest->ppQuestionRRSet + dwNumQuestions) = pDNSQuestion;

    pDNSRequest->wQuestions += 1;

error:

    return dwError;
}

DWORD
DNSStdAddAdditionalSection(
    PDNS_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    )
{
    DWORD dwError = 0;
    DWORD dwNumAdditionals = 0;

    dwNumAdditionals = pDNSRequest->wAdditionals;
    
    dwError = DNSReallocMemory(
                pDNSRequest->ppAdditionalRRSet,
                (PVOID *)&pDNSRequest->ppAdditionalRRSet,
                (dwNumAdditionals + 1) * sizeof(PDNS_RR_RECORD));
    BAIL_ON_LWDNS_ERROR(dwError);

    *(pDNSRequest->ppAdditionalRRSet + dwNumAdditionals) = pDNSRecord;

    pDNSRequest->wAdditionals += 1;

error:

    return dwError;
}

DWORD
DNSStdAddAnswerSection(
    PDNS_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    )
{
    DWORD dwError = 0;
    WORD wNumAnswers = 0;

    wNumAnswers = pDNSRequest->wAnswers;
    
    dwError = DNSReallocMemory(
                pDNSRequest->ppAnswerRRSet,
                (PVOID *)&pDNSRequest->ppAnswerRRSet,
                (wNumAnswers + 1) * sizeof(PDNS_RR_RECORD));
    BAIL_ON_LWDNS_ERROR(dwError);

    *(pDNSRequest->ppAnswerRRSet + wNumAnswers) = pDNSRecord;

    pDNSRequest->wAnswers += 1;

error:

    return dwError;
}

DWORD
DNSStdAddAuthoritySection(
    PDNS_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    )
{
    DWORD dwError = 0;
    WORD wNumAuthoritys = 0;
    
    wNumAuthoritys = pDNSRequest->wAuthoritys;
    
    dwError = DNSReallocMemory(
                pDNSRequest->ppAuthorityRRSet,
                (PVOID *)&pDNSRequest->ppAuthorityRRSet,
                (wNumAuthoritys + 1) * sizeof(PDNS_RR_RECORD));
    BAIL_ON_LWDNS_ERROR(dwError);

    *(pDNSRequest->ppAuthorityRRSet + wNumAuthoritys) = pDNSRecord;

    pDNSRequest->wAuthoritys += 1;

error:

    return(dwError);
}

VOID
DNSStdFreeRequest(
    PDNS_REQUEST pDNSRequest
    )
{
    if (pDNSRequest->ppQuestionRRSet)
    {
        DNSFreeQuestionRecordList(
                pDNSRequest->ppQuestionRRSet,
                pDNSRequest->wQuestions);
    }
    
    if (pDNSRequest->ppAnswerRRSet)
    {
        DNSFreeRecordList(
                pDNSRequest->ppAnswerRRSet,
                pDNSRequest->wAnswers);
    }
    
    if (pDNSRequest->ppAuthorityRRSet)
    {
        DNSFreeRecordList(
                pDNSRequest->ppAuthorityRRSet,
                pDNSRequest->wAuthoritys);
    }
    
    if (pDNSRequest->ppAdditionalRRSet)
    {
        DNSFreeRecordList(
                pDNSRequest->ppAdditionalRRSet,
                pDNSRequest->wAdditionals);
    }
    
    DNSFreeMemory(pDNSRequest);
}

static
DWORD
DNSStdMarshallQuestionSection(
    HANDLE hSendBuffer,
    PDNS_QUESTION_RECORD * ppDNSQuestionRecords,
    WORD wQuestions
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    for (i = 0; i < wQuestions; i++)
    {
        PDNS_QUESTION_RECORD pDNSQuestionRecord = NULL;
        WORD  wnQueryType = 0;
        WORD  wnQueryClass = 0;
        DWORD dwRead = 0;
        
        pDNSQuestionRecord = *(ppDNSQuestionRecords + i);
        dwError = DNSMarshallDomainName(
                    hSendBuffer,
                    pDNSQuestionRecord->pDomainName);
        BAIL_ON_LWDNS_ERROR(dwError);

        wnQueryType = htons(pDNSQuestionRecord->wQueryType);
        dwError = DNSMarshallBuffer(
                    hSendBuffer,
                    (PBYTE)&wnQueryType,
                    (DWORD)sizeof(WORD),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        wnQueryClass = htons(pDNSQuestionRecord->wQueryClass);
        dwError = DNSMarshallBuffer(
                    hSendBuffer,
                    (PBYTE)&wnQueryClass,
                    (DWORD)sizeof(WORD),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        pDNSQuestionRecord++;
    }

error:

    return(dwError);
}

static
DWORD
DNSStdMarshallAnswerSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAnswerRRRecords,
    WORD wAnswers
    )

{
    DWORD dwError = 0;
    DWORD i = 0;

    for (i = 0; i < wAnswers; i++)
    {
        PDNS_RR_RECORD pDNSAnswerRRRecord = NULL;
        
        pDNSAnswerRRRecord = 
            *(ppDNSAnswerRRRecords + i);

        dwError = DNSMarshallRRHeader(
                    hSendBuffer,
                    pDNSAnswerRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSMarshallRData(
                    hSendBuffer,
                    pDNSAnswerRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        pDNSAnswerRRRecord ++;
    }
    
error:

    return(dwError);
}

static
DWORD
DNSStdMarshallAuthoritySection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAuthorityRRRecords,
    WORD wAuthoritys
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    for (i = 0; i < wAuthoritys; i++)
    {
        PDNS_RR_RECORD pDNSAuthorityRRRecord = NULL;
        
        pDNSAuthorityRRRecord =
                *(ppDNSAuthorityRRRecords + i);

        dwError = DNSMarshallRRHeader(
                    hSendBuffer,
                    pDNSAuthorityRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError  = DNSMarshallRData(
                            hSendBuffer,
                            pDNSAuthorityRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
error:

    return(dwError);
}

static
DWORD
DNSStdMarshallAdditionalSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAdditionalsRRRecords,
    WORD wAdditionals
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_RR_RECORD pDNSAdditionalRRRecord = NULL;

    for (i = 0; i < wAdditionals; i++) {

        pDNSAdditionalRRRecord = *(ppDNSAdditionalsRRRecords + i);

        dwError = DNSMarshallRRHeader(
                    hSendBuffer,
                    pDNSAdditionalRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError  = DNSMarshallRData(
                        hSendBuffer,
                        pDNSAdditionalRRRecord
                        );
        BAIL_ON_LWDNS_ERROR(dwError);
    }

error:

    return(dwError);
}

DWORD
DNSMarshallDomainName(
    HANDLE hSendBuffer,
    PDNS_DOMAIN_NAME  pDomainName
    )
{
    DWORD dwError = 0;
    PDNS_DOMAIN_LABEL pTemp = NULL;
    DWORD dwSent = 0;
    char uEndChar = '\0';

    pTemp = pDomainName->pLabelList;
    
    while(pTemp)
    {
        BYTE len = pTemp->dwLength;
        
        dwError = DNSMarshallBuffer(
                    hSendBuffer,
                    &len,
                    sizeof(len),
                    &dwSent);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSMarshallBuffer(
                    hSendBuffer,
                    (PBYTE)pTemp->pszLabel,
                    len,
                    &dwSent);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        pTemp = pTemp->pNext;
    }
    
    DNSMarshallBuffer(
        hSendBuffer,
        (PBYTE)&uEndChar,
        1,
        &dwSent);

error:

    return(dwError);
}

DWORD
DNSMarshallRRHeader(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD pDNSRecord
    )
{
    DWORD dwError = 0;
    DWORD dwRead = 0;
    WORD wnType = 0;
    WORD wnClass = 0;
    DWORD dwnTTL = 0;

    dwError = DNSMarshallDomainName(
                hSendBuffer,
                pDNSRecord->RRHeader.pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    wnType = htons(pDNSRecord->RRHeader.wType);
    dwError = DNSMarshallBuffer(
                hSendBuffer,
                (PBYTE)&wnType,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);

    wnClass = htons(pDNSRecord->RRHeader.wClass);
    dwError = DNSMarshallBuffer(
                hSendBuffer,
                (PBYTE)&wnClass,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwnTTL = htonl(pDNSRecord->RRHeader.dwTTL);
    dwError = DNSMarshallBuffer(
                hSendBuffer,
                (PBYTE)&dwnTTL,
                sizeof(DWORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);

error:

    return(dwError);
}

DWORD
DNSMarshallRData(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD pDNSRecord
    )
{
    DWORD dwError = 0;
    DWORD dwWritten = 0;
    WORD wnRDataSize = 0;
    PDNS_SENDBUFFER_CONTEXT pDNSContext = NULL;
    DWORD dwSizeOffset = 0;
    DWORD dwDataStartOffset = 0;
    DWORD  dwRead = 0;

    pDNSContext = (PDNS_SENDBUFFER_CONTEXT)hSendBuffer;

    wnRDataSize = htons(pDNSRecord->RRHeader.wRDataSize);
    // Remember where the rdata size is written in the buffer so it can be
    // fixed up later
    dwSizeOffset = pDNSContext->dwBufferOffset;
    dwError = DNSMarshallBuffer(
                hSendBuffer,
                (PBYTE)&wnRDataSize,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwDataStartOffset = pDNSContext->dwBufferOffset;

    if (pDNSRecord->pRDataDomain)
    {
        dwError = DNSMarshallDomainName(
                    hSendBuffer,
                    pDNSRecord->pRDataDomain);
        BAIL_ON_LWDNS_ERROR(dwError);

        // Fix up the length
        wnRDataSize = htons(pDNSContext->dwBufferOffset - dwDataStartOffset);
        memcpy(&pDNSContext->pSendBuffer[dwSizeOffset],
               &wnRDataSize,
               sizeof(wnRDataSize));
    }
    else
    {
        dwError = DNSMarshallBuffer(
                    hSendBuffer,
                    pDNSRecord->pRData,
                    pDNSRecord->RRHeader.wRDataSize,
                    &dwWritten);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}
