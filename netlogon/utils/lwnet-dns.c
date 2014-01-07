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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-dns.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        DNS Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Ki Anderson (kanderson@likewise.com)
 */
#include "includes.h"
#include <assert.h>
#include <lwdef.h>

#define LWNET_LOCK_RESOLVER_API(bInLock) \
    do { \
        pthread_mutex_lock(&gLwnetResolverLock); \
        bInLock = TRUE; \
    } while (0)

#define LWNET_UNLOCK_RESOLVER_API(bInLock) \
    do { \
        if (bInLock) \
        { \
            pthread_mutex_unlock(&gLwnetResolverLock); \
            bInLock = FALSE; \
        } \
    } while (0)


DWORD
LWNetDnsGetHostInfoEx(
    OUT OPTIONAL PSTR* ppszHostname,
    OUT OPTIONAL PSTR* ppszFqdn,
    OUT OPTIONAL PSTR* ppszDomain
    )
{
    DWORD dwError = 0;
    struct hostent * host = NULL;
    CHAR szBuffer[256] = { 0 };
    PSTR pszDot = NULL;
    PSTR pszFoundFqdn = NULL;
    PSTR pszFoundDomain = NULL;
    PSTR pszHostname = NULL;
    PSTR pszDomain = NULL;
    PSTR pszFqdn = NULL;

    if (!ppszHostname && !ppszFqdn && !ppszDomain)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (gethostname(szBuffer, sizeof(szBuffer)-1) != 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    pszDot = strchr(szBuffer, '.');
    if (pszDot)
    {
        pszDot[0] = '\0';
    }

    if (ppszHostname)
    {
        dwError = LWNetAllocateString(szBuffer, &pszHostname);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (!ppszFqdn && !ppszDomain)
    {
        // done, so bail out
        dwError = 0;
        goto error;
    }

    host = gethostbyname(szBuffer);
    if ( !host )
    {
        dwError = LwMapHErrnoToLwError(h_errno);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    //
    // We look for the first name that looks like an FQDN.  This is
    // the same heuristics used by other software such as Kerberos and
    // Samba.
    //
    pszDot = strchr(host->h_name, '.');
    if (pszDot)
    {
        pszFoundFqdn = host->h_name;
        pszFoundDomain = pszDot + 1;
    }
    else
    {
        int i;
        for (i = 0; host->h_aliases[i]; i++)
        {
            pszDot = strchr(host->h_aliases[i], '.');
            if (pszDot)
            {
                pszFoundFqdn = host->h_aliases[i];
                pszFoundDomain = pszDot + 1;
                break;
            }
        }
    }

    // If we still have nothing, just return the first name, but no Domain part.
    if (!pszFoundFqdn)
    {
        pszFoundFqdn = host->h_name;
    }
    if (pszFoundFqdn && !pszFoundFqdn[0])
    {
        pszFoundFqdn = NULL;
    }
    if (pszFoundDomain && !pszFoundDomain[0])
    {
        pszFoundDomain = NULL;
    }

    if (ppszFqdn)
    {
        if (pszFoundFqdn)
        {
            dwError = LWNetAllocateString(pszFoundFqdn, &pszFqdn);
            BAIL_ON_LWNET_ERROR(dwError);
        }
        else
        {
            dwError = ERROR_NOT_FOUND;
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }

    if (ppszDomain)
    {
        if (pszFoundDomain)
        {
            dwError = LWNetAllocateString(pszFoundDomain, &pszDomain);
            BAIL_ON_LWNET_ERROR(dwError);
        }
        else
        {
            dwError = ERROR_NOT_FOUND;
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_STRING(pszHostname);
        LWNET_SAFE_FREE_STRING(pszFqdn);
        LWNET_SAFE_FREE_STRING(pszDomain);
    }

    if (ppszHostname)
    {
        *ppszHostname = pszHostname;
    }

    if (ppszDomain)
    {
        *ppszDomain = pszDomain;
    }

    if (ppszFqdn)
    {
        *ppszFqdn = pszFqdn;
    }

    return dwError;
}

DWORD
LWNetDnsGetHostInfo(
    OUT OPTIONAL PSTR* ppszHostname,
    OUT OPTIONAL PSTR* ppszDomain
    )
{
    return LWNetDnsGetHostInfoEx(ppszHostname, NULL, ppszDomain);
}

DWORD
LWNetDnsGetNameServerList(
    OUT PSTR** pppszNameServerList,
    OUT PDWORD pdwNumServers
    )
// Call LWNET_SAFE_FREE_STRING_ARRAY on returned server list
{
    DWORD   dwError = 0;
    PSTR*   ppszNameServerList = NULL;
    DWORD   dwNumServers = 0;
    FILE*   fp = NULL;
    BOOLEAN bFileExists = FALSE;
    PCSTR   pszConfigFilePath = "/etc/resolv.conf";
    const   DWORD dwMaxLineLen = 1024;
    CHAR    szBuf[dwMaxLineLen + 1];
    regex_t rx;
    PLW_DLINKED_LIST pNameServerList = NULL;
    PSTR    pszNameServer = NULL;
    
    memset(&rx, 0, sizeof(rx));
    
    if (regcomp(&rx, "^nameserver[[:space:]].*$", REG_EXTENDED) < 0) {
        dwError = ERROR_BAD_FORMAT;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LwCheckFileTypeExists(
                    pszConfigFilePath,
                    LWFILE_REGULAR,
                    &bFileExists);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (!bFileExists) {
        *pppszNameServerList = NULL;
        *pdwNumServers = 0;
        goto cleanup;
    }
    
    if ((fp = fopen(pszConfigFilePath, "r")) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LWNET_ERROR(dwError);
    } 
    
    while (1)
    {
          if (fgets(szBuf, dwMaxLineLen, fp) == NULL) {
             if (!feof(fp)) {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LWNET_ERROR(dwError);
             } else {
                break;
             }
          }
          
          LwStripWhitespace(szBuf, TRUE, TRUE);
          
          if (!LWNetDnsConfigLineIsComment(szBuf) &&
              !regexec(&rx, szBuf, (size_t)0, NULL, 0))
          {
             PSTR pszLocation = NULL;
             PCSTR pszSearchString = "nameserver";
             
             if ((pszLocation = strstr(szBuf, pszSearchString))) {
                pszLocation += strlen(pszSearchString);
                
                if (!IsNullOrEmptyString(pszLocation)) {
                   dwError = LWNetAllocateString(pszLocation, &pszNameServer);
                   BAIL_ON_LWNET_ERROR(dwError);
                }
                
                dwError = LwDLinkedListAppend(
                                    &pNameServerList,
                                    pszNameServer);
                BAIL_ON_LWNET_ERROR(dwError);
                
                pszNameServer = NULL;
                
                dwNumServers++;
             }
          }
    }
    
    if (dwNumServers)
    {
        PLW_DLINKED_LIST pListMember = NULL;
        DWORD iMember = 0;

        dwError = LWNetAllocateMemory(dwNumServers * sizeof(PSTR), (PVOID*)&ppszNameServerList);
        BAIL_ON_LWNET_ERROR(dwError);

        pListMember = pNameServerList;
        while (pListMember)
        {
            ppszNameServerList[iMember++] = (PSTR)pListMember->pItem;
            pListMember->pItem = NULL;
            pListMember = pListMember->pNext;
        }
    }

    *pppszNameServerList = ppszNameServerList;
    *pdwNumServers = dwNumServers;

cleanup:

    regfree(&rx);
    
    LwDLinkedListFree(pNameServerList);
    
    LWNET_SAFE_FREE_STRING(pszNameServer);
    
    if (fp) {
        fclose(fp);
    }

    return dwError;
    
error:

    *pppszNameServerList = NULL;
    *pdwNumServers = 0;
    
    if (ppszNameServerList) {
       LWNetFreeStringArray(ppszNameServerList, dwNumServers);
    }

    goto cleanup;
}

BOOLEAN
LWNetDnsConfigLineIsComment(
    IN PCSTR pszLine
    )
{
    PCSTR pszTmp = pszLine;

    if (IsNullOrEmptyString(pszLine))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int) *pszTmp))
        pszTmp++;

    return *pszTmp == '#' || *pszTmp == '\0';
}

VOID
LWNetDnsFixHeaderForEndianness(
    IN OUT PDNS_RESPONSE_HEADER pHeader
    )
{
    pHeader->wId = ntohs(pHeader->wId);
    pHeader->flags.W = ntohs(pHeader->flags.W);
    pHeader->wQuestions = ntohs(pHeader->wQuestions);
    pHeader->wAnswers = ntohs(pHeader->wAnswers);
    pHeader->wAuths = ntohs(pHeader->wAuths);
    pHeader->wAdditionals = ntohs(pHeader->wAdditionals);
}

BOOLEAN
LWNetDnsIsValidResponse(
    IN PDNS_RESPONSE_HEADER pHeader
    )
{
    return (pHeader &&
            pHeader->flags.B.qr_message_type == 1 /* Response */ &&
            pHeader->flags.B.opcode == 0 /* Std Query */ &&
            pHeader->flags.B.reply_code == 0 /* No error */);
}

BOOLEAN
LWNetDnsIsTruncatedResponse(
    IN PDNS_RESPONSE_HEADER pHeader
    )
{
    return (pHeader &&
            (((WORD)pHeader->flags.B.truncated) != 0));
}

DWORD
LWNetDnsParseQueryResponse(
    IN PDNS_RESPONSE_HEADER pHeader,
    OUT OPTIONAL PLW_DLINKED_LIST* ppAnswersList,
    OUT OPTIONAL PLW_DLINKED_LIST* ppAuthsList,
    OUT OPTIONAL PLW_DLINKED_LIST* ppAdditionalsList
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
    for (iQuestion = 0; iQuestion < pHeader->wQuestions; iQuestion++ )
    {
        DWORD dwNameLen = 0;
            
        dwError = LWNetDnsParseName(pHeader, pData, &dwNameLen, NULL);
        BAIL_ON_LWNET_ERROR(dwError);

        pData += dwNameLen;
        pData += sizeof(WORD); // Type
        pData += sizeof(WORD); // Class
    }

    if ( pHeader->wAnswers )
    {
        DWORD dwAnswersLen = 0;
        
        dwError = LWNetDnsParseRecords(
                        pHeader,
                        pHeader->wAnswers,
                        pData,
                        &pAnswersList,
                        &dwAnswersLen);
        BAIL_ON_LWNET_ERROR(dwError);

        pData += dwAnswersLen;
    }

    if ( pHeader->wAuths )
    {
        DWORD dwAuthsLen = 0;
        
        dwError = LWNetDnsParseRecords(
                        pHeader,
                        pHeader->wAuths,
                        pData,
                        &pAuthsList,
                        &dwAuthsLen);
        BAIL_ON_LWNET_ERROR(dwError);

        pData += dwAuthsLen;
    }

    if ( pHeader->wAdditionals )
    {
        DWORD dwAdditionalsLen = 0;
        
        dwError = LWNetDnsParseRecords(
                        pHeader,
                        pHeader->wAdditionals,
                        pData,
                        &pAdditionalsList,
                        &dwAdditionalsLen);
        BAIL_ON_LWNET_ERROR(dwError);

        pData += dwAdditionalsLen;
    }

error:
cleanup:

    if (dwError)
    {
        LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAnswersList);
        LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAuthsList);
        LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAdditionalsList);
    }

    if (ppAnswersList)
    {
       *ppAnswersList = pAnswersList;
    }
    else
    {
        LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAnswersList);
    }

    if (ppAuthsList)
    {
       *ppAuthsList = pAuthsList;
    }
    else
    {
        LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAuthsList);
    }

    if (ppAdditionalsList)
    {
       *ppAdditionalsList = pAdditionalsList;
    }
    else
    {
        LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAdditionalsList);
    }

    return dwError;
}

