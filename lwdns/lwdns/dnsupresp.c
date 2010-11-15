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
 *        dnsupresp.c
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
DNSUpdateAllocateResponse(
    PDNS_UPDATE_RESPONSE * ppDNSResponse
    );

DWORD
DNSUpdateReceiveUpdateResponse(
    HANDLE hDNSHandle,
    PDNS_UPDATE_RESPONSE * ppDNSResponse
    )
{
    PDNS_UPDATE_RESPONSE pDNSResponse = NULL;
    DWORD dwError = 0;
    WORD wnIdentification,wIdentification = 0;
    WORD wnParameter, wParameter = 0;
    WORD wnZones, wZones = 0;
    WORD wnPRs, wPRs = 0;
    WORD wnAdditionals, wAdditionals = 0;
    WORD wnUpdates, wUpdates = 0;
    DWORD dwRead = 0;
    PDNS_RR_RECORD * ppDNSPRRecords = NULL;
    PDNS_RR_RECORD *  ppDNSAdditionalRecords = NULL;
    PDNS_RR_RECORD *  ppDNSUpdateRecords = NULL;
    PDNS_ZONE_RECORD * ppDNSZoneRecords = NULL;
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
                (PBYTE)&wnZones,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    wZones = ntohs(wnZones);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnPRs,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    wPRs = ntohs(wnPRs);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnUpdates,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    wUpdates = ntohs(wnUpdates);

    dwError = DNSUnmarshallBuffer(
                hRecvBuffer,
                (PBYTE)&wnAdditionals,
                sizeof(WORD),
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    wAdditionals = ntohs(wnAdditionals);

    if (wZones)
    {
        dwError = DNSUpdateUnmarshallZoneSection(
                    hRecvBuffer,
                    wZones,
                    &ppDNSZoneRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (wPRs)
    {
        dwError = DNSUpdateUnmarshallPRSection(
                    hRecvBuffer,
                    wPRs,
                    &ppDNSPRRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (wUpdates)
    {
        dwError = DNSUpdateUnmarshallUpdateSection(
                    hRecvBuffer,
                    wUpdates,
                    &ppDNSUpdateRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (wAdditionals)
    {
        dwError = DNSUpdateUnmarshallAdditionalSection(
                    hRecvBuffer,
                    wAdditionals,
                    &ppDNSAdditionalRecords);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSUpdateAllocateResponse(&pDNSResponse);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDNSResponse->wIdentification = wIdentification;
    pDNSResponse->wParameter = wParameter;
    pDNSResponse->wZones = wZones;
    pDNSResponse->wPRs = wPRs;
    pDNSResponse->wUpdates = wUpdates;
    pDNSResponse->wAdditionals = wAdditionals;

    pDNSResponse->ppZoneRRSet = ppDNSZoneRecords;
    pDNSResponse->ppPRRRSet = ppDNSPRRecords;
    pDNSResponse->ppUpdateRRSet = ppDNSUpdateRecords;
    pDNSResponse->ppAdditionalRRSet = ppDNSAdditionalRecords;
    
    *ppDNSResponse = pDNSResponse;

cleanup:

    if (hRecvBuffer != (HANDLE)NULL)
    {
        DNSFreeReceiveBufferContext(hRecvBuffer);
    }

    return dwError;
    
error:

    if (pDNSResponse)
    {
        DNSUpdateFreeResponse(pDNSResponse);
    }
    
    if (ppDNSPRRecords)
    {
        DNSFreeRecordList(
                ppDNSPRRecords,
                wPRs);
    }
    
    if (ppDNSAdditionalRecords)
    {
        DNSFreeRecordList(
                ppDNSAdditionalRecords,
                wAdditionals);
    }
    
    if (ppDNSUpdateRecords)
    {
        DNSFreeRecordList(
                ppDNSUpdateRecords,
                wUpdates);
    }
    
    if (ppDNSZoneRecords)
    {
        DNSFreeZoneRecordList(
                ppDNSZoneRecords,
                wZones);
    }

    *ppDNSResponse = NULL;

    goto cleanup;
}    
    

DWORD
DNSUpdateUnmarshallZoneSection(    
    HANDLE hReceiveBuffer,
    WORD wZones,
    PDNS_ZONE_RECORD ** pppDNSZoneRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_ZONE_RECORD * ppDNSZoneRecords = NULL;
    
    dwError = DNSAllocateMemory(
                wZones * sizeof(PDNS_ZONE_RECORD), 
                (PVOID *)&ppDNSZoneRecords);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (i = 0; i < wZones; i++)
    {
        DWORD dwRead = 0;
        WORD wnZoneClass = 0;
        WORD wnZoneType = 0;
        
        dwError = DNSAllocateMemory(
                    sizeof(DNS_ZONE_RECORD),
                    (PVOID *)&pDNSZoneRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSUnmarshallDomainName(
                    hReceiveBuffer,
                    &pDNSZoneRecord->pDomainName);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSUnmarshallBuffer(
                    hReceiveBuffer,
                    (PBYTE)&wnZoneType,
                    (DWORD)sizeof(WORD),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        pDNSZoneRecord->wZoneType = ntohs(wnZoneType);
        
        dwError = DNSUnmarshallBuffer(
                    hReceiveBuffer,
                    (PBYTE)&wnZoneClass,
                    (DWORD)sizeof(WORD),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        pDNSZoneRecord->wZoneClass = ntohs(wnZoneClass);

        *(ppDNSZoneRecords + i) = pDNSZoneRecord;
        pDNSZoneRecord = NULL;
    }    

    *pppDNSZoneRecords = ppDNSZoneRecords;
    
cleanup:

    return(dwError);

error:

    if (pDNSZoneRecord)
    {
        DNSFreeZoneRecord(pDNSZoneRecord);
    }
    
    if (ppDNSZoneRecords)
    {
        DNSFreeZoneRecordList(
                ppDNSZoneRecords,
                wZones);
    }
    
    *pppDNSZoneRecords = NULL;

    goto cleanup;   
}

DWORD
DNSUpdateUnmarshallPRSection(
    HANDLE hReceiveBuffer,
    WORD wPRs,
    PDNS_RR_RECORD ** pppDNSPRRRRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_RR_RECORD * ppDNSPRRRRecords = NULL;
    PBYTE pRRData = NULL;
    DNS_RR_HEADER RRHeader = {0};
    PDNS_RR_HEADER pRRHeader = &RRHeader;

    dwError = DNSAllocateMemory(
                wPRs * sizeof(PDNS_RR_RECORD), 
                (PVOID *)&ppDNSPRRRRecords);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (i = 0; i < wPRs; i++)
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
                    &pRRData, &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        memcpy(&pDNSRRRecord->RRHeader,
               pRRHeader,
               sizeof(DNS_RR_HEADER));
        pRRHeader->pDomainName = NULL;
        
        pDNSRRRecord->pRData = pRRData;
        pRRData = NULL;

        *(ppDNSPRRRRecords + i) = pDNSRRRecord;
        pDNSRRRecord = NULL;
    }    
    
    *pppDNSPRRRRecords = ppDNSPRRRRecords;

cleanup:

    return(dwError);

error:

    if (pRRData)
    {
        DNSFreeMemory(pRRData);
    }
    
    if (pDNSRRRecord)
    {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    if (pRRHeader && pRRHeader->pDomainName)
    {
        DNSFreeDomainName(pRRHeader->pDomainName);
    }
    
    if (ppDNSPRRRRecords)
    {
        DNSFreeRecordList(
                ppDNSPRRRRecords,
                wPRs);
    }
    
    *pppDNSPRRRRecords = NULL;
    
    goto cleanup;
}

DWORD
DNSUpdateUnmarshallUpdateSection(
    HANDLE hReceiveBuffer,
    WORD wUpdates,
    PDNS_RR_RECORD ** pppDNSUpdateRRRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_RR_RECORD * ppDNSUpdateRRRecords = NULL;
    PBYTE pRRData = NULL;
    DNS_RR_HEADER RRHeader = {0};
    PDNS_RR_HEADER pRRHeader = &RRHeader;
    
    dwError = DNSAllocateMemory(
                wUpdates * sizeof(PDNS_RR_RECORD), 
                (PVOID *)&ppDNSUpdateRRRecords);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (i = 0; i < wUpdates; i++)
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
                    &pRRData, &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        memcpy(&pDNSRRRecord->RRHeader,
               pRRHeader,
               sizeof(DNS_RR_HEADER));
        pRRHeader->pDomainName = NULL;
        
        pDNSRRRecord->pRData = pRRData;
        pRRData = NULL;

        *(ppDNSUpdateRRRecords + i) = pDNSRRRecord;
        pDNSRRRecord = NULL;
    }    

    *pppDNSUpdateRRRecords = ppDNSUpdateRRRecords;

cleanup:

    return(dwError);

error:

    if (pRRData)
    {
        DNSFreeMemory(pRRData);
    }
    
    if (pDNSRRRecord)
    {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    if (pRRHeader && pRRHeader->pDomainName)
    {
        DNSFreeDomainName(pRRHeader->pDomainName);
    }
    
    if (ppDNSUpdateRRRecords)
    {
        DNSFreeRecordList(
                ppDNSUpdateRRRecords,
                wUpdates);
    }
    
    *pppDNSUpdateRRRecords = NULL;

    goto cleanup;
}

DWORD 
DNSUpdateUnmarshallAdditionalSection(
    HANDLE hReceiveBuffer,
    WORD   wAdditionals,
    PDNS_RR_RECORD ** pppDNSAdditionalsRRRecords
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_RR_RECORD * ppDNSAdditionalRRRecords = NULL;
    PBYTE pRRData = NULL;
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
                    &pRRData, 
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        memcpy(&pDNSRRRecord->RRHeader,
               pRRHeader,
               sizeof(DNS_RR_HEADER));
        pRRHeader->pDomainName = NULL;
        
        pDNSRRRecord->pRData = pRRData;
        pRRData = NULL;

        *(ppDNSAdditionalRRRecords + i) = pDNSRRRecord;
        pDNSRRRecord = NULL;
    }
    
    *pppDNSAdditionalsRRRecords = ppDNSAdditionalRRRecords;

cleanup:

    return(dwError);
    
error:

    if (pRRData)
    {
        DNSFreeMemory(pRRData);
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
DNSUpdateAllocateResponse(
    PDNS_UPDATE_RESPONSE * ppDNSResponse
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_RESPONSE pDNSResponse = NULL;

    dwError = DNSAllocateMemory(
                sizeof(DNS_UPDATE_RESPONSE),
                (PVOID *)&pDNSResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppDNSResponse = pDNSResponse;

cleanup:

    return(dwError);

error:

    *ppDNSResponse = NULL;

    goto cleanup;
}

VOID
DNSUpdateFreeResponse(
    PDNS_UPDATE_RESPONSE pDNSResponse
    )
{
    if (pDNSResponse->ppZoneRRSet)
    {
        DNSFreeZoneRecordList(
                pDNSResponse->ppZoneRRSet,
                pDNSResponse->wZones);
    }
    
    if (pDNSResponse->ppPRRRSet)
    {
        DNSFreeRecordList(
                pDNSResponse->ppPRRRSet,
                pDNSResponse->wPRs);
    }
    
    if (pDNSResponse->ppUpdateRRSet)
    {
        DNSFreeRecordList(
                pDNSResponse->ppUpdateRRSet,
                pDNSResponse->wUpdates);
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
            

