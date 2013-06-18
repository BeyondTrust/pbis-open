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
 *        regutilwrap.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry public wrapper for regshell utility functions
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 *
 */
#include "includes.h"


DWORD
RegUtilIsValidKey(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszKey)
{
    return RegShellIsValidKey(
               hReg,
               pszRootKeyName,
               pszKey);
}


DWORD
RegUtilAddKey(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR pszKeyName)
{
    return RegShellUtilAddKey(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               pszKeyName,
               FALSE);
}


DWORD
RegUtilAddKeySecDesc(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR pszKeyName,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor)
{
    return RegShellUtilAddKeySecDesc(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               pszKeyName,
               FALSE,
               AccessDesired,
               pSecurityDescriptor);
}


DWORD
RegUtilDeleteKey(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR keyName)
{
    return RegShellUtilDeleteKey(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               keyName);
}
     

DWORD
RegUtilDeleteTree(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR keyName)
{
    return RegShellUtilDeleteTree(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               keyName);
}


DWORD
RegUtilGetKeys(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR keyName,
    OUT PWSTR **pppSubKeys,
    OUT PDWORD pdwSubKeyCount)
{
    return RegShellUtilGetKeys(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               keyName,
               pppSubKeys,
               pdwSubKeyCount);
}


DWORD
RegUtilSetValue(
    IN OPTIONAL HANDLE hReg,
    IN PCSTR pszRootKeyName,
    IN PCSTR pszSubKeyPath,
    IN PCSTR keyName,
    IN PCSTR valueName,
    IN REG_DATA_TYPE type,
    IN PVOID data,
    IN DWORD dataLen)
{
    return RegShellUtilSetValue(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               keyName,
               valueName,
               type,
               data,
               dataLen);
}


DWORD
RegUtilGetValues(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR keyName,
    OUT PREGSHELL_UTIL_VALUE *valueArray,
    OUT PDWORD pdwValueArrayLen)
{
    return RegShellUtilGetValues(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               keyName,
               valueArray,
               pdwValueArrayLen);
}


DWORD
RegUtilGetKeyObjectCounts(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR keyName,
    OUT OPTIONAL PDWORD pdwSubKeysCount,
    OUT OPTIONAL PDWORD pdwValuesCount)
{
    return RegShellUtilGetKeyObjectCounts(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               keyName,
               pdwSubKeysCount,
               pdwValuesCount);
}


DWORD
RegUtilDeleteValue(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszSubKeyPath,
    IN PSTR keyName,
    IN PSTR valueName)
{
    return RegShellUtilDeleteValue(
               hReg,
               pszRootKeyName,
               pszSubKeyPath,
               keyName,
               valueName);
}


DWORD
RegUtilGetValue(
    IN OPTIONAL HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN OPTIONAL PSTR pszDefaultKey,
    IN PSTR pszKeyName,
    IN PSTR pszValueName,
    OUT OPTIONAL PREG_DATA_TYPE pRegType,
    OUT OPTIONAL PVOID *ppValue,
    OUT OPTIONAL PDWORD pdwValueLen)
{
    return RegShellUtilGetValue(
               hReg,
               pszRootKeyName,
               pszDefaultKey,
               pszKeyName,
               pszValueName,
               pRegType,
               ppValue,
               pdwValueLen);
}