DWORD
LWNetDnsParseName(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PBYTE pData,
    OUT PDWORD pdwBytesToAdvance,
    OUT OPTIONAL PSTR* ppszName
    )
{
    DWORD dwError = 0;
    DWORD dwBytesToAdvance = 0;
    DWORD dwNameLen = 0;
    PSTR pszName = NULL;

    /* Figure out the size and how many bytes the parse will advance */
    LWNetDnsParseNameWorker(pHeader, pData, &dwBytesToAdvance, &dwNameLen, NULL);

    if (ppszName)
    {
        /* Now allocate the memory, overallocating to ensure NULL termination in case
           the DNS packet does not termiante */
        dwError = LWNetAllocateMemory((dwNameLen+3) * sizeof(CHAR), (PVOID*)&pszName);
        BAIL_ON_LWNET_ERROR(dwError);

        /* Fill in the name */
        LWNetDnsParseNameWorker(pHeader, pData, NULL, NULL, pszName);

        /* Ensure NULL termination */
        pszName[dwNameLen] = 0;
    }
    
error:
    if (dwError)
    {
        LWNET_SAFE_FREE_MEMORY(pszName);
    }

    *pdwBytesToAdvance = dwBytesToAdvance;

    if (ppszName)
    {
        *ppszName = pszName;
    }

    return dwError;
}

