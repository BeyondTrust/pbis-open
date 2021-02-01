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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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

