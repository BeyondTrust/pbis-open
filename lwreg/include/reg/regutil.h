/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        regutil.h
 *
 * Abstract:
 *
 *        Registry Helper Utilities
 *
 *        Public Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewisesoftware.com)
 */
#ifndef __REGUTIL_H__
#define __REGUTIL_H__

#include <reg/lwreg.h>


typedef struct _REGSHELL_UTIL_VALUE
{
    REG_DATA_TYPE type;
    PWSTR pValueName;
    PVOID pData;
    DWORD dwDataLen;
} REGSHELL_UTIL_VALUE, *PREGSHELL_UTIL_VALUE;


DWORD
RegUtilIsValidKey(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN PCSTR pszKey
    );

DWORD
RegUtilAddKey(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszSubKeyPath,
    IN PCSTR pszKeyName
    );

DWORD
RegUtilAddKeySecDesc(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN PCSTR pszSubKeyPath,
    IN PCSTR pszKeyName,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor
    );

DWORD
RegUtilDeleteKey(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszSubKeyPath,
    IN PCSTR pszKey
    );

DWORD
RegUtilDeleteTree(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszSubKeyPath,
    IN PCSTR pszKeyName
    );

DWORD
RegUtilGetKeys(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszSubKeyPath,
    IN PCSTR pszKeyName,
    OUT PWCHAR **pppSubKeys,
    OUT PDWORD pdwSubKeyCount
    );

DWORD
RegUtilSetValue(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszSubKeyPath,
    IN PCSTR pszKeyName,
    IN PCSTR pszValueName,
    IN REG_DATA_TYPE valueType,
    IN PVOID pData,
    IN DWORD dwDataLen
    );

DWORD
RegUtilGetValues(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszSubKeyPath,
    IN PCSTR pszKeyName,
    OUT PREGSHELL_UTIL_VALUE *valueArray,
    OUT PDWORD pdwValueArrayLen
    );

DWORD
RegUtilGetKeyObjectCounts(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR keyName,
    OUT OPTIONAL PDWORD pdwSubKeysCount,
    OUT OPTIONAL PDWORD pdwValuesCount
    );

DWORD
RegUtilDeleteValue(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszSubKeyPath,
    IN PCSTR pszKeyName,
    IN PCSTR pszValueName
    );

DWORD
RegUtilGetValue(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszDefaultKey,
    IN PCSTR pszKeyName,
    IN PCSTR pszValueName,
    OUT OPTIONAL PREG_DATA_TYPE pRegType,
    OUT OPTIONAL PVOID *ppValue,
    OUT OPTIONAL PDWORD pdwValueLen
    );

#endif
