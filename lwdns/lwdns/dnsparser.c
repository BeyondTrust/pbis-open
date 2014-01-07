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
 *        dnsparser.c
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 *
 *        DNS Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
VOID
DNSFixHeaderForEndianness(
    PDNS_RESPONSE_HEADER pHeader
    );

static
BOOLEAN
DNSIsValidResponse(
    PDNS_RESPONSE_HEADER pHeader
    );

static
DWORD
DNSParseQueryResponse(
    PDNS_RESPONSE_HEADER pHeader,
    PLW_DLINKED_LIST* ppAnswersList,
    PLW_DLINKED_LIST* ppAuthsList,
    PLW_DLINKED_LIST* ppAdditionalsList
    );

static
DWORD
DNSParseName(
    PDNS_RESPONSE_HEADER pHeader,
    PBYTE  pData,
    PDWORD pdwBytesToAdvance,
    PSTR*  ppszName
    );

static
VOID
DNSParseNameWorker(
    PDNS_RESPONSE_HEADER pHeader,
    PBYTE pData,
    PDWORD pdwBytesToAdvance,
    PDWORD pdwNameLen,
    PSTR pszName
    );

static
DWORD
DNSParseRecords(
    PDNS_RESPONSE_HEADER pHeader,
    WORD  wNRecords,
    PBYTE pData,
    PLW_DLINKED_LIST* ppRecordList,
    PDWORD pdwBytesToAdvance
    );

static
DWORD
DNSParseRecord(
    PDNS_RESPONSE_HEADER pHeader,
    PBYTE pData,
    PDNS_RECORD* ppRecord,
    PDWORD pdwBytesToAdvance
    );

static
BYTE
DNSReadBYTE(
    PBYTE pBuffer
    );

static
WORD
DNSReadWORD(
    PBYTE pBuffer
    );

static
DWORD
DNSReadDWORD(
    PBYTE pBuffer
    );

static
VOID
DNSFreeRecordLinkedList(
    PLW_DLINKED_LIST pRecordList
    );

static
VOID
DNSFreeRecordInList(
    PVOID pRecord,
    PVOID pUserData
    );

static
VOID
DNSFreeRecord2(
    PDNS_RECORD pRecord
    );

static
DWORD
DNSBuildNameServerList(
    PCSTR                pszDomain,
    PDNS_RESPONSE_HEADER pHeader,
    PLW_DLINKED_LIST      pAnswers,
    PLW_DLINKED_LIST      pAuths,
    PLW_DLINKED_LIST      pAdditionals,
    PLW_DLINKED_LIST*     ppNameServers
    );

static
DWORD
DNSFindAddressForServer(
    PLW_DLINKED_LIST pAdditionalsList,
    PCSTR  pszHostname,
    PDWORD pdwIP
    );

static
DWORD
DNSBuildNameServerArray(
    PLW_DLINKED_LIST pNameServerList,
    PLW_NS_INFO*    ppNSInfoArray,
    PDWORD          pdwNumInfos);

static
VOID
DNSFreeNameServerList(
    PLW_DLINKED_LIST pNameServerList
    );

static
VOID
DNSFreeNameServerInfoInList(
    PVOID pData,
    PVOID pUserData
    );

