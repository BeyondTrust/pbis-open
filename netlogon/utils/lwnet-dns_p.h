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
 *        lwnet-dns_p.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        DNS API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LWNETDNS_P_H__
#define __LWNETDNS_P_H__

#include <lwdlinked-list.h>

#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) -1)
#endif

#if defined HAVE_DECL_RES_INIT && !HAVE_DECL_RES_INIT

int
res_init(void);

#endif

#if defined HAVE_DECL_RES_QUERY && !HAVE_DECL_RES_QUERY

int
res_query(
    const char*    dname,
    int            class,
    int            type,
    unsigned char* answer,
    int            anslen
    );

#endif

typedef struct _DNS_RECORD
{
    PSTR    pszName;
    WORD    wType;
    WORD    wClass;
    DWORD   dwTTL;
    WORD    wDataLen;
    PBYTE   pData;
} DNS_RECORD, *PDNS_RECORD;

typedef struct _DNS_SRV_INFO_RECORD {
    WORD    wPriority;
    WORD    wWeight;
    WORD    wPort;
    PSTR    pszTarget;
    PSTR    pszAddress;
} DNS_SRV_INFO_RECORD, *PDNS_SRV_INFO_RECORD;

#if defined(WORDS_BIGENDIAN)
typedef struct {
    uint16_t qr_message_type   : 1; // 0 = Query, 1 = Response
    uint16_t opcode            : 4; // 0 = Std Qry, 1 = Inverse Qry, 2 = Status Req
    uint16_t auth_answer       : 1; // 1 = response from an authoritative server
    uint16_t truncated         : 1; // 1 = message is longer than allowed
    uint16_t recursion_desired : 1; // 1 = server should persue the request recursively
    uint16_t recursion_avail   : 1; // 1 = recursive querying is available on server
    uint16_t reserved          : 2; // zero
    uint16_t answer_authentic  : 1; // 0 = answer authenticated by server, 1 = answer not authenticated
    uint16_t reply_code        : 4; // 0=No err, 1=Format err, 2=Server failure, 3=Name err, 4=Not impl, 5=Refused
} dns_flag_bits;
#else // Mirror image
typedef struct {
    uint16_t reply_code        : 4; // 0=No err, 1=Format err, 2=Server failure, 3=Name err, 4=Not impl, 5=Refused
    uint16_t answer_authentic  : 1; // 0 = answer authenticated by server, 1 = answer not authenticated
    uint16_t reserved          : 2; // zero
    uint16_t recursion_avail   : 1; // 1 = recursive querying is available on server
    uint16_t recursion_desired : 1; // 1 = server should persue the request recursively
    uint16_t truncated         : 1; // 1 = message is longer than allowed
    uint16_t auth_answer       : 1; // 1 = response from an authoritative server
    uint16_t opcode            : 4; // 0 = Std Qry, 1 = Inverse Qry, 2 = Status Req
    uint16_t qr_message_type   : 1; // 0 = Query, 1 = Response
} dns_flag_bits;
#endif

typedef struct _DNS_RESPONSE_HEADER
{
    WORD wId;
    union {
        dns_flag_bits B;
        WORD        W;
    } flags;
    WORD wQuestions;
    WORD wAnswers;
    WORD wAuths;
    WORD wAdditionals;
    BYTE data[1];
} DNS_RESPONSE_HEADER, *PDNS_RESPONSE_HEADER;

#define MAX_DNS_UDP_BUFFER 512

DWORD
LWNetDnsGetHostInfoEx(
    OUT OPTIONAL PSTR* ppszHostname,
    OUT OPTIONAL PSTR* ppszFqdn,
    OUT OPTIONAL PSTR* ppszDomain
    );

BOOLEAN
LWNetDnsConfigLineIsComment(
    IN PCSTR pszLine
    );

VOID
LWNetDnsFixHeaderForEndianness(
    IN OUT PDNS_RESPONSE_HEADER pHeader
    );

BOOLEAN
LWNetDnsIsValidResponse(
    IN PDNS_RESPONSE_HEADER pHeader
    );

BOOLEAN
LWNetDnsIsTruncatedResponse(
    IN PDNS_RESPONSE_HEADER pHeader
    );

