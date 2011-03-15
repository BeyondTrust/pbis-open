/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        memdb.c
 *
 * Abstract:
 *        Database implementation for registry memory provider backend
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#include "includes.h"
/*
 * All functions that implement the memory-based registry
 * provider are implemented in this file.
 */



NTSTATUS
MemDbOpen(
    OUT PREG_DB_HANDLE phDb
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pConn, REG_DB_CONNECTION, sizeof(*pConn));
    BAIL_ON_NT_STATUS(status);

    memset(pConn, 0, sizeof(*pConn));
    status = MemRegStoreOpen(&pConn->pMemReg);

    *phDb = pConn;

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbClose(
    IN PREG_DB_HANDLE phDb)
{
    NTSTATUS status = 0;

    BAIL_ON_NT_STATUS(status);


cleanup:
    return status;

error:
    goto cleanup;
}


static PWSTR
pwstr_wcschr(
    PWSTR pwszHaystack, WCHAR wcNeedle)
{
    DWORD i = 0;
    for (i=0; pwszHaystack[i] != '\0'; i++)
    {
        if (pwszHaystack[i] == wcNeedle)
        {
            return &pwszHaystack[i];
        }
    }
    return NULL;
}

NTSTATUS
MemDbOpenKey(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyPath,
    OUT OPTIONAL MEM_REG_STORE_HANDLE *pRegKey)
{
    NTSTATUS status = 0;
    PWSTR pwszPtr = NULL;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszTmpFullPath = NULL;

    MEM_REG_STORE_HANDLE hParentKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    BOOLEAN bEndOfString = FALSE;
     
    if (!hDb)
    {
        status = MemRegStoreFindNode(
                     ghMemRegRoot,
                     pwszFullKeyPath,
                     pRegKey);
    }
    else
    {
        status = LwRtlWC16StringDuplicate(&pwszTmpFullPath, pwszFullKeyPath);
        BAIL_ON_NT_STATUS(status);

        pwszSubKey = pwszTmpFullPath;
        hParentKey = hDb->pMemReg;
        do 
        {
            pwszPtr = pwstr_wcschr(pwszSubKey, L'\\');
            if (pwszPtr)
            {
                *pwszPtr++ = L'\0';
            }
            else
            {
                pwszPtr = pwszSubKey;
                bEndOfString = TRUE;
            }

      
            /*
             * Iterate over subkeys in \ sepearated path.
             */
            status = MemRegStoreFindNode(
                         hParentKey,
                         pwszSubKey,
                         &hSubKey);
            hParentKey = hSubKey;
            pwszSubKey = pwszPtr;
            *pRegKey = hParentKey;
        } while (status == 0 && !bEndOfString);
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszTmpFullPath);
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pwszTmpFullPath);
    goto cleanup;
}


NTSTATUS
MemDbCreateKeyEx(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pcwszSubKey,
    IN DWORD dwReserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT PMEM_REG_STORE_HANDLE phSubKey,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hParentKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    
    hParentKey = hDb->pMemReg;

    /*
     * Iterate over subkeys in \ sepearated path.
     */
    status = MemRegStoreFindNode(
                 hParentKey,
                 pcwszSubKey,
                 &hSubKey);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        /* New node for current subkey, add it */
        status = MemRegStoreAddNode(
                         hParentKey,
                         pcwszSubKey,
                         REGMEM_TYPE_KEY,
                         NULL,  // SD parameter
                         NULL,
                         &hParentKey);
        BAIL_ON_NT_STATUS(status);
        *phSubKey = hParentKey;
    }
    else
    {
        /* Current node exists, return subkey handle */
        *phSubKey = hSubKey;
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbQueryInfoKey(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    /*
     * A pointer to a buffer that receives the user-defined class of the key. 
     * This parameter can be NULL.
     */
    OUT OPTIONAL PWSTR pClass, 
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pdwReserved, /* This parameter is reserved and must be NULL. */
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen, /* implement this later */
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime /* implement this later */
    )
{
    MEM_REG_STORE_HANDLE hKey = NULL;
    NTSTATUS status = 0;
    DWORD keyLen = 0;
    DWORD maxKeyLen = 0;
    DWORD valueLen = 0;
    DWORD maxValueLen = 0;
    DWORD indx = 0;

    BAIL_ON_NT_STATUS(status);
    
    /*
     * Query info about keys
     */
    hKey = hDb->pMemReg;
    if (pcSubKeys)
    {
        *pcSubKeys = hKey->NodesLen;
    }

    if (pcMaxSubKeyLen)
    {
        for (indx=0, keyLen=0, maxKeyLen; indx < hKey->NodesLen; indx++)
        {
            keyLen = RtlWC16StringNumChars(hKey->SubNodes[indx]->Name);
            if (keyLen > maxKeyLen)
            {
                maxKeyLen = keyLen;
            }
        
        }
        *pcMaxSubKeyLen = maxKeyLen;
    }

    /*
     * Query info about values
     */
    if (pcValues)
    {
        *pcValues = hKey->ValuesLen;
    }

    if (pcMaxValueNameLen)
    {
        for (indx=0, valueLen=0, maxValueLen; indx < hKey->ValuesLen; indx++)
        {
            valueLen = RtlWC16StringNumChars(hKey->Values[indx].Name);
            if (valueLen > maxValueLen)
            {
                maxValueLen = valueLen;
            }
        
        }
        *pcMaxValueNameLen = maxValueLen;
    }

    if (pcMaxValueLen)
    {
        for (indx=0, valueLen=0, maxValueLen; indx < hKey->ValuesLen; indx++)
        {
            valueLen = hKey->Values[indx].DataLen;
            if (valueLen > maxValueLen)
            {
                maxValueLen = valueLen;
            }
        }
    }

cleanup:
    return 0;

error:
     goto cleanup;
}


NTSTATUS
MemDbEnumKeyEx(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN DWORD dwIndex,
    OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pdwReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT PFILETIME pftLastWriteTime
    )
{
    MEM_REG_STORE_HANDLE hKey = NULL;
    NTSTATUS status = 0;
    DWORD keyLen = 0;

    hKey = hDb->pMemReg;
    if (dwIndex >= hKey->NodesLen)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }
    
    /*
     * Query info about keys
     */
    keyLen = RtlWC16StringNumChars(hKey->SubNodes[dwIndex]->Name);
    if (keyLen > *pcName)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pName, hKey->SubNodes[dwIndex]->Name, keyLen * sizeof(WCHAR));
    *pcName = keyLen;

cleanup:
    return status;

error:
    goto cleanup;
}

