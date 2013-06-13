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

DWORD
DNSUpdateCreatePtrRUpdateRequest(
    PDNS_UPDATE_REQUEST* ppDNSUpdateRequest,
    PCSTR pszZoneName,
    PCSTR pszPtrName,
    PCSTR pszHostNameFQDN
    );

DWORD
DNSSendPtrUpdate(
    HANDLE hDNSServer,
    PCSTR  pszZoneName,
    PCSTR pszPtrName,
    PCSTR pszHostNameFQDN,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    );

DWORD
DNSSendPtrSecureUpdate(
    HANDLE hDNSServer,
    PCtxtHandle pGSSContext,
    PCSTR pszKeyName,
    PCSTR pszZoneName,
    PCSTR pszPtrName,
    PCSTR pszHostNameFQDN,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    );

DWORD
DNSUpdatePtrSecureOnServer(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszZoneName,
    PCSTR  pszPtrName,
    PCSTR  pszHostNameFQDN
    );

DWORD
DNSUpdateCreateARUpdateRequest(
    PDNS_UPDATE_REQUEST* ppDNSUpdateRequest,
    PCSTR pszZoneName,
    PCSTR pszHostnameFQDN,
    DWORD  dwIPV4Count,
    DWORD  dwIPV6Count,
    PSOCKADDR_IN pAddrArray,
    PSOCKADDR_IN6 pAddr6Array
    );

DWORD
DNSNegotiateContextAndSecureUpdate(
    HANDLE hDNSServer,
    PCSTR  pszServiceName,
    PCSTR  pszDomainName,
    PCSTR  pszHost,
    DWORD  dwIPAddress
    );



DWORD
DNSSendUpdate(
    HANDLE hDNSServer,
    PCSTR  pszZoneName,
    PCSTR  pszHost,
    DWORD  dwIPV4Count,
    DWORD  dwIPV6Count,
    PSOCKADDR_IN pAddrArray,
    PSOCKADDR_IN6 pAddr6Array,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    );

DWORD
DNSSendSecureUpdate(
    HANDLE hDNSServer,
    PCtxtHandle pGSSContext,
    PCSTR pszKeyName,
    PCSTR pszZoneName,
    PCSTR pszHost,
    DWORD  dwIPV4Count,
    DWORD  dwIPV6Count,
    PSOCKADDR_IN pAddrArray,
    PSOCKADDR_IN6 pAddr6Array,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    );


DWORD
DNSUpdateGenerateSignature(
    PCtxtHandle pGSSContext,
    PDNS_UPDATE_REQUEST pDNSUpdateRequest,
    PCSTR pszKeyName
    );

DWORD
DNSBuildMessageBuffer(
    PDNS_UPDATE_REQUEST pDNSUpdateRequest,
    PCSTR pszKeyName,
    DWORD * pdwTimeSigned,
    WORD * pwFudge,
    PBYTE * ppMessageBuffer,
    PDWORD pdwMessageSize
    );

DWORD
DNSGetPtrDomainForAddr(
    PSTR*        ppszDomainName,
    PSOCKADDR_IN pAddr
    );

DWORD
DNSGetPtrNameForAddr(
    PSTR* ppszRecordName,
    PSOCKADDR_IN pAddr
    );

DWORD
DNSGetPtrZoneForAddr(
    PSTR* ppszZoneName,
    PSOCKADDR_IN pAddr
    );