VOID
LWNetDnsParseNameWorker(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PBYTE pData,
    OUT OPTIONAL PDWORD pdwBytesToAdvance,
    OUT OPTIONAL PDWORD pdwNameLen,
    OUT OPTIONAL PSTR pszName
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
        BYTE length = LWNetDnsReadBYTE(pCurrent);
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
            WORD wOffset = LWNetDnsReadWORD(pCurrent) & 0x3FFF;
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

DWORD
LWNetDnsParseRecords(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN WORD wNRecords,
    IN PBYTE pData,
    OUT PLW_DLINKED_LIST* ppRecordList,
    OUT PDWORD pdwBytesToAdvance
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
        
        dwError = LWNetDnsParseRecord(pHeader, pCurrent, &pRecord, &dwLen);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LwDLinkedListAppend(&pRecordList, pRecord);
        BAIL_ON_LWNET_ERROR(dwError);

        pRecord = NULL;

        pCurrent += dwLen;
        dwBytesToAdvance += dwLen;
    }

error:    
    if (dwError)
    {
        if (pRecord)
        {
            LWNetDnsFreeRecord(pRecord);
        }
        LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pRecordList);
        dwBytesToAdvance = 0;
    }

    *ppRecordList = pRecordList;
    *pdwBytesToAdvance = dwBytesToAdvance;

    return dwError;
}

