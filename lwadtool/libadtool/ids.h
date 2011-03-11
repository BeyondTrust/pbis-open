/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        ids.h
 *
 * Abstract:
 *        Methods fo ID manipulations.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: May 7, 2010
 *
 */

#ifndef _AD_TOOL_IDS_H_
#define _AD_TOOL_IDS_H_

/**
 * Covert object GUID to hex string.
 *
 * @param pszStr Bytes to convert.
 * @param pszStr Converted string.
 * @return 0 on success; error code on failure.
 */
extern DWORD Guid2Str(IN PVOID pszStr, OUT PSTR* ppszHexStr);

/**
 * Convert char to hex.
 *
 * @param in Char to convert.
 * @param s Preallocated 2 chars array.
 */
extern VOID Char2Hex(IN UCHAR in, OUT PSTR s);

/**
 * Covert AD object's SID to an ASCII string.
 *
 * @param s Bytes to convert.
 * @param out Converted string.
 * @return 0 on success; error code on failure.
 */
extern DWORD Sid2Str(IN PVOID s, OUT PSTR *out);

/**
 * Generate UID from SID.
 *
 * @param s SID bytes.
 * @param out UID (dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD Sid2Id(IN PVOID s, OUT PDWORD out);

#endif /* _AD_TOOL_IDS_H_ */
