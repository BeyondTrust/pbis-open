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
#ifndef HAVE_HPUX_OS
    DWORD  dwIPV6Count,
    PSOCKADDR_IN pAddrArray,
    PSOCKADDR_IN6 pAddr6Array);
#else
    PSOCKADDR_IN pAddrArray);
#endif

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
#ifndef HAVE_HPUX_OS
    DWORD  dwIPV6Count,
#endif
    PSOCKADDR_IN pAddrArray,
#ifndef HAVE_HPUX_OS
    PSOCKADDR_IN6 pAddr6Array,
#endif
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
#ifndef HAVE_HPUX_OS
    DWORD  dwIPV6Count,
#endif
    PSOCKADDR_IN pAddrArray,
#ifndef HAVE_HPUX_OS
    PSOCKADDR_IN6 pAddr6Array,
#endif
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
