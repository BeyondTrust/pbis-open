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

#ifndef __DNSUTILS_H__
#define __DNSUTILS_H__

DWORD
DNSGenerateIdentifier(
	 WORD * pwIdentifer
	 );

DWORD
DNSGetDomainNameLength(
	PDNS_DOMAIN_NAME pDomainName,
	PDWORD pdwLength
	);

DWORD
DNSCopyDomainName(
	PBYTE pBuffer,
	PDNS_DOMAIN_NAME pDomainName,
	PDWORD pdwCopied
	);

DWORD
DNSGenerateKeyName(
	PSTR* ppszKeyName
	);

DWORD
DNSDomainNameFromString(
    PCSTR pszDomainName,
    PDNS_DOMAIN_NAME * ppDomainName
    );

VOID
DNSFreeDomainName(
    PDNS_DOMAIN_NAME pDomainName
    );

DWORD
DNSMapResponseCode(
    WORD wResponseCode
    );

DWORD
DNSAppendLabel(
    PDNS_DOMAIN_LABEL pLabelList,
    PDNS_DOMAIN_LABEL pLabel,
    PDNS_DOMAIN_LABEL * ppNewLabelList
    );

VOID
DNSFreeLabelList(
    PDNS_DOMAIN_LABEL pLabelList
    );

VOID
DNSFreeLabel(
    PDNS_DOMAIN_LABEL pLabel
    );

#endif /* __DNSUTILS_H__ */
