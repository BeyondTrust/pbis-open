/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lwps-sid.h
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 *        Security Identifier API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWPS_SID_H__
#define __LWPS_SID_H__

//format of string representation of SID in SECURITYIDENTIFIER:
//S-<revision>-<authority>-<domain_computer_id>-<relative ID>
//example: S-1-5-32-546 (Guests)
//See http://support.microsoft.com/kb/243330

#define WELLKNOWN_SID_DOMAIN_USER_GROUP_RID 513

typedef struct __LWPS_SECURITY_IDENTIFIER {
    UCHAR* pucSidBytes;
    DWORD dwByteLength;
} LWPS_SECURITY_IDENTIFIER, *PLWPS_SECURITY_IDENTIFIER;


DWORD
LwpsAllocSecurityIdentifierFromBinary(
    UCHAR* sidBytes,
    DWORD dwSidBytesLength,
    PLWPS_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

DWORD
LwpsAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLWPS_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

VOID
LwpsFreeSecurityIdentifier(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier
    );

DWORD
LwpsGetSecurityIdentifierRid(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD pdwRid
    );

DWORD
LwpsSetSecurityIdentifierRid(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    DWORD dwRid
    );

//The UID is a DWORD constructued using
//a non-cryptographic, 2-way hash of 
//the User SID and Domain SID.
DWORD
LwpsGetSecurityIdentifierHashedRid(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    );

DWORD
LwpsGetSecurityIdentifierBinary(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    );

DWORD
LwpsGetSecurityIdentifierString(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    );

DWORD
LwpsGetSecurityIdentifierDomain(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLWPS_SECURITY_IDENTIFIER* ppSecurityIdentifierDomain
    );

DWORD
LwpsHexStrToByteArray(
        PCSTR pszHexString,
        UCHAR** ppucByteArray,
        DWORD* pdwByteArrayLength
        );

DWORD
LwpsByteArrayToHexStr(
        UCHAR* pucByteArray,
        DWORD dwByteArrayLength,
        PSTR* ppszHexString
        );

#endif /* __LWPS_SID_H__ */
