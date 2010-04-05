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

#ifndef __DNSRESPONSE_H__
#define __DNSRESPONSE_H__

DWORD
DNSStdReceiveStdResponse(
    HANDLE         hDNSHandle,
    PDNS_RESPONSE* ppDNSResponse
    );

DWORD
DNSUpdateGetResponseCode(
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse,
    PDWORD pdwResponseCode
    );

DWORD
DNSStdGetResponseCode(
    PDNS_RESPONSE pDNSResponse
    );

VOID
DNSStdFreeResponse(
    PDNS_RESPONSE pDNSResponse
    );

DWORD
DNSUnmarshallDomainName(
    HANDLE hRecvBuffer, 
    PDNS_DOMAIN_NAME * ppDomainName
    );

DWORD
DNSUnmarshallRRHeader(
    HANDLE         hRecvBuffer,
    PDNS_RR_HEADER pRRHeader
    );

DWORD
DNSUnmarshallRData(
    HANDLE hRecvBuffer,
    DWORD  dwSize,
    PBYTE* ppRData,
    PDWORD pdwRead
    );

#endif /* __DNSRESPONSE_H__ */

