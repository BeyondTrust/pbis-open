/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-dns_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
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
    OUT PLW_DLINKED_LIST* ppAddressList
    );

DWORD
LWNetDnsGetAddressesForServer(
    IN PCSTR pszHostname,
    OUT PLW_DLINKED_LIST* ppAddressList
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
    IN OUT PLW_DLINKED_LIST PstrList
    );

#define LWNET_SAFE_FREE_DNS_RECORD_LINKED_LIST(DnsRecordList) \
    _LWNET_MAKE_SAFE_FREE(DnsRecordList, LWNetDnsFreeDnsRecordLinkedList)

#define LWNET_SAFE_FREE_SRV_INFO_LINKED_LIST(SrvInfoList) \
    _LWNET_MAKE_SAFE_FREE(SrvInfoList, LWNetDnsFreeSrvInfoLinkedList)

#define LWNET_SAFE_FREE_PSTR_LINKED_LIST(PstrList) \
    _LWNET_MAKE_SAFE_FREE(PstrList, LWNetDnsFreePstrLinkedList)

#endif /* __LWNETDNS_P_H__ */
