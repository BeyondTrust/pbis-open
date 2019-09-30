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
 *        dnsstruct.h
 *
 * Abstract:
 *
 *        BeyondTrust Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __DNSSTRUCT_H__
#define __DNSSTRUCT_H__



#ifndef WIN32

typedef int SOCKET;
typedef struct sockaddr SOCKADDR, *PSOCKADDR;
typedef gss_ctx_id_t CtxtHandle, *PCtxtHandle;

#endif

typedef struct _DNS_SERVER {
    PSTR pszServer;
} DNS_SERVER, *PDNS_SERVER;

typedef struct _DNS_DOMAIN_LABEL {
	char * pszLabel;
	DWORD dwLength;
	struct _DNS_DOMAIN_LABEL * pNext;
} DNS_DOMAIN_LABEL, *PDNS_DOMAIN_LABEL;

typedef struct _DNS_DOMAIN_NAME {
	PDNS_DOMAIN_LABEL pLabelList;
}DNS_DOMAIN_NAME, *PDNS_DOMAIN_NAME;

typedef struct _DNS_QUESTION_RECORD {
	PDNS_DOMAIN_NAME pDomainName;
	WORD wQueryType;
	WORD wQueryClass;
}DNS_QUESTION_RECORD, *PDNS_QUESTION_RECORD;


typedef struct _DNS_ZONE_RECORD {
	PDNS_DOMAIN_NAME pDomainName;
	WORD wZoneType;
	WORD wZoneClass;
}DNS_ZONE_RECORD, *PDNS_ZONE_RECORD;


typedef struct _DNS_RR_HEADER {
	PDNS_DOMAIN_NAME pDomainName;
	WORD wType;
	WORD wClass;
	DWORD dwTTL;
	WORD  wRDataSize;
}DNS_RR_HEADER, *PDNS_RR_HEADER;


typedef struct _DNS_DEF_DATA {
	PBYTE pDefData;
} DNS_DEF_RDATA, *PDNS_DEF_RDATA;

typedef struct _DNS_TKEY_RDATA {
	WORD wAlgorithmOffset;
	WORD wInceptionOffset;
	WORD wExpirationOffset;
	WORD wModeOffset;
	WORD wErrorOffset;
	WORD wKeySizeOffset;
	WORD wKeyDataOffset;
	WORD wOtherSizeOffset;
	WORD wOtherDataOffset;
}DNS_TKEY_OFFSETS, *PDNS_TKEY_OFFSETS;

typedef struct _DNS_TSIG_RDATA {
	WORD wAlgorithmOffset;
	WORD wTimeSignedOffset;
	WORD wFudgeOffset;
	WORD wMacSizeOffset;
	WORD wMacDataOffset;
	WORD wOriginalMessageIdOffset;
	WORD wErrorOffset;
	WORD wOtherSizeOffset;
	WORD wOtherDataOffset;
} DNS_TSIG_OFFSETS, *PDNS_TSIG_OFFSETS;


typedef struct _DNS_RR_RECORD {
	DNS_RR_HEADER RRHeader;
	union {
		DNS_TKEY_OFFSETS TKey;
		DNS_TSIG_OFFSETS TSig;
	}Offsets;
	// either pRData or pRDataDomain may be set
	PBYTE pRData;
	PDNS_DOMAIN_NAME pRDataDomain;
}DNS_RR_RECORD, *PDNS_RR_RECORD;


typedef struct _DNS_REQUEST {
	WORD wIdentification;
	WORD wParameter;
	WORD wQuestions;
	WORD wAnswers;
	WORD wAuthoritys;
	WORD wAdditionals;
	PDNS_QUESTION_RECORD * ppQuestionRRSet;
	PDNS_RR_RECORD * ppAnswerRRSet;
	PDNS_RR_RECORD * ppAuthorityRRSet;
	PDNS_RR_RECORD * ppAdditionalRRSet;
} DNS_REQUEST, * PDNS_REQUEST;


