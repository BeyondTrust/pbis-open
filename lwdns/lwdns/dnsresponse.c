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
 *        dnsresponse.c
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
DNSStdUnmarshallQuestionSection(    
    HANDLE hReceiveBuffer,
    WORD wQuestions,
    PDNS_QUESTION_RECORD ** pppDNSQuestionRecords
    );

static
DWORD
DNSStdUnmarshallAnswerSection(
    HANDLE hReceiveBuffer,
    WORD   wAnswers,
    PDNS_RR_RECORD ** pppDNSAnswerRRRecords
    );

static
DWORD
DNSStdUnmarshallAuthoritySection(
    HANDLE hReceiveBuffer,
    WORD   wAuthoritys,
    PDNS_RR_RECORD ** pppDNSAuthorityRRRecords
    );

static
DWORD 
DNSStdUnmarshallAdditionalSection(
    HANDLE hReceiveBuffer,
    WORD   wAdditionals,
    PDNS_RR_RECORD ** pppDNSAdditionalsRRRecords
    );

static
DWORD
DNSStdAllocateResponse(
    PDNS_RESPONSE * ppDNSResponse
    );

static
DWORD
DNSUnmarshallDomainNameAtOffset(
    HANDLE hRecvBuffer,
    WORD wOffset,
    PDNS_DOMAIN_LABEL * ppDomainLabel
    );

