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
 *        dnsrequest.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

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