typedef struct _DNS_UPDATE_REQUEST {
	// Called Transaction ID in wireshark
	// Called ID in RFC2136
	// This is a number randomly chosen by the client to identify which
	// reply belongs to the request incase the request was sent over UDP.
	WORD wIdentification;
	// Composed of QR, Opcode, Z, and RCODE in RFC2136
	// Called Flags in wireshark
	WORD wParameter;
	// Called ZOCOUNT in RFC2136
	// Number of number zones in the update request (ppZoneRRSet)
	// The only value that makes sense is 1
	WORD wZones;
	// Called PRCOUNT in RFC2136
	// Number of prerequisites in update request (ppPRRRSet)
	WORD wPRs;
	// Called UPCOUNT in RFC2136
	// Number of updates (adds or deletes) in update request
	// (ppUpdateRRSet)
	WORD wUpdates;
	// Called ADCOUNT in RFC2136
	// Number of additional records (only a signature record is used here)
	// in update request (ppAdditionalRRSet)
	WORD wAdditionals;
	PDNS_ZONE_RECORD * ppZoneRRSet;
	PDNS_RR_RECORD * ppPRRRSet;
	PDNS_RR_RECORD * ppUpdateRRSet;
	PDNS_RR_RECORD * ppAdditionalRRSet;
} DNS_UPDATE_REQUEST, * PDNS_UPDATE_REQUEST;


typedef struct _DNS_RESPONSE {
	WORD wIdentification;
	WORD wParameter;
	WORD wQuestions;
	WORD wAnswers;
	WORD wAuthoritys;
	WORD wAdditionals;
	PDNS_QUESTION_RECORD * ppQuestionRRSet;
	PDNS_RR_RECORD * ppAnswerRRSet;
	PDNS_RR_RECORD * ppAuthorityRRSet;
	PDNS_RR_RECORD * ppAdditionalRRSet;
    PBYTE pDNSOutBuffer;
	DWORD dwNumBytes;
} DNS_RESPONSE, * PDNS_RESPONSE;

typedef struct _DNS_UPDATE_RESPONSE {
	WORD wIdentification;
	WORD wParameter;
	WORD wZones;
	WORD wPRs;
	WORD wUpdates;
	WORD wAdditionals;
	PDNS_ZONE_RECORD * ppZoneRRSet;
	PDNS_RR_RECORD * ppPRRRSet;
	PDNS_RR_RECORD * ppUpdateRRSet;
	PDNS_RR_RECORD * ppAdditionalRRSet;
    PBYTE pDNSOutBuffer;
	DWORD dwNumBytes;
} DNS_UPDATE_RESPONSE, * PDNS_UPDATE_RESPONSE;


typedef struct _DNS_CONNECTION_CONTEXT {
	DWORD hType;
	SOCKET s;
	SOCKADDR_IN RecvAddr;
} DNS_CONNECTION_CONTEXT, *PDNS_CONNECTION_CONTEXT;


typedef struct _DNS_SENDBUFFER_CONTEXT {
	PBYTE pSendBuffer;
	DWORD dwBufferSize;
	DWORD dwBytesWritten;
	DWORD dwBufferOffset;
} DNS_SENDBUFFER_CONTEXT, *PDNS_SENDBUFFER_CONTEXT;



typedef struct _DNS_RECEIVEBUFFER_CONTEXT {
	PBYTE pRecvBuffer;
	DWORD dwBufferSize;
	DWORD dwBytesRecvd;
	DWORD dwBytesRead;
} DNS_RECEIVEBUFFER_CONTEXT, *PDNS_RECEIVEBUFFER_CONTEXT;

typedef struct _DNS_RECORD
{
    PSTR    pszName;
    WORD    wType;
    WORD    wClass;
    DWORD   dwTTL;
    WORD    wDataLen;
    PBYTE   pData;
} DNS_RECORD, *PDNS_RECORD;

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

#endif /* __DNSSTRUCT_H__ */
