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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dnsutils.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __DNSUTILS_H__
#define __DNSUTILS_H__

#define INETV6_ADDRSTRLEN 256 
#define CANONICAL_INET6_ADDRSTRLEN 256
#define SUBNET_MASK 64
#define HEXADECIMAL_BASE 16

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

DWORD
DNSInet6ValidateAddress(
    PCSTR pszInet6InputAddr
    );

VOID
DNSInet6AddressReverse(
    PSTR pszInet6Addr
    );

VOID
DNSInet6Canonicalize(
    PSTR pszInet6InputAddr,
    PSTR pszInet6OutputAddr
    );

VOID
DNSInet6FillUpZeros(
    PSTR pszInet6InputAddr,
    PSTR pszInet6OutputAddr
    );

VOID
DNSInet6ExpandAddress(
    PSTR pszInet6InputAddr
    );


DWORD
DNSInet6GetPtrAddress(
    PSTR pszInet6InputAddr,
    PSTR *ppszInet6OutputAddr
    );

DWORD
DNSInet6GetPtrZoneAddress(
    PSTR pszInet6InputAddr,
    PSTR *ppszInet6OutputAddr
    );
#endif /* __DNSUTILS_H__ */
