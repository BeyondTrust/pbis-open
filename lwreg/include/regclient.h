/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        regclient.h
 *
 * Abstract:
 *
 *        Registry Subsystem Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#ifndef __REGCLIENT_H__
#define __REGCLIENT_H__

#include <regsystem.h>

#include <reg/reg.h>


#include <regdef.h>

#include <lw/base.h>
#include <lw/types.h>
#include <lw/ntstatus.h>
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>

#include <lwmsg/lwmsg.h>

#include "regipc.h"

#include "regutils.h"

typedef struct __REG_VALUE
{
    PWSTR pwszValueName;
    REG_DATA_TYPE type;
    PBYTE pData;
    DWORD dwDataLen;
} REG_VALUE, *PREG_VALUE;


/*client utility functions*/

DWORD
GetValueAsStr(
    IN REG_DATA_TYPE dataType,
    IN PBYTE value,
    IN DWORD dwValueLen,
    OUT PSTR* ppszValue
    );

DWORD
CreateKeys(
    IN HANDLE hReg,
    IN HKEY pRootKey,
    IN PSTR pszParentKey,
    IN PSTR* ppszSubKeys,
    IN DWORD dwSubKeyNum,
    OUT PHKEY phkParentKey,
    OUT OPTIONAL PHKEY* pphkSubKeys
    );

DWORD
EnumKeys(
    HANDLE hReg,
    HKEY hKey
    );

DWORD
EnumValues(
    HANDLE hReg,
    HKEY hKey
    );

DWORD
SetValues(
    HANDLE hReg,
    HKEY hKey,
    PREG_VALUE* ppValues,
    DWORD dwValueNum
    );

void
FreeRegValue(
    PREG_VALUE* ppRegValue
    );

void
FreeRegValueList(
    PREG_VALUE* ppRegValue,
    DWORD  dwNumValues
    );

void
FreePVALENT(
    PVALENT pVal
    );

#endif /* __REGCLIENT_H__ */
