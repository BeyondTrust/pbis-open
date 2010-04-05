/*++
	Linux DNS client library implementation
	Copyright (C) 2006 Krishna Ganugapati

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

++*/

#ifndef __DNSREQUEST_H__
#define __DNSREQUEST_H__

DWORD
DNSMakeQuestion(
    HANDLE         hDNSServer,
    PCSTR          pszQuestionName,
    WORD           wQClass,
    WORD           wQType,
    PDNS_RESPONSE* ppDNSResponse
    );

DWORD
DNSStdSendStdRequest2(
    HANDLE hDNSServer,
    PDNS_REQUEST pDNSRequest
    );

DWORD
DNSStdSendRequestSynch2(
    HANDLE       hDNSServer,
    PDNS_REQUEST pDNSRequest,
    PDNS_RESPONSE * ppDNSResponse
    );

DWORD
DNSBuildRequestMessage(
    PDNS_REQUEST pDNSRequest,
    PHANDLE      phSendBuffer
    );

DWORD
DNSStdCreateStdRequest(
    PDNS_REQUEST * ppDNSRequest
    );

DWORD
DNSStdAddQuestionSection(
    PDNS_REQUEST pDNSRequest,
    PDNS_QUESTION_RECORD pDNSQuestion
    );

DWORD
DNSStdAddAdditionalSection(
    PDNS_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    );

DWORD
DNSStdAddAnswerSection(
    PDNS_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    );

DWORD
DNSStdAddAuthoritySection(
    PDNS_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    );

VOID
DNSStdFreeRequest(
    PDNS_REQUEST pDNSRequest
    );

DWORD
DNSMarshallDomainName(
    HANDLE hSendBuffer,
    PDNS_DOMAIN_NAME  pDomainName
    );

DWORD
DNSMarshallRRHeader(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD pDNSRecord
    );

DWORD
DNSMarshallRData(
    HANDLE hSendBuffer,
    PDNS_RR_RECORD pDNSRecord
    );

#endif /* __DNSREQUEST_H__ */


