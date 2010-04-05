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

#ifndef __DNSUPRESP_H__
#define __DNSUPRESP_H__

DWORD
DNSUpdateReceiveUpdateResponse(
    HANDLE hDNSHandle,
    PDNS_UPDATE_RESPONSE * ppDNSResponse
    );   
    

DWORD
DNSUpdateUnmarshallZoneSection(    
    HANDLE hReceiveBuffer,
    WORD wZones,
    PDNS_ZONE_RECORD ** pppDNSZoneRecords
    );

DWORD
DNSUpdateUnmarshallPRSection(
    HANDLE hReceiveBuffer,
    WORD wPRs,
    PDNS_RR_RECORD ** pppDNSPRRRRecords
    );

DWORD
DNSUpdateUnmarshallUpdateSection(
    HANDLE hReceiveBuffer,
    WORD wUpdates,
    PDNS_RR_RECORD ** pppDNSUpdateRRRecords
    );

DWORD 
DNSUpdateUnmarshallAdditionalSection(
    HANDLE hReceiveBuffer,
    WORD   wAdditionals,
    PDNS_RR_RECORD ** pppDNSAdditionalsRRRecords
    );

VOID
DNSUpdateFreeResponse(
    PDNS_UPDATE_RESPONSE pDNSResponse
    );

#endif /* __DNSUPRESP_H__ */

