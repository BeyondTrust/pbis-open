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

#ifndef __DNSUPREQUEST_H__
#define __DNSUPREQUEST_H__

DWORD
DNSUpdateCreateUpdateRequest(
    PDNS_UPDATE_REQUEST * ppDNSRequest
    );

DWORD
DNSUpdateSendUpdateRequest2(
    HANDLE hDNSServer,
    PDNS_UPDATE_REQUEST pDNSRequest
    );

DWORD
DNSUpdateBuildRequestMessage(
    PDNS_UPDATE_REQUEST pDNSRequest,
    HANDLE * phSendBuffer
    );

VOID
DNSUpdateFreeRequest(
    PDNS_UPDATE_REQUEST pDNSRequest
    );

DWORD
DNSUpdateAddZoneSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
    PDNS_ZONE_RECORD pDNSZone
    );
    
DWORD
DNSUpdateAddAdditionalSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    );

DWORD
DNSUpdateAddPRSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    );

DWORD
DNSUpdateAddUpdateSection(
    PDNS_UPDATE_REQUEST pDNSRequest,
    PDNS_RR_RECORD pDNSRecord
    );

#endif /* __DNSUPREQUEST_H__ */
