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
 *        dnsuprequest.c
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
DNSUpdateMarshallZoneSection(    
    HANDLE hSendBuffer,
    PDNS_ZONE_RECORD * ppDNSZoneRecords,
    WORD wZones
    );

static
DWORD
DNSUpdateMarshallPRSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSPRRRRecords,
    WORD   wPRs
    );

static
DWORD
DNSUpdateMarshallUpdateSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSUpdateRRRecords,
    WORD wZones
    );

static
DWORD 
DNSUpdateMarshallAdditionalSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAdditionalsRRRecords,
    WORD wAdditionals
    );

DWORD
DNSUpdateCreateUpdateRequest(
    PDNS_UPDATE_REQUEST * ppDNSRequest
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST pDNSRequest = NULL;
    
    dwError = DNSAllocateMemory(
                sizeof(DNS_UPDATE_REQUEST),
                (PVOID *)&pDNSRequest);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSGenerateIdentifier(&pDNSRequest->wIdentification);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRequest->wParameter = 0x2800;

    *ppDNSRequest = pDNSRequest;

cleanup:

    return dwError;

error:

    if (pDNSRequest){
        DNSUpdateFreeRequest(pDNSRequest);
    }
    
    *ppDNSRequest = NULL;
    
    goto cleanup;
}

DWORD
DNSUpdateSendUpdateRequest2(
    HANDLE hDNSServer,
    PDNS_UPDATE_REQUEST pDNSRequest
    )
{
    DWORD dwError = 0;
    DWORD dwBytesSent = 0;
    HANDLE hSendBuffer = (HANDLE)NULL;

    dwError = DNSUpdateBuildRequestMessage(
                pDNSRequest,
                &hSendBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSSendBufferContext(
                hDNSServer,
                hSendBuffer,
                &dwBytesSent);
    BAIL_ON_LWDNS_ERROR(dwError);

cleanup:

    if (hSendBuffer)
    {
        DNSFreeSendBufferContext(hSendBuffer);
    }

    return dwError;
    
error:

    goto cleanup;
}

DWORD
DNSUpdateBuildRequestMessage(
    PDNS_UPDATE_REQUEST pDNSRequest,
    HANDLE * phSendBuffer
    )
{    
    DWORD dwError = 0;
    CHAR  header[12];
    WORD  wnIdentification = 0;
    WORD  wnParameter = 0;
    WORD  wnZones = 0;
    WORD  wnPRs = 0;
    WORD  wnUpdates = 0;
    WORD  wnAdditionals = 0;
    DWORD dwRead = 0;
    HANDLE hSendBuffer = (HANDLE)NULL;

    dwError = DNSCreateSendBuffer(&hSendBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    wnIdentification = htons(pDNSRequest->wIdentification);
    memcpy(header, (char *)&wnIdentification, 2);

    wnParameter = htons(pDNSRequest->wParameter);
    memcpy(&header[2], (char *)&wnParameter, sizeof(wnParameter));

    wnZones = htons(pDNSRequest->wZones);
    memcpy(&header[4], (char *)&wnZones, sizeof(wnZones));

    wnPRs = htons(pDNSRequest->wPRs);
    memcpy(&header[6], (char *)&wnPRs, sizeof(wnPRs));

    wnUpdates = htons(pDNSRequest->wUpdates);
    memcpy(&header[8], (char *)&wnUpdates, sizeof(wnUpdates));

    wnAdditionals = htons(pDNSRequest->wAdditionals);
    memcpy(&header[10], (char *)&wnAdditionals, sizeof(wnAdditionals));

    dwError = DNSMarshallBuffer(
                hSendBuffer, 
                (PBYTE)header,
                sizeof(header), 
                &dwRead);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (pDNSRequest->wZones) {
        dwError = DNSUpdateMarshallZoneSection(
                    hSendBuffer, 
                    pDNSRequest->ppZoneRRSet,
                    pDNSRequest->wZones);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (pDNSRequest->wPRs) {
        dwError = DNSUpdateMarshallPRSection(
                    hSendBuffer, 
                    pDNSRequest->ppPRRRSet, 
                    pDNSRequest->wPRs);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (pDNSRequest->wUpdates) {
        dwError = DNSUpdateMarshallUpdateSection(
                    hSendBuffer,
                    pDNSRequest->ppUpdateRRSet,
                    pDNSRequest->wUpdates);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (pDNSRequest->wAdditionals) {
        dwError = DNSUpdateMarshallAdditionalSection(
                    hSendBuffer, 
                    pDNSRequest->ppAdditionalRRSet, 
                    pDNSRequest->wAdditionals);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    DNSDumpSendBufferContext(hSendBuffer);

    *phSendBuffer = hSendBuffer;
    
cleanup:

    return dwError;

error:

    if(hSendBuffer) {
        DNSFreeSendBufferContext(hSendBuffer);
    }

    *phSendBuffer = (HANDLE)NULL;
    
    goto cleanup;
}

VOID
DNSUpdateFreeRequest(
    PDNS_UPDATE_REQUEST pDNSRequest
    )
{
    if (pDNSRequest->ppZoneRRSet)
    {
        DNSFreeZoneRecordList(
                pDNSRequest->ppZoneRRSet,
                pDNSRequest->wZones);
    }
    
    if (pDNSRequest->ppPRRRSet)
    {
        DNSFreeRecordList(
                pDNSRequest->ppPRRRSet,
                pDNSRequest->wPRs);
    }
    
    if (pDNSRequest->ppUpdateRRSet)
    {
        DNSFreeRecordList(
                pDNSRequest->ppUpdateRRSet,
                pDNSRequest->wUpdates);
    }
    
    if (pDNSRequest->ppAdditionalRRSet)
    {
        DNSFreeRecordList(
                pDNSRequest->ppAdditionalRRSet,
                pDNSRequest->wAdditionals);
    }
    
    DNSFreeMemory(pDNSRequest);
}

DWORD
DNSUpdateAddZoneSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
    PDNS_ZONE_RECORD pDNSZone
    )
{
    DWORD dwError = 0;
    DWORD dwNumZones = 0;

    dwNumZones = pDNSRequest->wZones;

    dwError = DNSReallocMemory(
                (PBYTE)pDNSRequest->ppZoneRRSet, 
                (PVOID *)&pDNSRequest->ppZoneRRSet, 
                (dwNumZones + 1)*sizeof(PDNS_ZONE_RECORD));
    BAIL_ON_LWDNS_ERROR(dwError);

    *(pDNSRequest->ppZoneRRSet + dwNumZones) = pDNSZone;
        
    pDNSRequest->wZones += 1;
    
error:
    
    return dwError;
}
    
DWORD
DNSUpdateAddAdditionalSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
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
DNSUpdateAddPRSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    )
{    
    DWORD dwError = 0;
    DWORD dwNumPRs = 0;

    dwNumPRs = pDNSRequest->wPRs;
    
    dwError = DNSReallocMemory(
                pDNSRequest->ppPRRRSet,
                (PVOID *)&pDNSRequest->ppPRRRSet,
                (dwNumPRs + 1) *sizeof(PDNS_RR_RECORD));
    BAIL_ON_LWDNS_ERROR(dwError);

    *(pDNSRequest->ppPRRRSet + dwNumPRs) = pDNSRecord;

    pDNSRequest->wPRs += 1;

error:

    return dwError;
}

DWORD
DNSUpdateAddUpdateSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    )
{
    DWORD dwError = 0;
    WORD wNumUpdates = 0;
    
    wNumUpdates = pDNSRequest->wUpdates;
    
    dwError = DNSReallocMemory(
                pDNSRequest->ppUpdateRRSet,
                (PVOID *)&pDNSRequest->ppUpdateRRSet,
                (wNumUpdates + 1) * sizeof(PDNS_RR_RECORD));
    BAIL_ON_LWDNS_ERROR(dwError);

    *(pDNSRequest->ppUpdateRRSet + wNumUpdates) = pDNSRecord;

    pDNSRequest->wUpdates += 1;

error:

    return dwError;
}
  
static
DWORD
DNSUpdateMarshallZoneSection(    
    HANDLE hSendBuffer,
    PDNS_ZONE_RECORD * ppDNSZoneRecords,
    WORD wZones
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    
    for (i = 0; i < wZones; i++)
    {
        PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
        DWORD dwRead = 0;
        WORD wnZoneType = 0;
        WORD wnZoneClass = 0;
        
        pDNSZoneRecord = *(ppDNSZoneRecords + i);
        
        dwError = DNSMarshallDomainName(
                    hSendBuffer, 
                    pDNSZoneRecord->pDomainName);
        BAIL_ON_LWDNS_ERROR(dwError);

        wnZoneType = htons(pDNSZoneRecord->wZoneType);
        dwError = DNSMarshallBuffer(
                    hSendBuffer, 
                    (PBYTE)&wnZoneType, 
                    (DWORD)sizeof(WORD),
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);

        wnZoneClass = htons(pDNSZoneRecord->wZoneClass);
        dwError = DNSMarshallBuffer(
                    hSendBuffer, 
                    (PBYTE)&wnZoneClass, 
                    (DWORD)sizeof(WORD), 
                    &dwRead);
        BAIL_ON_LWDNS_ERROR(dwError);
    }    

error:

    return dwError;    
}

static
DWORD
DNSUpdateMarshallPRSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSPRRRRecords,
    WORD   wPRs
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    for (i = 0; i < wPRs; i++)
    {
        PDNS_RR_RECORD pDNSPRRRRecord = NULL;
        
        pDNSPRRRRecord = *(ppDNSPRRRRecords + i);

        dwError = DNSMarshallRRHeader(
                        hSendBuffer,
                        pDNSPRRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError  = DNSMarshallRData(
                        hSendBuffer,
                        pDNSPRRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
error:

    return dwError;
}

static
DWORD
DNSUpdateMarshallUpdateSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSUpdateRRRecords,
    WORD wZones
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    for (i = 0; i < wZones; i++)
    {
        PDNS_RR_RECORD pDNSUpdateRRRecord = NULL;
        
        pDNSUpdateRRRecord = *(ppDNSUpdateRRRecords + i);

        dwError = DNSMarshallRRHeader(
                        hSendBuffer,
                        pDNSUpdateRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        dwError  = DNSMarshallRData(
                        hSendBuffer, 
                        pDNSUpdateRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);
    } 
    
error:

    return dwError;
}

static
DWORD 
DNSUpdateMarshallAdditionalSection(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD * ppDNSAdditionalsRRRecords,
    WORD wAdditionals
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    for (i = 0; i < wAdditionals; i++)
    {
        PDNS_RR_RECORD pDNSAdditionalRRRecord = NULL;
        
        pDNSAdditionalRRRecord = *(ppDNSAdditionalsRRRecords + i);

        dwError = DNSMarshallRRHeader(
                    hSendBuffer, 
                    pDNSAdditionalRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError  = DNSMarshallRData(
                            hSendBuffer, 
                            pDNSAdditionalRRRecord);
        BAIL_ON_LWDNS_ERROR(dwError);
    }    

error:

    return dwError;
}