DWORD
DNSStdReceiveStdResponse(
    HANDLE         hDNSHandle,
    PDNS_RESPONSE* ppDNSResponse
    )
{
    PDNS_RESPONSE pDNSResponse = NULL;
    DWORD dwError = 0;
    WORD wnIdentification,wIdentification = 0;
    WORD wnParameter, wParameter = 0;
    WORD wnQuestions, wQuestions = 0;
    WORD wnAnswers, wAnswers = 0;
    WORD wnAdditionals, wAdditionals = 0;
    WORD wnAuthoritys, wAuthoritys = 0;
    DWORD dwRead = 0;
    PDNS_RR_RECORD * ppDNSAnswerRecords = NULL;
    PDNS_RR_RECORD *  ppDNSAdditionalRecords = NULL;
    PDNS_RR_RECORD *  ppDNSAuthorityRecords = NULL;
    PDNS_QUESTION_RECORD * ppDNSQuestionRecords = NULL;
    HANDLE hRecvBuffer = (HANDLE)NULL;

    dwError = DNSCreateReceiveBuffer(&hRecvBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSReceiveBufferContext(
                hDNSHandle,
                hRecvBuffer,
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSDumpRecvBufferContext(hRecvBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnIdentification,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    wIdentification = ntohs(wnIdentification);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnParameter,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    wParameter = ntohs(wnParameter);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnQuestions,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    wQuestions = ntohs(wnQuestions);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnAnswers,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    wAnswers = ntohs(wnAnswers);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnAuthoritys,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    wAuthoritys = ntohs(wnAuthoritys);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnAdditionals,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    wAdditionals = ntohs(wnAdditionals);

    if (wQuestions)
    {
        dwError = DNSStdUnmarshallQuestionSection(
                    hRecvBuffer, 
                    wQuestions, 
                    &ppDNSQuestionRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (wAnswers)
    {
        dwError = DNSStdUnmarshallAnswerSection(
                    hRecvBuffer, 
                    wAnswers, 
                    &ppDNSAnswerRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (wAuthoritys)
    {
        dwError = DNSStdUnmarshallAuthoritySection(
                    hRecvBuffer, 
                    wAuthoritys, 
                    &ppDNSAuthorityRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (wAdditionals)
    {
        dwError = DNSStdUnmarshallAdditionalSection(
                    hRecvBuffer, 
                    wAdditionals, 
                    &ppDNSAdditionalRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSStdAllocateResponse(&pDNSResponse);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDNSResponse->wIdentification = wIdentification;
    pDNSResponse->wParameter = wParameter;
    pDNSResponse->wQuestions = wQuestions;
    pDNSResponse->wAnswers = wAnswers;
    pDNSResponse->wAuthoritys = wAuthoritys;
    pDNSResponse->wAdditionals = wAdditionals;

    pDNSResponse->ppQuestionRRSet = ppDNSQuestionRecords;
    pDNSResponse->ppAnswerRRSet = ppDNSAnswerRecords;
    pDNSResponse->ppAuthorityRRSet = ppDNSAuthorityRecords;
    pDNSResponse->ppAdditionalRRSet = ppDNSAdditionalRecords;
    
    *ppDNSResponse = pDNSResponse;
    
cleanup:

    if (hRecvBuffer != (HANDLE)NULL)
    {
        DNSFreeReceiveBufferContext(hRecvBuffer);
    }

    return dwError;

error:

    if (ppDNSAnswerRecords)
    {
        DNSFreeRecordList(
                ppDNSAnswerRecords,
                wAnswers);
    }
    
    if (ppDNSAdditionalRecords)
    {
        DNSFreeRecordList(
                ppDNSAdditionalRecords,
                wAdditionals);
    }
    
    if (ppDNSAuthorityRecords)
    {
        DNSFreeRecordList(
                ppDNSAuthorityRecords,
                wAuthoritys);
    }
    
    if (ppDNSQuestionRecords)
    {
        DNSFreeQuestionRecordList(
                ppDNSQuestionRecords,
                wQuestions);
    }
    
    *ppDNSResponse = NULL;

    goto cleanup;
}

DWORD
DNSUpdateGetResponseCode(
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse,
    PDWORD pdwResponseCode
    )
{
    *pdwResponseCode = DNSMapResponseCode(pDNSUpdateResponse->wParameter);

    return 0;
}

DWORD
DNSStdGetResponseCode(
    PDNS_RESPONSE pDNSResponse
    )
{
    return(DNSMapResponseCode(pDNSResponse->wParameter));    
}

static
DWORD
DNSStdUnmarshallQuestionSection(    
    HANDLE hReceiveBuffer,
    WORD wQuestions,
    PDNS_QUESTION_RECORD ** pppDNSQuestionRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_QUESTION_RECORD pDNSQuestionRecord = NULL;
    PDNS_QUESTION_RECORD * ppDNSQuestionRecords = NULL;

    dwError = DNSAllocateMemory(
                    wQuestions * sizeof(PDNS_QUESTION_RECORD), 
                    (PVOID *)&ppDNSQuestionRecords);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (i = 0; i < wQuestions; i++)
    {   
        DWORD dwRead = 0;
        WORD  wnQueryClass = 0;
        WORD  wnQueryType = 0;
        
        dwError = DNSAllocateMemory(
                    sizeof(DNS_QUESTION_RECORD),
                    (PVOID *)&pDNSQuestionRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSUnmarshallDomainName(
                    hReceiveBuffer,
                    &pDNSQuestionRecord->pDomainName);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSUnmarshallBuffer(
                    hReceiveBuffer,
                    (PBYTE)&wnQueryType,
                    (DWORD)sizeof(WORD),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);
        pDNSQuestionRecord->wQueryType = ntohs(wnQueryType);
        
        dwError = DNSUnmarshallBuffer(
                    hReceiveBuffer,
                    (PBYTE)&wnQueryClass,
                    (DWORD)sizeof(WORD),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        pDNSQuestionRecord->wQueryClass = ntohs(wnQueryClass);

        *(ppDNSQuestionRecords + i) = pDNSQuestionRecord;
        pDNSQuestionRecord = NULL;
    }    

    *pppDNSQuestionRecords = ppDNSQuestionRecords;
    
cleanup:

    return dwError;

error:

    if (ppDNSQuestionRecords)
    {
        DNSFreeQuestionRecordList(
                ppDNSQuestionRecords,
                wQuestions);
    }
    
    if (pDNSQuestionRecord)
    {
        DNSFreeQuestionRecord(pDNSQuestionRecord);
    }

    *pppDNSQuestionRecords = NULL;

    goto cleanup; 
}

static
DWORD
DNSStdUnmarshallAnswerSection(
    HANDLE hReceiveBuffer,
    WORD   wAnswers,
    PDNS_RR_RECORD ** pppDNSAnswerRRRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_RR_RECORD * ppDNSAnswerRRRecords = NULL;
    PBYTE pRData = NULL;
    DNS_RR_HEADER RRHeader = {0};
    PDNS_RR_HEADER pRRHeader = &RRHeader;

    dwError = DNSAllocateMemory(
                    wAnswers * sizeof(PDNS_RR_RECORD), 
                    (PVOID *)&ppDNSAnswerRRRecords);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (i = 0; i < wAnswers; i++)
    {
        DWORD dwRead = 0;
        
        memset(pRRHeader, 0, sizeof(DNS_RR_HEADER));
        
        dwError = DNSUnmarshallRRHeader(
                    hReceiveBuffer,
                    pRRHeader);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        dwError = DNSUnmarshallRData(
                    hReceiveBuffer,
                    pRRHeader->wRDataSize,
                    &pRData,
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        memcpy(&pDNSRRRecord->RRHeader, pRRHeader, sizeof(DNS_RR_HEADER));
        pRRHeader->pDomainName = NULL;
        
        pDNSRRRecord->pRData = pRData;
        pRData = NULL;

        *(ppDNSAnswerRRRecords + i) = pDNSRRRecord;
        pDNSRRRecord = NULL;
    }    
    
    *pppDNSAnswerRRRecords = ppDNSAnswerRRRecords;

cleanup:
    
    return(dwError);
    
error:

    if (pRData)
    {
        DNSFreeMemory(pRData);
    }
    
    if (pDNSRRRecord)
    {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    if (pRRHeader && pRRHeader->pDomainName)
    {
        DNSFreeDomainName(pRRHeader->pDomainName);
    }
    
    if (ppDNSAnswerRRRecords)
    {
        DNSFreeRecordList(
                ppDNSAnswerRRRecords,
                wAnswers);
    }
    
    *pppDNSAnswerRRRecords = NULL;

    goto cleanup;
}

static
DWORD
DNSStdUnmarshallAuthoritySection(
    HANDLE hReceiveBuffer,
    WORD   wAuthoritys,
    PDNS_RR_RECORD ** pppDNSAuthorityRRRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_RR_RECORD * ppDNSAuthorityRRRecords = NULL;
    PBYTE pRData = NULL;
    DNS_RR_HEADER RRHeader = {0};
    PDNS_RR_HEADER pRRHeader = &RRHeader;

    dwError = DNSAllocateMemory(
                    wAuthoritys * sizeof(PDNS_RR_RECORD), 
                    (PVOID *)&ppDNSAuthorityRRRecords);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (i = 0; i < wAuthoritys; i++)
    {
        DWORD dwRead = 0;
        
        memset(pRRHeader, 0, sizeof(DNS_RR_HEADER));
        
        dwError = DNSUnmarshallRRHeader(
                    hReceiveBuffer,
                    pRRHeader);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        dwError = DNSUnmarshallRData(
                    hReceiveBuffer,
                    pRRHeader->wRDataSize,
                    &pRData,
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        memcpy(&pDNSRRRecord->RRHeader, pRRHeader, sizeof(DNS_RR_HEADER));
        pRRHeader->pDomainName = NULL;
        
        pDNSRRRecord->pRData = pRData;
        pRData = NULL;

        *(ppDNSAuthorityRRRecords + i) = pDNSRRRecord;
        pDNSRRRecord = NULL;
    }    

    *pppDNSAuthorityRRRecords = ppDNSAuthorityRRRecords;

cleanup:

    return dwError;
    
error:

    if (pRData)
    {
        DNSFreeMemory(pRData);
    }
    
    if (pDNSRRRecord)
    {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    if (pRRHeader && pRRHeader->pDomainName)
    {
        DNSFreeDomainName(pRRHeader->pDomainName);
    }
    
    if (ppDNSAuthorityRRRecords)
    {
        DNSFreeRecordList(
                ppDNSAuthorityRRRecords,
                wAuthoritys);
    }

    *pppDNSAuthorityRRRecords = NULL;
    
    goto cleanup;
}

static
DWORD 
DNSStdUnmarshallAdditionalSection(
    HANDLE hReceiveBuffer,
    WORD   wAdditionals,
    PDNS_RR_RECORD ** pppDNSAdditionalsRRRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_RR_RECORD * ppDNSAdditionalRRRecords = NULL;
    PBYTE pRData = NULL;
    DNS_RR_HEADER RRHeader = {0};
    PDNS_RR_HEADER pRRHeader = &RRHeader;

    dwError = DNSAllocateMemory(
                wAdditionals * sizeof(PDNS_RR_RECORD), 
                (PVOID *)&ppDNSAdditionalRRRecords);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (i = 0; i < wAdditionals; i++)
    {
        DWORD dwRead = 0;
        
        memset(pRRHeader, 0, sizeof(DNS_RR_HEADER));
        
        dwError = DNSUnmarshallRRHeader(
                    hReceiveBuffer,
                    pRRHeader);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        dwError = DNSUnmarshallRData(
                    hReceiveBuffer,
                    pRRHeader->wRDataSize,
                    &pRData,
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        memcpy(&pDNSRRRecord->RRHeader, pRRHeader, sizeof(DNS_RR_HEADER));
        pRRHeader->pDomainName = NULL;
        
        pDNSRRRecord->pRData = pRData;
        pRData = NULL;
        
        *(ppDNSAdditionalRRRecords + i) = pDNSRRRecord;
        pDNSRRRecord = NULL;
    }
    
    *pppDNSAdditionalsRRRecords = ppDNSAdditionalRRRecords;
    
cleanup:

    return dwError;

error:

    if (pRData)
    {
        DNSFreeMemory(pRData);
    }
    
    if (pDNSRRRecord)
    {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    if (pRRHeader && pRRHeader->pDomainName)
    {
        DNSFreeDomainName(pRRHeader->pDomainName);
    }
    
    if (ppDNSAdditionalRRRecords)
    {
        DNSFreeRecordList(
                ppDNSAdditionalRRRecords,
                wAdditionals);
    }
    
    *pppDNSAdditionalsRRRecords = NULL;

    goto cleanup;
}

static
DWORD
DNSStdAllocateResponse(
    PDNS_RESPONSE * ppDNSResponse
    )
{
    DWORD dwError = 0;
    PDNS_RESPONSE pDNSResponse = NULL;

    dwError = DNSAllocateMemory(
                sizeof(DNS_RESPONSE),
                (PVOID *)&pDNSResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppDNSResponse = pDNSResponse;

cleanup:

    return dwError;

error:

    *ppDNSResponse = NULL;
    
    if (pDNSResponse)
    {
        DNSStdFreeResponse(pDNSResponse);
    }

    goto cleanup;
}

VOID
DNSStdFreeResponse(
    PDNS_RESPONSE pDNSResponse
    )
{   
    if (pDNSResponse->ppQuestionRRSet)
    {
        DNSFreeQuestionRecordList(
                pDNSResponse->ppQuestionRRSet,
                pDNSResponse->wQuestions);
    }
    
    if (pDNSResponse->ppAnswerRRSet)
    {
        DNSFreeRecordList(
                pDNSResponse->ppAnswerRRSet,
                pDNSResponse->wAnswers);
    }
    
    if (pDNSResponse->ppAuthorityRRSet)
    {
        DNSFreeRecordList(
                pDNSResponse->ppAuthorityRRSet,
                pDNSResponse->wAuthoritys);
    }
    
    if (pDNSResponse->ppAdditionalRRSet)
    {
        DNSFreeRecordList(
                pDNSResponse->ppAdditionalRRSet,
                pDNSResponse->wAdditionals);
    }
    
    if (pDNSResponse->pDNSOutBuffer)
    {
        DNSFreeMemory(pDNSResponse->pDNSOutBuffer);
    }
    
    DNSFreeMemory(pDNSResponse);
}

DWORD
DNSUnmarshallDomainName(
    HANDLE hRecvBuffer, 
    PDNS_DOMAIN_NAME * ppDomainName
    )
{
    DWORD dwError = 0;
    PDNS_DOMAIN_LABEL pLabel = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;
    BYTE uLen = 0;
    DWORD dwRead = 0;
    BYTE uLen1, uLen2 = 0;
    WORD wOffset = 0;
    BOOLEAN bDone = FALSE;
    
    dwError = DNSAllocateMemory(
                sizeof(DNS_DOMAIN_NAME),
                (PVOID *)&pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    do
    {
        dwError = DNSUnmarshallBuffer(
                    hRecvBuffer,
                    &uLen1,
                    sizeof(char),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        switch (uLen1 & 0xC0)
        {
            case 0xC0: /* pointer to string */

                dwError = DNSUnmarshallBuffer(
                            hRecvBuffer,
                            &uLen2,
                            sizeof(char),
                            &dwRead);
                BAIL_ON_LWDNS_ERROR(dwError);

                wOffset = ntohs((((WORD)(uLen1) << 8)|uLen2) & 0x3FFF);

                dwError = DNSUnmarshallDomainNameAtOffset(
                            hRecvBuffer,
                            wOffset,
                            &pLabel);
                BAIL_ON_LWDNS_ERROR(dwError);

                dwError = DNSAppendLabel(
                                pDomainName->pLabelList,
                                pLabel,
                                &pDomainName->pLabelList);
                BAIL_ON_LWDNS_ERROR(dwError);
                pLabel = NULL;

                break;

            case 0x00: /* string component */

                {
                    CHAR szLabel[65];

                    dwError = DNSReceiveBufferMoveBackIndex(hRecvBuffer, 1);
                    BAIL_ON_LWDNS_ERROR(dwError);

                    dwError = DNSUnmarshallBuffer(
                                hRecvBuffer,
                                &uLen,
                                sizeof(char),
                                &dwRead);
                    BAIL_ON_LWDNS_ERROR(dwError);

                    if (uLen == 0)
                    {
                        bDone = TRUE;
                        break;
                    }

                    memset(szLabel, 0, sizeof(szLabel));

                    dwError = DNSUnmarshallBuffer(
                                hRecvBuffer,
                                (PBYTE)szLabel,
                                uLen,
                                &dwRead);
                    BAIL_ON_LWDNS_ERROR(dwError);

                    dwError = DNSAllocateMemory(
                                sizeof(DNS_DOMAIN_LABEL),
                                (PVOID *)&pLabel);
                    BAIL_ON_LWDNS_ERROR(dwError);

                    dwError = DNSAllocateString(szLabel, &pLabel->pszLabel);
                    BAIL_ON_LWDNS_ERROR(dwError);

                    dwError = DNSAppendLabel(
                                pDomainName->pLabelList,
                                pLabel,
                                &pDomainName->pLabelList);
                    BAIL_ON_LWDNS_ERROR(dwError);

                    pLabel = NULL;
                }

                break;

            default:   /* unexpected */

                dwError = ERROR_BAD_NET_RESP;
                BAIL_ON_LWDNS_ERROR(dwError);

                break;
        }
    } while (!bDone);

    *ppDomainName = pDomainName;
    
cleanup:

    return(dwError);
    
error:

    if (pDomainName)
    {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pLabel)
    {
        DNSFreeLabel(pLabel);
    }
    
    *ppDomainName = NULL;
    
    goto cleanup;
}

/*
 * Note: This function reads the string without updating the index
 */
static
DWORD
DNSUnmarshallDomainNameAtOffset(
    HANDLE hRecvBuffer,
    WORD wOffset,
    PDNS_DOMAIN_LABEL * ppDomainLabel
    )
{
    DWORD dwError = 0;
    PDNS_DOMAIN_LABEL pLabel = NULL;
    BYTE  uLen = 0;
    DWORD dwCurrent = 0;
    PDNS_RECEIVEBUFFER_CONTEXT pRecvContext = NULL;
    CHAR szLabel[65];

    pRecvContext = (PDNS_RECEIVEBUFFER_CONTEXT)hRecvBuffer;
    dwCurrent = wOffset;

    memcpy(&uLen, pRecvContext->pRecvBuffer+dwCurrent, sizeof(char));

    switch (uLen & 0xC0)
    {
        case 0x00:

            dwCurrent++;

            memset(szLabel, 0, sizeof(szLabel));
            memcpy(szLabel, pRecvContext->pRecvBuffer+dwCurrent, uLen);
            dwCurrent += uLen;

            dwError = DNSAllocateMemory(
                            sizeof(DNS_DOMAIN_LABEL),
                            (PVOID *)&pLabel);
            BAIL_ON_LWDNS_ERROR(dwError);

            dwError = DNSAllocateString(szLabel, &pLabel->pszLabel);
            BAIL_ON_LWDNS_ERROR(dwError);

            break;

        default:

            dwError = ERROR_BAD_NET_RESP;
            BAIL_ON_LWDNS_ERROR(dwError);

            break;
    }

    *ppDomainLabel = pLabel;

cleanup:

    return dwError;

error:

    *ppDomainLabel = NULL;

    if (pLabel)
    {
        DNSFreeLabel(pLabel);
    }

    goto cleanup;
}

DWORD
DNSUnmarshallRRHeader(
    HANDLE         hRecvBuffer,
    PDNS_RR_HEADER pRRHeader
    )
{
    DWORD dwError = 0;
    DWORD dwRead = 0;
    WORD wnType = 0;
    WORD wnClass = 0;
    WORD wnRDataSize = 0;
    DWORD dwnTTL = 0;

    dwError = DNSUnmarshallDomainName(
                hRecvBuffer,
                &pRRHeader->pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnType,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    pRRHeader->wType = ntohs(wnType);
    
    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnClass,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    pRRHeader->wClass = ntohs(wnClass);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&dwnTTL,
                sizeof(DWORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    pRRHeader->dwTTL = ntohl(dwnTTL);
    
    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnRDataSize,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    pRRHeader->wRDataSize = htons(wnRDataSize);

error:

    return dwError;
}

DWORD
DNSUnmarshallRData(
    HANDLE hRecvBuffer,
    DWORD  dwSize,
    PBYTE* ppRData,
    PDWORD pdwRead
    )
{
    DWORD dwError = 0;
    PBYTE pMemory = NULL;

    dwError = DNSAllocateMemory(
                dwSize,
                (PVOID *)&pMemory);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)pMemory,
                dwSize,
                pdwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    *ppRData = pMemory;

cleanup:

    return(dwError);

error:

    if (pMemory) {
        DNSFreeMemory(pMemory);
    }

    *ppRData = NULL;
    
    goto cleanup;
}