DWORD
DNSGetNameServers(
    IN PCSTR pszDomain,
    OUT PSTR* ppszZone,
    OUT PLW_NS_INFO* ppNSInfoList,
    OUT PDWORD pdwNumServers
    )
{
    DWORD dwError = 0;
    const size_t dwBufferSize = (64 * 1024);
    PBYTE pBuffer = NULL;
    int   responseLen = -1;
    PDNS_RESPONSE_HEADER pHeader = NULL;
    PLW_DLINKED_LIST pAnswersList = NULL;
    PLW_DLINKED_LIST pAuthsList = NULL;
    PLW_DLINKED_LIST pAdditionalsList = NULL;
    PLW_DLINKED_LIST pNameServerList = NULL;
    PLW_NS_INFO     pNSInfoArray = NULL;
    DWORD           dwNumInfos = 0;
    PCSTR pszZone = NULL;
    PSTR pszDupZone = NULL;
    
    dwError = DNSAllocateMemory(
                    dwBufferSize,
                    (PVOID*)&pBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    if (res_init() < 0)
    {
        dwError = ERROR_DLL_INIT_FAILED;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    // Use TCP
    _res.options |= RES_USEVC;

    pszZone = pszDomain;

    while (responseLen < 0)
    {
        LWDNS_LOG_DEBUG(
                "Querying DNS for SOA Record for zone [%s]",
                pszZone);

        LWDNS_SAFE_FREE_STRING(pszDupZone);

        dwError = DNSAllocateString(
                        pszZone,
                        &pszDupZone);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        responseLen = res_query(
                        pszDupZone, //this parameter is char* on HP-UX
                        ns_c_in,
                        ns_t_soa,
                        pBuffer,
                        dwBufferSize);
        if (responseLen < 0)
        {
            if (h_errno == HOST_NOT_FOUND || h_errno == NO_DATA)
            {
                LWDNS_LOG_DEBUG("SOA record not found [h_errno:%d]",
                        h_errno);
                // Try looking in the parent zone by jumping past the first
                // component.
                pszZone = strchr(pszZone, '.');
                if (pszZone == NULL || pszZone[1] == 0)
                {
                    LWDNS_LOG_ERROR("No more zones to check");
                    dwError = DNS_ERROR_ZONE_HAS_NO_SOA_RECORD;
                    BAIL_ON_LWDNS_ERROR(dwError);
                }
                // Skip the .
                pszZone++;
                continue;
            }
            LWDNS_LOG_ERROR(
                    "DNS Query for Name Servers failed [errno:%d] [h_errno:%d]",
                    errno,
                    h_errno);
            
            dwError = DNSMapHerrno(h_errno);
            BAIL_ON_LWDNS_ERROR(dwError);
        }
    }
    
    if (responseLen > dwBufferSize)
    {
        dwError = ERROR_BAD_NET_RESP;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (responseLen < CT_FIELD_OFFSET(DNS_RESPONSE_HEADER, data))
    {
       dwError = ERROR_BAD_NET_RESP;
       BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    pHeader = (PDNS_RESPONSE_HEADER)pBuffer;

    DNSFixHeaderForEndianness(pHeader);

    if (!DNSIsValidResponse(pHeader))
    {
        dwError = ERROR_BAD_NET_RESP;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    // Decode DNS response w/o taking into account record type
    dwError = DNSParseQueryResponse(
                     pHeader,
                     &pAnswersList,
                     &pAuthsList,
                     &pAdditionalsList);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSBuildNameServerList(
                    pszZone,
                    pHeader,
                    pAnswersList,
                    pAuthsList,
                    pAdditionalsList,
                    &pNameServerList);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSBuildNameServerArray(
                    pNameServerList,
                    &pNSInfoArray,
                    &dwNumInfos);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppNSInfoList = pNSInfoArray;
    *pdwNumServers = dwNumInfos;
    *ppszZone = pszDupZone;
    
cleanup:

    if (pBuffer)
    {
        DNSFreeMemory(pBuffer);
    }

    if (pAnswersList)
    {
        DNSFreeRecordLinkedList(pAnswersList);
    }
    
    if (pAuthsList)
    {
        DNSFreeRecordLinkedList(pAuthsList);
    }
    
    if (pAdditionalsList)
    {
        DNSFreeRecordLinkedList(pAdditionalsList);
    }
    
    if (pNameServerList)
    {
        DNSFreeNameServerList(pNameServerList);
    }

    return dwError;
    
error:

    if (pNSInfoArray)
    {
        DNSFreeNameServerInfoArray(pNSInfoArray, dwNumInfos);
    }

    LWDNS_SAFE_FREE_STRING(pszDupZone);

    *ppNSInfoList = NULL;
    *pdwNumServers = 0;
    *ppszZone = NULL;
    
    LWDNS_LOG_ERROR("Failed to find NS Records for domain [%s]. Error code: %d",
                    IsNullOrEmptyString(pszDomain) ? "" : pszDomain,
                    dwError);

    goto cleanup;
}

static
VOID
DNSFixHeaderForEndianness(
    PDNS_RESPONSE_HEADER pHeader
    )
{
    pHeader->wId = ntohs(pHeader->wId);
    pHeader->flags.W = ntohs(pHeader->flags.W);
    pHeader->wQuestions = ntohs(pHeader->wQuestions);
    pHeader->wAnswers = ntohs(pHeader->wAnswers);
    pHeader->wAuths = ntohs(pHeader->wAuths);
    pHeader->wAdditionals = ntohs(pHeader->wAdditionals);
}

static
BOOLEAN
DNSIsValidResponse(
    PDNS_RESPONSE_HEADER pHeader
    )
{
    return (pHeader &&
            pHeader->flags.B.qr_message_type == 1 /* Response */ &&
            pHeader->flags.B.opcode == 0 /* Std Query */ &&
            pHeader->flags.B.reply_code == 0 /* No error */);
}

static
DWORD
DNSParseQueryResponse(
    PDNS_RESPONSE_HEADER pHeader,
    PLW_DLINKED_LIST* ppAnswersList,
    PLW_DLINKED_LIST* ppAuthsList,
    PLW_DLINKED_LIST* ppAdditionalsList
    )
{
    DWORD dwError = 0;
    PBYTE pData = &pHeader->data[0];
    PLW_DLINKED_LIST pAnswersList = NULL;
    PLW_DLINKED_LIST pAuthsList = NULL;
    PLW_DLINKED_LIST pAdditionalsList = NULL;
    WORD iQuestion = 0;

    if (!pData)
    {
        goto cleanup;
    }

    // Skip question section
    for (; iQuestion < pHeader->wQuestions; iQuestion++)
    {
        DWORD dwNameLen = 0;
            
        dwError = DNSParseName(
                    pHeader,
                    pData,
                    &dwNameLen,
                    NULL);
        BAIL_ON_LWDNS_ERROR(dwError);

        pData += dwNameLen;
        pData += sizeof(WORD); // Type
        pData += sizeof(WORD); // Class
    }

    if ( pHeader->wAnswers )
    {
        DWORD dwAnswersLen = 0;
        
        dwError = DNSParseRecords(
                        pHeader,
                        pHeader->wAnswers,
                        pData,
                        &pAnswersList,
                        &dwAnswersLen);
        BAIL_ON_LWDNS_ERROR(dwError);

        pData += dwAnswersLen;
    }

    if ( pHeader->wAuths )
    {
        DWORD dwAuthsLen = 0;
        
        dwError = DNSParseRecords(
                        pHeader,
                        pHeader->wAuths,
                        pData,
                        &pAuthsList,
                        &dwAuthsLen);
        BAIL_ON_LWDNS_ERROR(dwError);

        pData += dwAuthsLen;
    }

    if ( pHeader->wAdditionals )
    {
        DWORD dwAdditionalsLen = 0;
        
        dwError = DNSParseRecords(
                        pHeader,
                        pHeader->wAdditionals,
                        pData,
                        &pAdditionalsList,
                        &dwAdditionalsLen);
        BAIL_ON_LWDNS_ERROR(dwError);

        pData += dwAdditionalsLen;
    }
    
    *ppAnswersList = pAnswersList;
    *ppAuthsList = pAuthsList;
    *ppAdditionalsList = pAdditionalsList;
    
cleanup:

    return dwError;
    
error:

    if (pAnswersList)
    {
        DNSFreeRecordLinkedList(pAnswersList);
    }
    
    if (pAuthsList)
    {
        DNSFreeRecordLinkedList(pAuthsList);
    }
   
    if (pAdditionalsList)
    {
        DNSFreeRecordLinkedList(pAdditionalsList);
    }

    goto cleanup;
}

static
DWORD
DNSParseName(
    PDNS_RESPONSE_HEADER pHeader,
    PBYTE  pData,
    PDWORD pdwBytesToAdvance,
    PSTR*  ppszName
    )
{
    DWORD dwError = 0;
    DWORD dwBytesToAdvance = 0;
    DWORD dwNameLen = 0;
    PSTR pszName = NULL;

    /* Figure out the size and how many bytes
     * the parse will advance
     */
    DNSParseNameWorker(
        pHeader, 
        pData, 
        &dwBytesToAdvance, 
        &dwNameLen, 
        NULL);

    if (ppszName)
    {
        /* Now allocate the memory, overallocating to
         * ensure NULL termination in case
         * the DNS packet does not termiante
         */
        dwError = DNSAllocateMemory(
                    (dwNameLen+3) * sizeof(CHAR),
                    (PVOID*)&pszName);
        BAIL_ON_LWDNS_ERROR(dwError);

        /* Fill in the name */
        DNSParseNameWorker(
               pHeader,
               pData,
               NULL,
               NULL,
               pszName);

        /* Ensure NULL termination */
        pszName[dwNameLen] = 0;
    }
    
    if (ppszName)
    {
        *ppszName = pszName;
        pszName = NULL;
    }
    
    *pdwBytesToAdvance = dwBytesToAdvance;
    
cleanup:

    if (pszName)
    {
        DNSFreeMemory(pszName);
    }

    return dwError;
    
error:
    
    *pdwBytesToAdvance = 0;
    
    if (ppszName)
    {
        *ppszName = NULL;
    }

    goto cleanup;
}

static
VOID
DNSParseNameWorker(
    PDNS_RESPONSE_HEADER pHeader,
    PBYTE pData,
    PDWORD pdwBytesToAdvance,
    PDWORD pdwNameLen,
    PSTR pszName
    )
{
    PBYTE pCurrent = pData;
    DWORD dwBytesToAdvance = 0;
    BOOLEAN bDone = FALSE;
    DWORD dwNameLen = 0;
    DWORD dwIndex = 0;

    /* Figure out the size and how many bytes the parse will advance */
    for (;;)
    {
        BYTE length = DNSReadBYTE(pCurrent);
        if (!bDone)
        {
            dwBytesToAdvance += sizeof(BYTE);
        }
        
        if (0 == length)
        {
            bDone = TRUE;
            break;
        }
        /* TODO: Verify on big and little endian */
        /* Check whether this is a "pointer" */
        else if (length & 0xC0)
        {
            /* A "pointer" is a 16-bit offset minus the mask above */
            WORD wOffset = DNSReadWORD(pCurrent) & 0x3FFF;
            if (!bDone)
            {
                dwBytesToAdvance += sizeof(BYTE);
            }
            pCurrent = ((PBYTE)pHeader) + wOffset;
            bDone = TRUE;
        }
        else
        {
            if (!bDone)
            {
                dwBytesToAdvance += length;
            }
            if (pszName)
            {
                /* Need to add prefix dot for components past the first */
                if (dwIndex > 0)
                {
                    pszName[dwIndex] = '.';
                    dwIndex++;
                }
                memcpy(&pszName[dwIndex], pCurrent + sizeof(BYTE), length);
                dwIndex += length;
            }
            /* need to advance past the length and the  characters */
            pCurrent += sizeof(BYTE) + length;
            /* Need to add space for prefix dot for components past the first */
            if (dwNameLen > 0)
            {
                dwNameLen += 1;
            }
            dwNameLen += length;
        }
    }

    if (pdwNameLen)
    {
        *pdwNameLen = dwNameLen;
    }
    
    if (pdwBytesToAdvance)
    {
        *pdwBytesToAdvance = dwBytesToAdvance;
    }
}

static
DWORD
DNSParseRecords(
    PDNS_RESPONSE_HEADER pHeader,
    WORD  wNRecords,
    PBYTE pData,
    PLW_DLINKED_LIST* ppRecordList,
    PDWORD pdwBytesToAdvance
    )
{
    DWORD dwError = 0;
    PBYTE pCurrent = pData;
    PLW_DLINKED_LIST pRecordList = NULL;
    PDNS_RECORD pRecord = NULL;
    DWORD dwBytesToAdvance = 0;
    WORD iRecord = 0;

    for (iRecord = 0; iRecord < wNRecords; iRecord++)
    {
        DWORD dwLen = 0;
        
        dwError = DNSParseRecord(pHeader, pCurrent, &pRecord, &dwLen);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = LwDLinkedListAppend(&pRecordList, pRecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        pRecord = NULL;

        pCurrent += dwLen;
        dwBytesToAdvance += dwLen;
    }
    
    *ppRecordList = pRecordList;
    *pdwBytesToAdvance = dwBytesToAdvance;

cleanup:

    return dwError;
    
error:

    if (pRecord)
    {
        DNSFreeRecord2(pRecord);
    }
    
    if (pRecordList)
    {
        DNSFreeRecordLinkedList(pRecordList);
    }
    dwBytesToAdvance = 0;

    goto cleanup;
}

static
DWORD
DNSParseRecord(
    PDNS_RESPONSE_HEADER pHeader,
    PBYTE pData,
    PDNS_RECORD* ppRecord,
    PDWORD pdwBytesToAdvance
    )
{
    DWORD dwError = 0;
    PBYTE pCurrent = pData;
    PSTR  pszName = NULL;
    PDNS_RECORD pRecord = NULL;
    DWORD dwBytesToAdvance = 0;
    DWORD dwNameBytesToAdvance = 0;
    WORD  wDataLen = 0; /* As read from DNS record response data */

    dwError = DNSParseName(
                pHeader,
                pCurrent,
                &dwNameBytesToAdvance,
                &pszName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwBytesToAdvance += dwNameBytesToAdvance;
    dwBytesToAdvance += sizeof(WORD); /* Type  */
    dwBytesToAdvance += sizeof(WORD); /* Class */
    dwBytesToAdvance += sizeof(DWORD); /* TTL   */
    pCurrent += dwBytesToAdvance;

    wDataLen = DNSReadWORD( pCurrent );
    dwBytesToAdvance += wDataLen + sizeof(WORD);

    dwError = DNSAllocateMemory(
                sizeof(DNS_RECORD) + wDataLen,
                (PVOID*)&pRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    // Fill in new record from buffer
    pCurrent = pData;
    pCurrent += dwNameBytesToAdvance;

    pRecord->pszName = pszName;
    pszName = NULL;

    pRecord->wType = DNSReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    pRecord->wClass = DNSReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    pRecord->dwTTL = DNSReadDWORD(pCurrent);
    pCurrent += sizeof(DWORD);

    pRecord->wDataLen = DNSReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    pRecord->pData = (PBYTE)pRecord + sizeof(DNS_RECORD);
    memcpy(pRecord->pData, pCurrent, pRecord->wDataLen);

    *pdwBytesToAdvance = dwBytesToAdvance;
    *ppRecord = pRecord;
    
cleanup:

    LWDNS_SAFE_FREE_STRING(pszName);
    
    return dwError;
    
error:

    if (pRecord)
    {
        DNSFreeRecord2(pRecord);
    }

    *pdwBytesToAdvance = 0;
    *ppRecord = NULL;

    goto cleanup;
}

static
BYTE
DNSReadBYTE(
    PBYTE pBuffer
    )
{
    BYTE byte = 0;

    if ( pBuffer )
    {
        memcpy( &byte, pBuffer, sizeof(BYTE) );
    }

    return byte;
}

static
WORD
DNSReadWORD(
    PBYTE pBuffer
    )
{
    WORD wVal = 0;

    if ( pBuffer )
    {
        memcpy( &wVal, pBuffer, sizeof(WORD) );
    }

    return ntohs(wVal);
}

static
DWORD
DNSReadDWORD(
    PBYTE pBuffer
    )
{
    DWORD dwVal = 0;
    
    if ( pBuffer )
    {
        memcpy( &dwVal, pBuffer, sizeof(DWORD) );
    }

    return ntohl(dwVal);
}

static
VOID
DNSFreeRecordLinkedList(
    PLW_DLINKED_LIST pRecordList
    )
{
    LwDLinkedListForEach(pRecordList, &DNSFreeRecordInList, NULL);
    LwDLinkedListFree(pRecordList);
}

static
VOID
DNSFreeRecordInList(
    PVOID pRecord,
    PVOID pUserData
    )
{
    PDNS_RECORD pDnsRecord = (PDNS_RECORD)pRecord;
    if (pDnsRecord)
    {
        DNSFreeRecord2(pDnsRecord);
    }
}

static
VOID
DNSFreeRecord2(
    PDNS_RECORD pRecord
    )
{
    LWDNS_SAFE_FREE_STRING(pRecord->pszName);
    DNSFreeMemory(pRecord);
}

static
DWORD
DNSBuildNameServerList(
    PCSTR                pszDomain,
    PDNS_RESPONSE_HEADER pHeader,
    PLW_DLINKED_LIST      pAnswers,
    PLW_DLINKED_LIST      pAuths,
    PLW_DLINKED_LIST      pAdditionals,
    PLW_DLINKED_LIST*     ppNameServers
    )
{
    DWORD dwError = 0;
    PLW_DLINKED_LIST pNameServers = NULL;
    PLW_DLINKED_LIST pIter = NULL;
    PLW_NS_INFO pNSInfo = NULL;
    
    for (pIter = pAnswers; pIter; pIter = pIter->pNext)
    {
        PDNS_RECORD pRecord = NULL;
        
        pRecord = (PDNS_RECORD)pIter->pItem;
        
        if (pRecord->wType == ns_t_soa &&
            pRecord->wDataLen &&
            pRecord->pData)
        {
            DWORD dwNameLen = 0;
            
            dwError = DNSAllocateMemory(
                            sizeof(LW_NS_INFO),
                            (PVOID*)&pNSInfo);
            BAIL_ON_LWDNS_ERROR(dwError);
            
            dwError = DNSParseName(
                        pHeader,
                        pRecord->pData,
                        &dwNameLen,
                        &pNSInfo->pszNSHostName);
            BAIL_ON_LWDNS_ERROR(dwError);
            
            LWDNS_LOG_DEBUG("Adding Name Server [%s]", 
                            IsNullOrEmptyString(pNSInfo->pszNSHostName) ? "" : pNSInfo->pszNSHostName);
            
            dwError = LwDLinkedListAppend(
                        &pNameServers,
                        pNSInfo);
            BAIL_ON_LWDNS_ERROR(dwError);
            
            pNSInfo = NULL;
        }
    }
    
    for (pIter = pNameServers; pIter; pIter = pIter->pNext)
    {
        PLW_NS_INFO pNameServerInfo = NULL;
        
        pNameServerInfo = (PLW_NS_INFO)pIter->pItem;
        
        // Best Effort to find IP
        DNSFindAddressForServer(
                pAdditionals,
                pNameServerInfo->pszNSHostName,
                &pNameServerInfo->dwIP);
    }
    
    *ppNameServers = pNameServers;
    
cleanup:

    return dwError;
    
error:

    if (pNSInfo)
    {
        DNSFreeNameServerInfo(pNSInfo);
    }
    
    if (pNameServers)
    {
        DNSFreeNameServerList(pNameServers);
    }
    
    *ppNameServers = NULL;

    goto cleanup;
}

static
DWORD
DNSFindAddressForServer(
    PLW_DLINKED_LIST pAdditionalsList,
    PCSTR  pszHostname,
    PDWORD pdwIP
    )
{
    DWORD dwError = 0;
    PLW_DLINKED_LIST pListMember = NULL;
    DWORD dwIP = 0;
    
    pListMember = pAdditionalsList;
    while (pListMember)
    {
        PDNS_RECORD pRecord = (PDNS_RECORD)pListMember->pItem;
        
        if ( (pRecord->wType == ns_t_a ) &&
             !strcasecmp(pRecord->pszName, pszHostname) &&
             pRecord->wDataLen == sizeof(DWORD) )
        {
            memcpy(&dwIP, pRecord->pData, sizeof(dwIP));
            break;
        }

        pListMember = pListMember->pNext;
    }
    
    if (dwIP == 0)
    {
        dwError = DNS_ERROR_RECORD_DOES_NOT_EXIST;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    *pdwIP = dwIP;
    
cleanup:

    return dwError;
    
error:

    *pdwIP = 0;

    goto cleanup;
}

static
DWORD
DNSBuildNameServerArray(
    PLW_DLINKED_LIST pNameServerList,
    PLW_NS_INFO*    ppNSInfoArray,
    PDWORD          pdwNumInfos)
{
    DWORD dwError = 0;
    DWORD dwNumInfos = 0;
    PLW_DLINKED_LIST pIter = NULL;
    PLW_NS_INFO pNSInfoArray = NULL;
    DWORD iNS = 0;
    
    for (pIter = pNameServerList; pIter; pIter = pIter->pNext)
    {     
        dwNumInfos++;
    }
    
    if (!dwNumInfos)
    {
        goto done;
    }
    
    dwError = DNSAllocateMemory(
                sizeof(LW_NS_INFO) * dwNumInfos,
                (PVOID*)&pNSInfoArray);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    for (pIter = pNameServerList; pIter; pIter = pIter->pNext, iNS++)
    {
        PLW_NS_INFO pSrcNSInfo = NULL;
        PLW_NS_INFO pDstNSInfo = NULL;
        
        pSrcNSInfo = (PLW_NS_INFO)pIter->pItem;
        pDstNSInfo = &pNSInfoArray[iNS];
        
        memcpy(pDstNSInfo, pSrcNSInfo, sizeof(LW_NS_INFO));
        memset(pSrcNSInfo, 0, sizeof(LW_NS_INFO));
    }
    
done:
    
    *ppNSInfoArray = pNSInfoArray;
    *pdwNumInfos = dwNumInfos;
    
cleanup:

    return dwError;
    
error:

    if (pNSInfoArray)
    {
        DNSFreeNameServerInfoArray(pNSInfoArray, dwNumInfos);
    }

    goto cleanup;
}

static
VOID
DNSFreeNameServerList(
    PLW_DLINKED_LIST pNameServerList
    )
{
    LwDLinkedListForEach(
            pNameServerList,
            &DNSFreeNameServerInfoInList,
            NULL);
    LwDLinkedListFree(pNameServerList);
}

static
VOID
DNSFreeNameServerInfoInList(
    PVOID pData,
    PVOID pUserData
    )
{
    if (pData)
    {
        DNSFreeNameServerInfo((PLW_NS_INFO)pData);
    }
}

VOID
DNSFreeNameServerInfoArray(
    PLW_NS_INFO pNSInfoArray,
    DWORD       dwNumInfos
    )
{
    DWORD iInfo = 0;
    
    for (; iInfo < dwNumInfos; iInfo++)
    {
        DNSFreeNameServerInfoContents(
                &pNSInfoArray[iInfo]);
    }
    DNSFreeMemory(pNSInfoArray);
}
    
VOID
DNSFreeNameServerInfo(
    PLW_NS_INFO pNSInfo
    )
{
    DNSFreeNameServerInfoContents(pNSInfo);
    
    DNSFreeMemory(pNSInfo);
}

VOID
DNSFreeNameServerInfoContents(
    PLW_NS_INFO pNSInfo
    )
{
    LWDNS_SAFE_FREE_STRING(pNSInfo->pszNSHostName);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
