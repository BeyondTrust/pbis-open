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
 *        securityidentifier_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 * 
 *        Security Identifier API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __SECURITYIDENTIFIER_P_H__
#define __SECURITYIDENTIFIER_P_H__

#define SECURITY_IDENTIFIER_HEADER_SIZE 8
#define SECURITY_IDENTIFIER_MINIMUM_SIZE ((SECURITY_IDENTIFIER_HEADER_SIZE) + (sizeof(DWORD)))

typedef enum {
    PARSE_MODE_OPEN = 0,
    PARSE_MODE_REVISION,
    PARSE_MODE_AUTHORITY,
    PARSE_MODE_TAIL,
    PARSE_SENTINEL
} SecurityIdentifierParseState;


DWORD
LsaHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    );

//format of string representation of SID in SECURITYIDENTIFIER:
//S-<revision>-<authority>-<domain_computer_id>-<relative ID>
//example: S-1-5-32-546 (Guests)
//See http://support.microsoft.com/kb/243330

//In binary format, 
//the fields are encoded as unsigned integers of varying lenths and endianness:
//<revision>: 1-byte, big-endian
//<word count>: 1-byte, big-endian
//<authority>: a 6-byte, big-endian number containing the number of subsequent 4-byte, little endian, unsigned integers
//<domain_computer_id>: a sequence of 4-byte, little-endian unsigned integers
//<relative-id>: 4-byte, little-endian, unsigned integer


//So for example, if your SID is S-1-5-21-2127521184-1604012920-1887927527-72713, 
//then your raw hex SID is
// 01 05 000000000005 15000000 A065CF7E 784B9B5F E77C8770 091C0100
//This breaks down as follows:
// 01   S-1
// 05   (seven dashes, seven minus two = 5)
// 000000000005 (5 = 0x000000000005, written as big-endian) 6 bytes
// 15000000 (21 = 0x00000015, written as big-endian)  4 bytes
// A065CF7E (2127521184 = 0x7ECF65A0, written as big-endian)  4 bytes
// 784B9B5F (1604012920 = 0x5F9B4B78, written as big-endian)  4 bytes
// E77C8770 (1887927527 = 0X70877CE7, written as big-endian)  4 bytes
// 091C0100 (72713 = 0x00011c09, written as big-endian)

//This will calculate a hash of the last three RID's (if they exist) and the low 19
//bits of the RID
void
LsaUidHashCalc(
    PDWORD pdwAuthorities,
    DWORD dwAuthorityCount,
    PDWORD pdwHash
    );
    
DWORD
LsaSidStringToBytes(
    IN PCSTR pszSidString,
    OUT UCHAR** ppucSidBytes,
    OUT DWORD* pdwSidBytesLength
    );

DWORD
LsaBuildSIDString(
    PCSTR pszRevision,
    PCSTR pszAuth,
    PBYTE pucSidBytes,
    DWORD dwWordCount,
    PSTR* ppszSidString
    );

#endif /* __SECURITYIDENTIFIER_P_H__ */
