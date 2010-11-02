/*
* Copyright (c) Likewise Software.  All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at
* your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.  You should have received a copy of the GNU General
* Public License along with this program.  If not, see
* <http://www.gnu.org/licenses/>.
*
* LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
* TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
* WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
* TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
* GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
* HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
* TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
* license@likewise.com
*/
#ifndef LWCONFIG_UTIL_H
#define LWCONFIG_UTIL_H

DWORD
UtilAllocateMultistring(
    PCSTR *ppszValues,
    DWORD dwValues,
    PSTR *ppszValue
    );

DWORD
UtilMultistringLength(
    PCSTR pszValue
    );

DWORD
UtilDuplicateMultistring(
    PCSTR pszValue,
    PSTR *ppszValue
    );

DWORD
UtilParseRegName(
    PCSTR pszPath,
    PSTR *ppszRoot,
    PSTR *ppszKey,
    PSTR *ppszValueName
    );

DWORD
UtilSetValueExA(
    PCSTR pszRoot,
    PCSTR pszKey,
    PCSTR pszValueName,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    );

DWORD
UtilGetValueExA(
    PCSTR pszRoot,
    PCSTR pszKey,
    PCSTR pszValueName,
    DWORD dwType,
    PVOID *ppvData,
    PDWORD pcbData
    );

DWORD
UtilParseLine(
    PCSTR pszLine,
    PSTR **pppszArgs,
    PDWORD pdwArgs
    );

DWORD
UtilReadLine(
    FILE* pStream,
    PSTR* ppszLine
    );

DWORD
UtilAllocateEscapedString(
    PCSTR pszStr,
    PSTR *ppszEscapedStr
    );

#endif
