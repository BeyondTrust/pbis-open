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
