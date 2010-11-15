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
 *        dnsrecord.c
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
DNSCreateDeleteRecord(
    PCSTR pszHost,
    WORD  wClass,
    WORD  wType,
    PDNS_RR_RECORD * ppDNSRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(
                pszHost,
                &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                sizeof(DNS_RR_RECORD),
                (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.dwTTL = 0;
    pDNSRRRecord->RRHeader.wClass = wClass;
    pDNSRRRecord->RRHeader.wType = wType;
    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wRDataSize = 0;

    *ppDNSRecord = pDNSRRRecord;
    
cleanup:

    return dwError;
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSRRRecord) {
        DNSFreeRecord(pDNSRRRecord);
    }

    *ppDNSRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreatePtrRecord(
    PCSTR pszName,
    WORD  wClass,
    PCSTR pszDest,
    PDNS_RR_RECORD * ppDNSRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(
                    pszName,
                    &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.dwTTL = DNS_ONE_HOUR_IN_SECS;
    pDNSRRRecord->RRHeader.wClass = wClass;
    pDNSRRRecord->RRHeader.wType = QTYPE_PTR;
    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wRDataSize = 0;

    dwError = DNSDomainNameFromString(
                    pszDest,
                    &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);
    pDNSRRRecord->pRDataDomain = pDomainName;
    pDomainName = NULL;

    *ppDNSRecord = pDNSRRRecord;
    
cleanup:
    if (pDomainName)
    {
        DNSFreeDomainName(pDomainName);
    }

    return dwError;

error:
    if (pDNSRRRecord)
    {
        DNSFreeRecord(pDNSRRRecord);
    }

    *ppDNSRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateARecord(
    PCSTR pszHost,
    WORD  wClass,
    WORD  wType,
    DWORD dwIP,
    PDNS_RR_RECORD * ppDNSRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;
    PBYTE pRData = NULL;
    DWORD dwnIP = 0;

    dwError = DNSDomainNameFromString(
                    pszHost,
                    &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.dwTTL = DNS_ONE_HOUR_IN_SECS;
    pDNSRRRecord->RRHeader.wClass = wClass;
    pDNSRRRecord->RRHeader.wType = wType;
    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wRDataSize = sizeof(DWORD);

    dwError = DNSAllocateMemory(
                    sizeof(DWORD),
                    (PVOID *)&pRData);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwnIP = htonl(dwIP);
    memcpy(pRData, &dwnIP, sizeof(DWORD));
    pDNSRRRecord->pRData = pRData;
    pRData = NULL;

    *ppDNSRecord = pDNSRRRecord;
    
cleanup:

    return dwError;

error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSRRRecord) {
        DNSFreeRecord(pDNSRRRecord);
    }

    if (pRData) {
        DNSFreeMemory(pRData);
    }
    
    *ppDNSRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateTKeyRecord(
    PCSTR pszKeyName,
    PBYTE pKeyData,
    WORD  wKeySize,
    PDNS_RR_RECORD * ppDNSRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRecord = NULL;
    PDNS_DOMAIN_NAME pAlgorithmName = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;
    time_t t;

    DWORD dwRDataSize =  0;
    DWORD dwnInception, dwInception = 0;
    DWORD dwnExpiration, dwExpiration = 0;
    WORD wnMode, wMode = 0;
    WORD wnError, wError = 0;
    WORD wnKeySize = 0;
    WORD wnOtherSize, wOtherSize = 0;

    DWORD dwAlgorithmLen = 0;
    DWORD dwCopied = 0;
    DWORD dwOffset = 0;

    PBYTE pRData = NULL;

    dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSDomainNameFromString(
                    pszKeyName,
                    &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSDomainNameFromString(
                    DNS_GSS_ALGORITHM,
                    &pAlgorithmName);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRecord->RRHeader.dwTTL = 0;
    pDNSRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRecord->RRHeader.wClass = DNS_CLASS_ANY;
    pDNSRecord->RRHeader.wType = QTYPE_TKEY;

    time(&t);
    dwExpiration = (DWORD)t + DNS_ONE_DAY_IN_SECS;
    dwInception = (DWORD)t;
    wError = 0;
    wMode = 3;

    dwError = DNSGetDomainNameLength(pAlgorithmName, &dwAlgorithmLen);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwRDataSize =  dwAlgorithmLen +
                    sizeof(dwExpiration) +
                    sizeof(dwInception) +
                    sizeof(wError) +
                    sizeof(wMode) +
                    sizeof(wError) +
                    sizeof(wKeySize) +
                    wKeySize +
                    sizeof(wOtherSize) +
                    wOtherSize;

    dwError = DNSAllocateMemory(
                    dwRDataSize,
                    (PVOID *)&pRData);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwnInception = htonl(dwInception);
    dwnExpiration = htonl(dwExpiration);
    wnMode = htons(wMode);
    wnError = htons(wError);
    wnKeySize = htons(wKeySize);
    wnOtherSize = htons(wOtherSize);

    dwError = DNSCopyDomainName(pRData, pAlgorithmName, &dwCopied);
    BAIL_ON_LWDNS_ERROR(dwError);
    dwOffset += dwCopied;

    memcpy(pRData + dwOffset, &dwnInception, sizeof(DWORD));
    dwOffset += sizeof(DWORD);

    memcpy(pRData + dwOffset, &dwnExpiration, sizeof(DWORD));
    dwOffset += sizeof(DWORD);

    memcpy(pRData + dwOffset, &wnMode, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, &wnError, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, &wnKeySize, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, pKeyData, wKeySize);
    dwOffset += wKeySize;

    memcpy(pRData + dwOffset, &wnOtherSize, sizeof(WORD));
    dwOffset += sizeof(WORD);

    pDNSRecord->RRHeader.wRDataSize = (WORD)dwRDataSize;

    pDNSRecord->pRData = pRData;
    pRData = NULL;
    
    *ppDNSRecord = pDNSRecord;

cleanup:

    if (pAlgorithmName) {
        DNSFreeDomainName(pAlgorithmName);
    }

    return dwError;

error:

    if (pDNSRecord) {
        DNSFreeMemory(pDNSRecord);
    }

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pRData)
    {
        DNSFreeMemory(pRData);
    }

    *ppDNSRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateTSIGRecord(
    PCSTR pszKeyName,
    DWORD dwTimeSigned,
    WORD  wFudge,
    WORD  wOriginalID,
    PBYTE pMac,
    WORD  wMacSize,
    PDNS_RR_RECORD * ppDNSRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRecord = NULL;
    PDNS_DOMAIN_NAME pAlgorithmName = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;
    DWORD dwRDataSize =  0;
    WORD  wnFudge = 0;
    WORD  wnError = 0, wError = 0;
    WORD  wnMacSize = 0;
    WORD  wnOriginalID = 0;
    WORD  wnOtherLen = 0, wOtherLen = 0;
    DWORD dwAlgorithmLen = 0;
    DWORD dwCopied = 0;
    DWORD dwOffset = 0;
    PBYTE pRData = NULL;
    DWORD dwnTimeSigned = 0;
    WORD  wnTimePrefix = 0;
    WORD  wTimePrefix = 0;

    dwError = DNSDomainNameFromString(DNS_GSS_ALGORITHM, &pAlgorithmName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSGetDomainNameLength(pAlgorithmName, &dwAlgorithmLen);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwRDataSize =  dwAlgorithmLen +
                   sizeof(wTimePrefix) + sizeof(dwTimeSigned) +
                   sizeof(wFudge) +
                   sizeof(wMacSize)+ wMacSize +
                   sizeof(wOriginalID) +
                   sizeof(wError) +
                   sizeof(wOtherLen);

    dwError = DNSAllocateMemory(dwRDataSize, (PVOID *)&pRData);
    BAIL_ON_LWDNS_ERROR(dwError);

    wnTimePrefix  = htons(wTimePrefix);
    dwnTimeSigned  = htonl(dwTimeSigned);
    wnFudge = htons(wFudge);
    wnMacSize = htons(wMacSize);
    wnOriginalID = htons(wOriginalID);
    wnError = htons(wError);
    wnOtherLen = htons(wOtherLen);

    dwError = DNSCopyDomainName(pRData, pAlgorithmName, &dwCopied);
    BAIL_ON_LWDNS_ERROR(dwError);
    dwOffset += dwCopied;

    memcpy(pRData + dwOffset, &wnTimePrefix, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, &dwnTimeSigned, sizeof(DWORD));
    dwOffset += sizeof(DWORD);

    memcpy(pRData + dwOffset, &wnFudge, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, &wnMacSize, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, pMac, wMacSize);
    dwOffset += wMacSize;

    memcpy(pRData + dwOffset, &wnOriginalID, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, &wnError, sizeof(WORD));
    dwOffset += sizeof(WORD);

    memcpy(pRData + dwOffset, &wnOtherLen, sizeof(WORD));
    dwOffset += sizeof(WORD);

    dwError = DNSAllocateMemory(
                sizeof(DNS_RR_RECORD),
                (PVOID *)&pDNSRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSDomainNameFromString(pszKeyName, &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRecord->RRHeader.dwTTL = 0;
    pDNSRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRecord->RRHeader.wClass = DNS_CLASS_ANY;
    pDNSRecord->RRHeader.wType = QTYPE_TSIG;
    pDNSRecord->RRHeader.wRDataSize = (WORD)dwRDataSize;
    pDNSRecord->pRData = pRData;
    pRData = NULL;

    *ppDNSRecord = pDNSRecord;

cleanup:

    if (pAlgorithmName) {
        DNSFreeDomainName(pAlgorithmName);
    }

    return(dwError);

error:

    if (pDNSRecord) {
        DNSFreeMemory(pDNSRecord);
    }

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pRData)
    {
        DNSFreeMemory(pRData);
    }

    *ppDNSRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateQuestionRecord(
    PCSTR pszQName,
    WORD  wQType,
    WORD  wQClass,
    PDNS_QUESTION_RECORD * ppDNSQuestionRecord
    )
{
    DWORD dwError = 0;
    PDNS_QUESTION_RECORD pDNSQuestionRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(
                    pszQName, 
                    &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                sizeof(DNS_QUESTION_RECORD),
                (PVOID *)&pDNSQuestionRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSQuestionRecord->pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSQuestionRecord->wQueryClass = wQClass;
    pDNSQuestionRecord->wQueryType = wQType;

    *ppDNSQuestionRecord = pDNSQuestionRecord;

cleanup:

    return(dwError);
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSQuestionRecord) {
        DNSFreeQuestionRecord(pDNSQuestionRecord);
    }
    
    *ppDNSQuestionRecord = NULL;
    
    goto cleanup;
}

VOID
DNSFreeQuestionRecordList(
    PDNS_QUESTION_RECORD* ppRecordList,
    DWORD           dwNumRecords
    )
{
    DWORD iRecord = 0;
    
    for (; iRecord < dwNumRecords; iRecord++)
    {
        PDNS_QUESTION_RECORD pRecord =
            *(ppRecordList + iRecord);
        
        if (pRecord)
        {
            DNSFreeQuestionRecord(pRecord);
        }
    }
    
    DNSFreeMemory(ppRecordList);
}

VOID
DNSFreeQuestionRecord(
    PDNS_QUESTION_RECORD pRecord
    )
{
    if (pRecord->pDomainName)
    {
        DNSFreeDomainName(pRecord->pDomainName);
    }
    DNSFreeMemory(pRecord);
}

DWORD
DNSCreateZoneRecord(
    PCSTR pszZName,
    PDNS_ZONE_RECORD * ppDNSZoneRecord
    )
{
    DWORD dwError = 0;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(pszZName, &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                sizeof(DNS_ZONE_RECORD),
                (PVOID *)&pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSZoneRecord->pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSZoneRecord->wZoneClass = DNS_CLASS_IN;
    pDNSZoneRecord->wZoneType = QTYPE_SOA;

    *ppDNSZoneRecord = pDNSZoneRecord;
    
cleanup:

    return dwError;
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSZoneRecord) {
        DNSFreeZoneRecord(pDNSZoneRecord);
    }
    
    *ppDNSZoneRecord = NULL;
    
    goto cleanup;
}

VOID
DNSFreeZoneRecordList(
    PDNS_ZONE_RECORD* ppRecordList,
    DWORD dwNumRecords
    )
{
    DWORD iRecord = 0;
    
    for (; iRecord < dwNumRecords; iRecord++)
    {
        PDNS_ZONE_RECORD pRecord =
            *(ppRecordList + iRecord);
        
        if (pRecord)
        {
            DNSFreeZoneRecord(pRecord);
        }
    }
    
    DNSFreeMemory(ppRecordList);
}

VOID
DNSFreeZoneRecord(
    PDNS_ZONE_RECORD pRecord
    )
{
    if (pRecord->pDomainName)
    {
        DNSFreeDomainName(pRecord->pDomainName);
    }
    DNSFreeMemory(pRecord);
}

DWORD
DNSCreateNameInUseRecord(
    PCSTR pszName,
    PDNS_RR_RECORD * ppDNSRRRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(pszName, &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wClass =  DNS_CLASS_ANY;
    pDNSRRRecord->RRHeader.wType =  QTYPE_ANY;
    pDNSRRRecord->RRHeader.dwTTL =  0;
    pDNSRRRecord->RRHeader.wRDataSize = 0;

    *ppDNSRRRecord = pDNSRRRecord;
    
cleanup:

    return dwError;
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSRRRecord) {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    *ppDNSRRRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateNameNotInUseRecord(
    PCSTR            pszName,
    PDNS_RR_RECORD * ppDNSRRRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(pszName, &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                sizeof(DNS_RR_RECORD),
                (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wClass =  DNS_CLASS_NONE;
    pDNSRRRecord->RRHeader.wType =  QTYPE_CNAME;
    pDNSRRRecord->RRHeader.dwTTL =  0;
    pDNSRRRecord->RRHeader.wRDataSize = 0;

    *ppDNSRRRecord = pDNSRRRecord;
    
cleanup:

    return dwError;
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSRRRecord) {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    *ppDNSRRRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateRRSetExistsVIRecord(
    PCSTR pszName,
    WORD  wType,
    PDNS_RR_RECORD * ppDNSRRRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(pszName, &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                sizeof(DNS_RR_RECORD),
                (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wClass =  DNS_CLASS_NONE;
    pDNSRRRecord->RRHeader.wType =  wType;
    pDNSRRRecord->RRHeader.dwTTL =  0;
    pDNSRRRecord->RRHeader.wRDataSize = 0;

    *ppDNSRRRecord = pDNSRRRecord;

cleanup:

    return dwError;
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSRRRecord) {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    *ppDNSRRRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateRRSetExistsVDRecord(
    PCSTR pszName,
    WORD  wType,
    PDNS_RR_RECORD * ppDNSRRRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(pszName, &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                sizeof(DNS_RR_RECORD),
                (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wClass =  DNS_CLASS_IN;
    pDNSRRRecord->RRHeader.wType =  wType;
    pDNSRRRecord->RRHeader.dwTTL =  0;
    pDNSRRRecord->RRHeader.wRDataSize = 0;

    *ppDNSRRRecord = pDNSRRRecord;

cleanup:

    return(dwError);
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSRRRecord) {
        DNSFreeRecord(pDNSRRRecord);
    }
    
    *ppDNSRRRecord = NULL;
    
    goto cleanup;
}

DWORD
DNSCreateRRSetNotExistsRecord(
    PCSTR pszName,
    WORD  wType,
    PDNS_RR_RECORD * ppDNSRRRecord
    )
{
    DWORD dwError = 0;
    PDNS_RR_RECORD pDNSRRRecord = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;

    dwError = DNSDomainNameFromString(
                    pszName,
                    &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    sizeof(DNS_RR_RECORD),
                    (PVOID *)&pDNSRRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSRRRecord->RRHeader.pDomainName = pDomainName;
    pDomainName = NULL;
    pDNSRRRecord->RRHeader.wClass =  DNS_CLASS_NONE;
    pDNSRRRecord->RRHeader.wType =  wType;
    pDNSRRRecord->RRHeader.dwTTL =  0;
    pDNSRRRecord->RRHeader.wRDataSize = 0;

    *ppDNSRRRecord = pDNSRRRecord;

cleanup:
    
    return dwError;
    
error:

    if (pDomainName) {
        DNSFreeDomainName(pDomainName);
    }
    
    if (pDNSRRRecord) {
        DNSFreeMemory(pDNSRRRecord);
    }
    
    *ppDNSRRRecord = NULL;
    
    goto cleanup;
}

VOID
DNSFreeRecordList(
    PDNS_RR_RECORD* ppRecordList,
    DWORD           dwNumRecords
    )
{
    DWORD iRecord = 0;
    
    for (; iRecord < dwNumRecords; iRecord++)
    {
        PDNS_RR_RECORD pRecord =
            *(ppRecordList + iRecord);
        
        if (pRecord)
        {
            DNSFreeRecord(pRecord);
        }
    }
    
    DNSFreeMemory(ppRecordList);
}

VOID
DNSFreeRecord(
    PDNS_RR_RECORD pRecord
    )
{
    if (pRecord->pRData)
    {
        DNSFreeMemory(pRecord->pRData);
    }

    if (pRecord->pRDataDomain)
    {
        DNSFreeDomainName(pRecord->pRDataDomain);
    }
    
    if (pRecord->RRHeader.pDomainName)
    {
        DNSFreeDomainName(pRecord->RRHeader.pDomainName);
    }
    
    DNSFreeMemory(pRecord);
}
