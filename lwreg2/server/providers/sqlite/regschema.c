/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        regschema.c
 *
 * Abstract:
 *
 *        Registry Schema Implementation (Value Attributes APIs etc)
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors:
 *          Wei Fu (wfu@likewise.com)
 */
#include "includes.h"

NTSTATUS
SqliteSetValueAttributes(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszValueName = NULL;
    BOOLEAN bIsWrongType = TRUE;
    wchar16_t wszEmptyValueName[] = REG_EMPTY_VALUE_NAME_W;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    // Do not free
    PBYTE pData = NULL;
    DWORD cbData = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);
    BAIL_ON_NT_INVALID_POINTER(pValueAttributes);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_SET_VALUE);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    if (MAX_VALUE_LENGTH < pValueAttributes->DefaultValueLen)
    {
        status = STATUS_INVALID_BLOCK_LENGTH;
        BAIL_ON_NT_STATUS(status);
    }

    status = LwRtlWC16StringDuplicate(&pwszValueName, !pValueName ? wszEmptyValueName : pValueName);
    BAIL_ON_NT_STATUS(status);

    status = RegDbGetValueAttributes(ghCacheConnection,
                              pKeyCtx->qwId,
                              (PCWSTR)pwszValueName,
                              (REG_DATA_TYPE)pValueAttributes->ValueType,
                              &bIsWrongType,
                              NULL);
    if (!status)
    {
        status = STATUS_DUPLICATE_NAME;
        BAIL_ON_NT_STATUS(status);
    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
    {
        status = 0;
    }
    BAIL_ON_NT_STATUS(status);

    pData = pValueAttributes->pDefaultValue;
    cbData = pValueAttributes->DefaultValueLen;

    if (cbData == 0)
    {
        goto done;
    }

    switch (pValueAttributes->ValueType)
    {
        case REG_BINARY:
        case REG_DWORD:
            break;

        case REG_MULTI_SZ:
        case REG_SZ:
            if (cbData == 1)
            {
                status = STATUS_INTERNAL_ERROR;
                BAIL_ON_NT_STATUS(status);
            }

            if (!pData || pData[cbData-1] != '\0' || pData[cbData-2] != '\0' )
            {
                status = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(status);
            }

            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(status);
    }

done:
    status = RegDbSetValueAttributes(ghCacheConnection,
                                     pKeyCtx->qwId,
                                     pwszValueName,
                                     pValueAttributes);
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWREG_SAFE_FREE_MEMORY(pwszValueName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteGetValueAttributes(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN PCWSTR pwszValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    )
{
    NTSTATUS status = 0;
    PLWREG_CURRENT_VALUEINFO pCurrentValue = NULL;

    CHAR szDefaultVal[] = "Default value";
    CHAR szDocString[] = "Document String";

    CHAR* ppszRangeEnumStrings[] = {"enum1", "enum2", "enum3", NULL};

    LWREG_VALUE_ATTRIBUTES_A ValueAttribute = {
            REG_SZ,
            szDefaultVal,
            sizeof(szDefaultVal),
            szDocString,
            LWREG_VALUE_RANGE_TYPE_ENUM,
            0};

    ValueAttribute.Range.ppszRangeEnumStrings = ppszRangeEnumStrings;

    status = LW_RTL_ALLOCATE((PVOID*)&pCurrentValue, LWREG_CURRENT_VALUEINFO, sizeof(*pCurrentValue));
    BAIL_ON_NT_STATUS(status);

    pCurrentValue->dwType = REG_SZ;

    status = LwRtlWC16StringAllocateFromCString((PWSTR*)&pCurrentValue->pvData,
                                                (PCSTR)"Current Value");
    BAIL_ON_NT_STATUS(status);

    pCurrentValue->cbData = (LwRtlWC16StringNumChars(pCurrentValue->pvData)+1)*sizeof(WCHAR);


    status = RegConvertValueAttributesAToW(ValueAttribute,
                                    ppValueAttributes);
    BAIL_ON_NT_STATUS(status);

    *ppCurrentValue = pCurrentValue;

cleanup:
    return status;

error:
    RegSafeFreeCurrentValueInfo(&pCurrentValue);
    *ppCurrentValue = NULL;
    *ppValueAttributes = NULL;

    goto cleanup;
}

NTSTATUS
SqliteDeleteValueAttributes(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