DWORD
LWNetDnsParseQueryResponse(
    IN PDNS_RESPONSE_HEADER pHeader,
    OUT OPTIONAL PLW_DLINKED_LIST* ppAnswersList,
    OUT OPTIONAL PLW_DLINKED_LIST* ppAuthsList,
    OUT OPTIONAL PLW_DLINKED_LIST* ppAdditionalsList
    );

DWORD
LWNetDnsParseName(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PBYTE pData,
    OUT PDWORD pdwBytesToAdvance,
    OUT OPTIONAL PSTR* ppszName
    );

VOID
LWNetDnsParseNameWorker(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PBYTE pData,
    OUT OPTIONAL PDWORD pdwBytesToAdvance,
    OUT OPTIONAL PDWORD pdwNameLen,
    OUT OPTIONAL PSTR pszName
    );

DWORD
LWNetDnsParseRecords(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN WORD wNRecords,
    IN PBYTE pData,
    OUT PLW_DLINKED_LIST* ppRecordList,
    OUT PDWORD pdwBytesToAdvance
    );

DWORD
LWNetDnsParseRecord(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PBYTE pData,
    OUT PDNS_RECORD* ppRecord,
    OUT PDWORD pdwBytesToAdvance
    );

BYTE
LWNetDnsReadBYTE(
    IN PBYTE pBuffer
    );

WORD
LWNetDnsReadWORD(
    IN PBYTE pBuffer
    );

DWORD
LWNetDnsReadDWORD(
    IN PBYTE pBuffer
    );

DWORD
LWNetDnsBuildServerArray(
    IN PLW_DLINKED_LIST pSrvRecordList,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    );

VOID
LWNetDnsFreeRecordInList(
    IN OUT PVOID pRecord,
    IN PVOID pUserData
    );

VOID
LWNetDnsFreeRecord(
    IN OUT PDNS_RECORD pRecord
    );

DWORD
LWNetDnsGetSrvRecordQuestion(
    OUT PSTR* ppszQuestion,
    IN PCSTR pszDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwFlags
    );

DWORD
LWNetDnsBuildSRVRecordList(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PLW_DLINKED_LIST pAnswersList,
    IN PLW_DLINKED_LIST pAdditionalsList,
    OUT PLW_DLINKED_LIST* ppSRVRecordList
    );

DWORD
LWNetDnsParseSrvRecord(
    IN PDNS_RESPONSE_HEADER pHeader,
    IN PDNS_RECORD pAnswerRecord,
    OUT PWORD Priority,
    OUT PWORD Weight,
    OUT PWORD Port,
    OUT PSTR* Target
    );

DWORD
LWNetDnsParseOrGetAddressesForServer(
    IN PLW_DLINKED_LIST pAdditionalsList,
    IN PCSTR pszHostname,
    OUT PDLINKEDLIST* ppAddressList
    );

DWORD
LWNetDnsGetAddressesForServer(
    IN PCSTR pszHostname,
    OUT PDLINKEDLIST* ppAddressList
    );

VOID
LWNetDnsFreeSRVInfoRecordInList(
    IN OUT PVOID pRecord,
    IN PVOID pUserData
    );

VOID
LWNetDnsFreeSRVInfoRecord(
    IN OUT PDNS_SRV_INFO_RECORD pRecord
    );

VOID
LWNetDnsFreeDnsRecordLinkedList(
    IN OUT PLW_DLINKED_LIST DnsRecordList
    );

VOID
LWNetDnsFreeSrvInfoLinkedList(
    IN OUT PLW_DLINKED_LIST SrvInfoList
    );

VOID
LWNetDnsFreePstrLinkedList(
    IN OUT PDLINKEDLIST PstrList
    );

#define LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(DnsRecordList) \
    _LWNET_MAKE_SAFE_FREE(DnsRecordList, LWNetDnsFreeDnsRecordLinkedList)

#define LWNET_SAFE_FREE_SRV_INFO_LINKED_LIST(SrvInfoList) \
    _LWNET_MAKE_SAFE_FREE(SrvInfoList, LWNetDnsFreeSrvInfoLinkedList)

#define LWNET_SAFE_FREE_PSTR_LINKED_LIST(PstrList) \
    _LWNET_MAKE_SAFE_FREE(PstrList, LWNetDnsFreePstrLinkedList)

#endif /* __LWNETDNS_P_H__ */