DWORD
LWNetDnsParseRecord(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PBYTE pData,
    OUT PDNS_RECORD* ppRecord,
    OUT PDWORD pdwBytesToAdvance
    )
{
    DWORD dwError = 0;
    PBYTE pCurrent = pData;
    PSTR  pszName = NULL;
    PDNS_RECORD pRecord = NULL;
    DWORD dwBytesToAdvance = 0;
    DWORD dwNameBytesToAdvance = 0;
    WORD  wDataLen = 0; /* As read from DNS record response data */

    dwError = LWNetDnsParseName(pHeader, pCurrent, &dwNameBytesToAdvance, &pszName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwBytesToAdvance += dwNameBytesToAdvance;
    dwBytesToAdvance += sizeof(WORD); /* Type  */
    dwBytesToAdvance += sizeof(WORD); /* Class */
    dwBytesToAdvance += sizeof(DWORD); /* TTL   */
    pCurrent += dwBytesToAdvance;

    wDataLen = LWNetDnsReadWORD( pCurrent );
    dwBytesToAdvance += wDataLen + sizeof(WORD);

    dwError = LWNetAllocateMemory(sizeof(DNS_RECORD) + wDataLen, (PVOID*)&pRecord);
    BAIL_ON_LWNET_ERROR(dwError);

    // Fill in new record from buffer
    pCurrent = pData;
    pCurrent += dwNameBytesToAdvance;

    pRecord->pszName = pszName;
    pszName = NULL;

    pRecord->wType = LWNetDnsReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    pRecord->wClass = LWNetDnsReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    pRecord->dwTTL = LWNetDnsReadDWORD(pCurrent);
    pCurrent += sizeof(DWORD);

    pRecord->wDataLen = LWNetDnsReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    pRecord->pData = (PBYTE)pRecord + sizeof(DNS_RECORD);
    memcpy(pRecord->pData, pCurrent, pRecord->wDataLen);

error:
    LWNET_SAFE_FREE_STRING(pszName);
    if (dwError)
    {
        if (pRecord)
        {
            LWNetDnsFreeRecord(pRecord);
            pRecord = NULL;
        }
        dwBytesToAdvance = 0;        
    }

    *pdwBytesToAdvance = dwBytesToAdvance;
    *ppRecord = pRecord;

    return dwError;
}

BYTE
LWNetDnsReadBYTE(
    IN PBYTE pBuffer
    )
{
    BYTE byte = 0;

    if ( pBuffer )
    {
        memcpy( &byte, pBuffer, sizeof(BYTE) );
    }

    return byte;
}

WORD
LWNetDnsReadWORD(
    IN PBYTE pBuffer
    )
{
    WORD wVal = 0;

    if ( pBuffer )
    {
        memcpy( &wVal, pBuffer, sizeof(WORD) );
    }

    return ntohs(wVal);
}

DWORD
LWNetDnsReadDWORD(
    IN PBYTE pBuffer
    )
{
    DWORD dwVal = 0;
    
    if ( pBuffer )
    {
        memcpy( &dwVal, pBuffer, sizeof(DWORD) );
    }

    return ntohl(dwVal);
}

VOID
LWNetDnsFreeRecordInList(
    IN OUT PVOID pRecord,
    IN PVOID pUserData
    )
{
    PDNS_RECORD pDnsRecord = (PDNS_RECORD)pRecord;
    if (pDnsRecord)
    {
        LWNetDnsFreeRecord(pDnsRecord);
    }
}

VOID
LWNetDnsFreeRecord(
    IN OUT PDNS_RECORD pRecord
    )
{
    LWNET_SAFE_FREE_STRING(pRecord->pszName);
    LWNetFreeMemory(pRecord);
}

DWORD
LWNetDnsBuildSRVRecordList(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PLW_DLINKED_LIST pAnswersList,
    IN PLW_DLINKED_LIST pAdditionalsList,
    OUT PLW_DLINKED_LIST* ppSRVRecordList
    )
{
    DWORD dwError = 0;
    WORD priority = 0;
    WORD weight = 0;
    WORD port = 0;
    PSTR pszTarget = NULL;
    PDNS_SRV_INFO_RECORD pSrvInfoRecord = NULL;
    PLW_DLINKED_LIST pListMember = NULL;
    PLW_DLINKED_LIST pSRVRecordList = NULL;
    PLW_DLINKED_LIST pAddressList = NULL;
    PLW_DLINKED_LIST pAddressListMember = NULL;

    for (pListMember = pAnswersList;
         pListMember;
         pListMember = pListMember->pNext)
    {
        LWNET_SAFE_FREE_STRING(pszTarget);
        dwError = LWNetDnsParseSrvRecord(pHeader,
                                        (PDNS_RECORD)pListMember->pItem,
                                        &priority,
                                        &weight,
                                        &port,
                                        &pszTarget);
        BAIL_ON_LWNET_ERROR(dwError);

        LWNET_SAFE_FREE_PSTR_LINKED_LIST(pAddressList);
        dwError = LWNetDnsParseOrGetAddressesForServer(
                        pAdditionalsList,
                        pszTarget,
                        &pAddressList);
        if (dwError)
        {
            // Already logged on ERROR_NOT_FOUND
            if (dwError != ERROR_NOT_FOUND)
            {
                LWNET_LOG_ERROR("Failed to build SRV record information");
            }
            // Skip
            dwError = 0;
            continue;
        }

        LW_ASSERT(pAddressList);

        for (pAddressListMember = pAddressList;
             pAddressListMember;
             pAddressListMember = pAddressListMember->pNext)
        {
            PCSTR pszAddress = (PCSTR)pAddressListMember->pItem;

            dwError = LWNetAllocateMemory(sizeof(*pSrvInfoRecord),
                                          OUT_PPVOID(&pSrvInfoRecord));
            BAIL_ON_LWNET_ERROR(dwError);

            pSrvInfoRecord->wPriority = priority;
            pSrvInfoRecord->wWeight = weight;
            pSrvInfoRecord->wPort = port;

            dwError = LwAllocateString(pszTarget, &pSrvInfoRecord->pszTarget);
            BAIL_ON_LWNET_ERROR(dwError);

            dwError = LwAllocateString(pszAddress, &pSrvInfoRecord->pszAddress);
            BAIL_ON_LWNET_ERROR(dwError);

            dwError = LwDLinkedListAppend(&pSRVRecordList, pSrvInfoRecord);
            BAIL_ON_LWNET_ERROR(dwError);
            pSrvInfoRecord = NULL;
        }
    }
    
error:
    if (dwError)
    {
        LWNET_SAFE_FREE_SRV_INFO_LINKED_LIST(pSRVRecordList);
    }

    if (pSrvInfoRecord)
    {
        LWNetDnsFreeSRVInfoRecord(pSrvInfoRecord);
    }
    LWNET_SAFE_FREE_STRING(pszTarget);
    LWNET_SAFE_FREE_PSTR_LINKED_LIST(pAddressList);

    *ppSRVRecordList = pSRVRecordList;

    return dwError;
}

DWORD
LWNetDnsParseSrvRecord(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PDNS_RECORD pAnswerRecord,
    OUT PWORD Priority,
    OUT PWORD Weight,
    OUT PWORD Port,
    OUT PSTR* Target
    )
{
    DWORD dwError = 0;
    PBYTE pCurrent = NULL;
    DWORD dwNameLen = 0;
    WORD priority = 0;
    WORD weight = 0;
    WORD port = 0;
    PSTR target = NULL;
    
    if (pAnswerRecord->wDataLen < (4 * sizeof(WORD)))
    {
        dwError = DNS_ERROR_BAD_PACKET;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    pCurrent = pAnswerRecord->pData;

    priority = LWNetDnsReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    weight = LWNetDnsReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    port = LWNetDnsReadWORD(pCurrent);
    pCurrent += sizeof(WORD);

    dwError = LWNetDnsParseName(pHeader,
                                pCurrent,
                                &dwNameLen,
                                &target);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        priority = 0;
        weight = 0;
        port = 0;
        LWNET_SAFE_FREE_STRING(target);
    }

    *Priority = priority;
    *Weight = weight;
    *Port = port;
    *Target = target;

    return dwError;
}

DWORD
LWNetDnsParseOrGetAddressesForServer(
    IN PLW_DLINKED_LIST pAdditionalsList,
    IN PCSTR pszHostname,
    OUT PLW_DLINKED_LIST* ppAddressList
    )
{
    DWORD dwError = 0;
    PSTR  pszAddress = NULL;
    PLW_DLINKED_LIST pListMember = NULL;
    PLW_DLINKED_LIST pAddressList = NULL;
    
    pListMember = pAdditionalsList;
    while (pListMember)
    {
        PDNS_RECORD pRecord = (PDNS_RECORD)pListMember->pItem;
        
        if ( (pRecord->wType == ns_t_a ) &&
             !strcasecmp( pRecord->pszName, pszHostname ) )
        {
            dwError = LwAllocateStringPrintf(&pszAddress, "%d.%d.%d.%d",
                                                pRecord->pData[0],
                                                pRecord->pData[1],
                                                pRecord->pData[2],
                                                pRecord->pData[3]);
            BAIL_ON_LWNET_ERROR(dwError);

            dwError = LwDLinkedListAppend(&pAddressList, pszAddress);
            BAIL_ON_LWNET_ERROR(dwError);

            pszAddress = NULL;
        }
        else if ((pRecord->wType == ns_t_aaaa) &&
                 (!strcasecmp(pRecord->pszName, pszHostname)))
        {
            dwError = LwAllocateStringPrintf(
                        &pszAddress,
                        "%x:%x:%x:%x:%x:%x:%x:%x",
                        LW_BTOH16(((UINT16 *)pRecord->pData)[0]),
                        LW_BTOH16(((UINT16 *)pRecord->pData)[1]),
                        LW_BTOH16(((UINT16 *)pRecord->pData)[2]),
                        LW_BTOH16(((UINT16 *)pRecord->pData)[3]),
                        LW_BTOH16(((UINT16 *)pRecord->pData)[4]),
                        LW_BTOH16(((UINT16 *)pRecord->pData)[5]),
                        LW_BTOH16(((UINT16 *)pRecord->pData)[6]),
                        LW_BTOH16(((UINT16 *)pRecord->pData)[7]));
            BAIL_ON_LWNET_ERROR(dwError);

            dwError = LwDLinkedListAppend(&pAddressList, pszAddress);
            BAIL_ON_LWNET_ERROR(dwError);

            pszAddress = NULL;
        }

        pListMember = pListMember->pNext;
    }

    if (!pAddressList)
    {
        dwError = LWNetDnsGetAddressesForServer(
                        pszHostname,
                        &pAddressList);
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_PSTR_LINKED_LIST(pAddressList);
    }

    LWNET_SAFE_FREE_STRING(pszAddress);

    *ppAddressList = pAddressList;

    return dwError;
}

DWORD
LWNetDnsGetAddressesForServer(
    IN PCSTR pszHostname,
    OUT PLW_DLINKED_LIST* ppAddressList
    )
{
    DWORD dwError = 0;
    int aiError = 0;
    struct addrinfo hints = { 0 };
    struct addrinfo* addressInfo = NULL;
    struct addrinfo* current = NULL;
    // Large enough size to accomodate NULL-terminated IPv4 or IPv6 address.
    char hostname[128] = { 0 };
    PSTR pszAddress = NULL;
    PLW_DLINKED_LIST pAddressList = NULL;

    LWNET_LOG_VERBOSE("Getting addresses for '%s'", pszHostname);

    // Prevent multiple entries for the same address family being returned.
    hints.ai_socktype = SOCK_STREAM;

    aiError = getaddrinfo(pszHostname, NULL, &hints, &addressInfo);
    if (aiError)
    {
        LWNET_LOG_WARNING("Failed to resolve address for '%s' (EAI = %d)",
                          pszHostname, aiError);
        dwError = ERROR_NOT_FOUND;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    for (current = addressInfo;
         current;
         current = current->ai_next)
    {
        BOOLEAN useAddress = FALSE;

        switch (current->ai_family)
        {
            case AF_INET:
            case AF_INET6:
                useAddress = TRUE;
                break;
            default:
                useAddress = FALSE;
                break;
        }

        if (!useAddress)
        {
            continue;
        }

        hostname[sizeof(hostname) - 1] = 0;

        aiError = getnameinfo(current->ai_addr,
                              current->ai_addrlen, 
                              hostname,
                              sizeof(hostname),
                              NULL,
                              0,
                              NI_NUMERICHOST);
        if (aiError)
        {
            // This should never happen
            LWNET_LOG_ERROR("Unexpectedly failed to convert address for '%s' (EAI = %d)",
                            pszHostname, aiError);
            dwError = ERROR_NOT_FOUND;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        // getnameinfo() can truncate hostname, so check for null
        // termination before continuing.
        if (hostname[sizeof(hostname) - 1] != 0)
        {
            // This should never happen
            LWNET_LOG_WARNING("Unexpectedly long address for '%s'",
                              pszHostname);
            dwError = ERROR_NOT_FOUND;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        dwError = LWNetAllocateString(hostname, &pszAddress);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LwDLinkedListAppend(&pAddressList, pszAddress);
        BAIL_ON_LWNET_ERROR(dwError);

        pszAddress = NULL;
    }

    if (!pAddressList)
    {
        // This could happen if non-IP addresses are found
        LWNET_LOG_WARNING("Unable to get IP address for '%s'", pszHostname);
        dwError = ERROR_NOT_FOUND;
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_PSTR_LINKED_LIST(pAddressList);
    }

    LWNET_SAFE_FREE_STRING(pszAddress);
    if (addressInfo)
    {
        freeaddrinfo(addressInfo);
    }

    *ppAddressList = pAddressList;

    return dwError;
}

DWORD
LWNetDnsBuildServerArray(
    IN PLW_DLINKED_LIST pSrvRecordList,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    )
{
    DWORD dwError = 0;
    DWORD dwServerCount = 0;
    PDNS_SERVER_INFO pServerArray = NULL;
    PLW_DLINKED_LIST pListMember = NULL;
    PDNS_SRV_INFO_RECORD pSrvRecord = NULL;
    DWORD dwServerIndex = 0;
    DWORD dwStringSize = 0;
    DWORD dwRequiredSize = 0;
    PSTR pStringLocation = NULL;

    for (pListMember = pSrvRecordList; pListMember; pListMember = pListMember->pNext)
    {
        pSrvRecord = (PDNS_SRV_INFO_RECORD)pListMember->pItem;

        dwStringSize += strlen(pSrvRecord->pszAddress) + 1;
        dwStringSize += strlen(pSrvRecord->pszTarget) + 1;

        dwServerCount++;
    }

    if (dwServerCount < 1)
    {
        // nothing to do, so we are done
        dwError = 0;
        goto error;
    }

    dwRequiredSize = dwServerCount * sizeof(DNS_SERVER_INFO) + dwStringSize;

    dwError = LWNetAllocateMemory(dwRequiredSize, (PVOID*)&pServerArray);
    BAIL_ON_LWNET_ERROR(dwError);

    pStringLocation = CT_PTR_ADD(pServerArray, dwServerCount * sizeof(DNS_SERVER_INFO));
    dwServerIndex = 0;
    for (pListMember = pSrvRecordList; pListMember; pListMember = pListMember->pNext)
    {
        PSTR source;

        pSrvRecord = (PDNS_SRV_INFO_RECORD)pListMember->pItem;
        
        // Copy the strings into the buffer
        pServerArray[dwServerIndex].pszAddress = pStringLocation;
        for (source = pSrvRecord->pszAddress; source[0]; source++)
        {
            pStringLocation[0] = source[0];
            pStringLocation++;
        }
        pStringLocation[0] = source[0];
        pStringLocation++;

        pServerArray[dwServerIndex].pszName = pStringLocation;
        for (source = pSrvRecord->pszTarget; source[0]; source++)
        {
            pStringLocation[0] = source[0];
            pStringLocation++;
        }
        pStringLocation[0] = source[0];
        pStringLocation++;

        dwServerIndex++;
    }

    // TODO: Turns this into ASSERT
    if (CT_PTR_OFFSET(pServerArray, pStringLocation) != dwRequiredSize)
    {
        LWNET_LOG_ERROR("ASSERT - potential buffer overflow");
    }

error:    
    if (dwError)
    {
        LWNET_SAFE_FREE_MEMORY(pServerArray);
        dwServerCount = 0;
    }

    *ppServerArray = pServerArray;
    *pdwServerCount = dwServerCount;

    return dwError;
}

VOID
LWNetDnsFreeSRVInfoRecordInList(
    IN OUT PVOID pRecord,
    IN PVOID pUserData
    )
{
    PDNS_SRV_INFO_RECORD pSRVRecord = (PDNS_SRV_INFO_RECORD)pRecord;
    if (pSRVRecord)
    {
        LWNetDnsFreeSRVInfoRecord(pSRVRecord);
    }
}

VOID
LWNetDnsFreeSRVInfoRecord(
    IN OUT PDNS_SRV_INFO_RECORD pRecord
    )
{
    LWNET_SAFE_FREE_STRING(pRecord->pszAddress);
    LWNET_SAFE_FREE_STRING(pRecord->pszTarget);
    LWNetFreeMemory(pRecord);
}

VOID
LWNetDnsFreeDnsRecordLinkedList(
    IN OUT PLW_DLINKED_LIST DnsRecordList
    )
{
    LwDLinkedListForEach(DnsRecordList, LWNetDnsFreeRecordInList, NULL);
    LwDLinkedListFree(DnsRecordList);
}

VOID
LWNetDnsFreeSrvInfoLinkedList(
    IN OUT PLW_DLINKED_LIST SrvInfoList
    )
{
    LwDLinkedListForEach(SrvInfoList, LWNetDnsFreeSRVInfoRecordInList, NULL);
    LwDLinkedListFree(SrvInfoList);
}

VOID
LWNetDnsFreePstrInList(
    IN OUT PVOID pPstr,
    IN PVOID pUserData
    )
{
    PSTR pStr = (PSTR)pPstr;
    if (pStr)
    {
        LWNET_SAFE_FREE_MEMORY(pStr);
    }
}

VOID
LWNetDnsFreePstrLinkedList(
    IN OUT PLW_DLINKED_LIST PstrList
    )
{
    LwDLinkedListForEach(PstrList, LWNetDnsFreePstrInList, NULL);
    LwDLinkedListFree(PstrList);
}

DWORD
LWNetDnsQueryWithBuffer(
    IN PCSTR pszQuestion,
    IN BOOLEAN bReInit,
    IN BOOLEAN bUseTcp,
    OUT PVOID pBuffer,
    IN DWORD dwBufferSize,
    OUT PDWORD pdwResponseSize
    )
{
    DWORD dwError = 0;
    PDNS_RESPONSE_HEADER pHeader = (PDNS_RESPONSE_HEADER)pBuffer;
    int responseSize =  0;
    BOOLEAN bInLock = FALSE;
#if HAVE_DECL_RES_NINIT
    union
    {
        struct __res_state res;
#ifdef __LWI_AIX__
        // struct __res_state was enlarged from 720 in AIX 5.2 to 824 in AIX
        // 5.3. This means calling res_ninit on AIX 5.3 on a structure compiled
        // on AIX 5.2 will result in a buffer overflow. Furthermore, even on
        // AIX 5.3, res_ninit seems to expect 1596 bytes in the structure (1491
        // on AIX 5.2). As a workaround, this padding will ensure enough space
        // is allocated on the stack.
        char buffer[2048];
#endif
    } resLocal = { {0} };
    res_state res = &resLocal.res;
#else
    struct __res_state *res = &_res;
#endif

    LWNET_LOCK_RESOLVER_API(bInLock);

#if HAVE_DECL_RES_NINIT
    if (res_ninit(res) != 0)
#else
    if (res_init() != 0)
#endif
    {
        dwError = ERROR_NOT_FOUND;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    if (dwBufferSize < CT_MIN(sizeof(DNS_RESPONSE_HEADER), MAX_DNS_UDP_BUFFER))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    // TODO: Add lock on calling resolver due to global options, which may or
    // may not be safe depending on the system.
    if (bUseTcp)
    {
        res->options |= RES_USEVC;
    }
    else
    {
        res->options &= ~(RES_USEVC);
    }

    /* Assertion: pResolverContext != NULL && pResolverContext->bLocked == TRUE */
#if HAVE_DECL_RES_NINIT
    responseSize = res_nquery(res, pszQuestion, ns_c_in, ns_t_srv, (PBYTE) pBuffer, dwBufferSize);
#else
    responseSize = res_query(pszQuestion, ns_c_in, ns_t_srv, (PBYTE) pBuffer, dwBufferSize);
#endif
    if (responseSize < 0)
    {
        LWNET_LOG_VERBOSE("DNS lookup for '%s' failed with errno %d, h_errno = %d", pszQuestion, errno, h_errno);
        dwError = DNS_ERROR_BAD_PACKET;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    if (responseSize < CT_FIELD_OFFSET(DNS_RESPONSE_HEADER, data))
    {
        dwError = DNS_ERROR_BAD_PACKET;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    if (responseSize > dwBufferSize)
    {
        dwError = DNS_ERROR_BAD_PACKET;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    LWNetDnsFixHeaderForEndianness(pHeader);

    if (!LWNetDnsIsValidResponse(pHeader))
    {
        dwError = DNS_ERROR_BAD_PACKET;
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:

#if HAVE_DECL_RES_NINIT
    res_nclose(res);
#else
    /* Indicate that we are done with the resolver, except on HPUX which
       does not implement the res_close function. */
#ifndef __LWI_HP_UX__
    res_close();
#endif
#endif

    LWNET_UNLOCK_RESOLVER_API(bInLock);

    if (dwError)
    {
        responseSize = 0;
    }
    *pdwResponseSize = responseSize;
    return dwError;
}

DWORD
LWNetDnsGetSrvRecordQuestion(
    OUT PSTR* ppszQuestion,
    IN PCSTR pszDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags
    )
{
    DWORD dwError = 0;
    PSTR question = NULL;
    PCSTR kind = "dc";
    PCSTR service = "_ldap";

    if (dwDsFlags & DS_PDC_REQUIRED)
    {
        kind = "pdc";
        service = "_ldap";
    }
    else if (dwDsFlags & DS_GC_SERVER_REQUIRED)
    {
        kind = "gc";
        service = "_ldap";
    }
    else if (dwDsFlags & DS_KDC_REQUIRED)
    {
        kind = "dc";
        service = "_kerberos";
    }
    else
    {
        kind = "dc";
        service = "_ldap";
    }

    if (IsNullOrEmptyString(pszSiteName))
    {
        dwError = LwAllocateStringPrintf(&question,
                                            "%s._tcp.%s._msdcs.%s",
                                            service, kind,
                                            pszDomainName);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(&question,
                                            "%s._tcp.%s._sites.%s._msdcs.%s",
                                            service, pszSiteName, kind,
                                            pszDomainName);
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_STRING(question);
    }
    *ppszQuestion = question;
    return dwError;
}

DWORD
LWNetDnsSrvQuery(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    )
// Call LWNET_SAFE_FREE_MEMORY on returned server array
{
    DWORD dwError = 0;
    PSTR pszQuestion = NULL;
    const size_t dwBufferSize = (64 * 1024);
    PVOID pBuffer = NULL;
    PDNS_RESPONSE_HEADER pResponse = NULL;
    DWORD dwResponseSize = 0;
    PLW_DLINKED_LIST pAnswersList = NULL;
    PLW_DLINKED_LIST pAdditionalsList = NULL;
    PLW_DLINKED_LIST pSRVRecordList = NULL;
    PDNS_SERVER_INFO pServerArray = NULL;
    DWORD dwServerCount = 0;

    // TODO - Handle trailing dot in domain; handle no dots in domain

    dwError = LWNetDnsGetSrvRecordQuestion(&pszQuestion, pszDnsDomainName,
                                           pszSiteName, dwDsFlags);
    BAIL_ON_LWNET_ERROR(dwError);
   
    dwError = LWNetAllocateMemory(dwBufferSize, &pBuffer);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetDnsQueryWithBuffer(pszQuestion, TRUE, FALSE,
                                      pBuffer, dwBufferSize,
                                      &dwResponseSize);
    BAIL_ON_LWNET_ERROR(dwError);

    pResponse = (PDNS_RESPONSE_HEADER) pBuffer;
    if (LWNetDnsIsTruncatedResponse(pResponse))
    {
        dwError = LWNetDnsQueryWithBuffer(pszQuestion, FALSE, TRUE,
                                          pBuffer, dwBufferSize,
                                          &dwResponseSize);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    // TODO: Add dwResponseSize validation to parsing.

    // Decode DNS response w/o taking into account record type
    dwError = LWNetDnsParseQueryResponse(pResponse,
                                         &pAnswersList,
                                         NULL,
                                         &pAdditionalsList);
    BAIL_ON_LWNET_ERROR(dwError);

    // Decode SRV records
    dwError = LWNetDnsBuildSRVRecordList(pResponse,
                                         pAnswersList,
                                         pAdditionalsList,
                                         &pSRVRecordList);
    BAIL_ON_LWNET_ERROR(dwError);

    // Create list of server names and addresses
    dwError = LWNetDnsBuildServerArray(pSRVRecordList,
                                       &pServerArray, &dwServerCount);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    LWNET_SAFE_FREE_STRING(pszQuestion);
    LWNET_SAFE_FREE_MEMORY(pBuffer);
    LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAnswersList);
    LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(pAdditionalsList);
    LWNET_SAFE_FREE_SRV_INFO_LINKED_LIST(pSRVRecordList);

    if (dwError)
    {
        LWNET_SAFE_FREE_MEMORY(pServerArray);
        dwServerCount = 0;
    }

    *ppServerArray= pServerArray;
    *pdwServerCount = dwServerCount;

    return dwError;
}

