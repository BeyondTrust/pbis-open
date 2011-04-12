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
#if 0
#define __MEMDB_PRINTF__ 1
#endif
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

    MemDbStartExportToFileThread();
    *phDb = pConn;

cleanup:
    return status;

error:
    goto cleanup;
}

static void *pfDeleteNodeCallback(
    MEM_REG_STORE_HANDLE pEntry,
    PVOID userContext,
    PWSTR subStringPrefix)
{
    MemRegStoreDeleteNode(pEntry);

    return NULL;
}


void *
pfMemRegExportToFile(
    MEM_REG_STORE_HANDLE pEntry, 
    PVOID userContext,
    PWSTR subStringPrefix)
{
    DWORD dwError = 0;
    PSTR pszDumpString = NULL;
    PSTR pszValueName = NULL;
    PSTR pszEnumValue = NULL;
    PSTR pszStringSecurityDescriptor = NULL;
    SECURITY_INFORMATION SecInfoAll = OWNER_SECURITY_INFORMATION
                                     |GROUP_SECURITY_INFORMATION
                                     |DACL_SECURITY_INFORMATION
                                     |SACL_SECURITY_INFORMATION;
    DWORD dwDumpStringLen = 0;
    DWORD index = 0;
    DWORD enumIndex = 0;
    DWORD valueType = 0;
    PREGMEM_VALUE Value = NULL;
    PLWREG_VALUE_ATTRIBUTES Attr = NULL;
    PMEMDB_FILE_EXPORT_CTX exportCtx = (PMEMDB_FILE_EXPORT_CTX) userContext;
    FILE *wfp = exportCtx->wfp;

    /* Format key first */
    dwError = RegExportEntry(
                  (PSTR) subStringPrefix,
                  "", // PCSTR pszSddlCString,
                  0, //     REG_DATA_TYPE valueType,
                  NULL, //     PCSTR valueName,
                  REG_WKEY, // DWORD Type
                  NULL, // LW_PVOID value,
                  0, //     DWORD valueLen,
                  &pszDumpString,
                  &dwDumpStringLen);
    /* Map to NT error status ? */
    BAIL_ON_NT_STATUS(dwError);

    fprintf(wfp, "%.*s\n", dwDumpStringLen, pszDumpString);
    LWREG_SAFE_FREE_STRING(pszDumpString);

    if ((pEntry->NodeType == REGMEM_TYPE_KEY ||
        pEntry->NodeType == REGMEM_TYPE_HIVE) &&
        pEntry->SecurityDescriptorAllocated)
    {
        dwError = RegNtStatusToWin32Error(
                     RtlAllocateSddlCStringFromSecurityDescriptor(
                         &pszStringSecurityDescriptor,
                         pEntry->SecurityDescriptor,
                         SDDL_REVISION_1,
                         SecInfoAll));
        BAIL_ON_NT_STATUS(dwError);
        fprintf(wfp, "@security=%s\n", pszStringSecurityDescriptor);
        LWREG_SAFE_FREE_STRING(pszStringSecurityDescriptor);
    }

    if (pEntry->Values)
    {
        /* Iterate through all values */
        for (index=0; index<pEntry->ValuesLen; index++)
        {
            Value = pEntry->Values[index];

            /* Fix up string type, as value is PWSTR */
            if (Value->Type == REG_SZ)
            {
                valueType = REG_WSZ;
            }
            else
            {
                valueType = Value->Type;
            }
            LwRtlCStringAllocateFromWC16String(
                &pszValueName, 
                Value->Name);
            fprintf(wfp, "\"%s\" = {\n", pszValueName);
            LWREG_SAFE_FREE_STRING(pszValueName);
      
            /* Deal with an override value first */
            if (Value->Data && Value->DataLen)
            {
                dwError = RegExportEntry(
                              NULL,
                              "", // PCSTR pszSddlCString
                              REG_SZ, // valueName type
                              "value",
                              valueType,
                              Value->Data,
                              Value->DataLen,
                              &pszDumpString,
                              &dwDumpStringLen);
                /* Map to NT error status ? */
                BAIL_ON_NT_STATUS(dwError);

                fprintf(wfp, "\t%.*s\n", dwDumpStringLen, pszDumpString);
                LWREG_SAFE_FREE_STRING(pszDumpString);
            }

            /* Deal with default values now */
            Attr = &Value->Attributes;
            if (Attr->pDefaultValue && Attr->DefaultValueLen)
            {
                dwError = RegExportEntry(
                              NULL,
                              "", // PCSTR pszSddlCString
                              REG_SZ, // valueName type
                              "default",
                              valueType,
                              Attr->pDefaultValue,
                              Attr->DefaultValueLen,
                              &pszDumpString,
                              &dwDumpStringLen);
                /* Map to NT error status ? */
                BAIL_ON_NT_STATUS(dwError);
    
                fprintf(wfp, "\t%.*s\n", dwDumpStringLen, pszDumpString);
                LWREG_SAFE_FREE_STRING(pszDumpString);
            }
 
            if (Attr->pwszDocString &&
                LwRtlWC16StringNumChars(Attr->pwszDocString))
            {
                dwError = RegExportEntry(
                              NULL,
                              "", // PCSTR pszSddlCString
                              REG_SZ, // valueName type
                              "doc",
                              REG_WSZ,
                              Attr->pwszDocString,
                              LwRtlWC16StringNumChars(Attr->pwszDocString),
                              &pszDumpString,
                              &dwDumpStringLen);
                /* Map to NT error status ? */
                BAIL_ON_NT_STATUS(dwError);
    
                fprintf(wfp, "\t%.*s\n", dwDumpStringLen, pszDumpString);
                LWREG_SAFE_FREE_STRING(pszDumpString);
            }

            switch (Attr->RangeType)
            {
                case LWREG_VALUE_RANGE_TYPE_BOOLEAN:
                    fprintf(wfp, "\t\"range\"=boolean\n");
                    break;

                case LWREG_VALUE_RANGE_TYPE_ENUM:
                    fprintf(wfp, "\trange=string:");
                    for (enumIndex=0; 
                         Attr->Range.ppwszRangeEnumStrings[enumIndex];
                         enumIndex++)
                    {
                        LwRtlCStringAllocateFromWC16String(
                             &pszEnumValue,
                             Attr->Range.ppwszRangeEnumStrings[enumIndex]);
                        fprintf(wfp, "%s\"%s\"", 
                               enumIndex == 0 ? "" : "\t\t", 
                               pszEnumValue);
                        LWREG_SAFE_FREE_MEMORY(pszEnumValue);

                        if (Attr->Range.ppwszRangeEnumStrings[enumIndex+1])
                        {
                            fprintf(wfp, " \\\n");
                        }
                    }
                    fprintf(wfp, "\n");
                    break;

                case LWREG_VALUE_RANGE_TYPE_INTEGER:
                    fprintf(wfp, "\t\"range\" = integer:%d-%d\n",
                        Attr->Range.RangeInteger.Min,
                        Attr->Range.RangeInteger.Max);
                           
                    break;
                default:
                    break;
            }

            LWREG_SAFE_FREE_STRING(pszDumpString);
            fprintf(wfp, "}\n");
        }
    }
    fprintf(wfp, "\n");
cleanup:
    LWREG_SAFE_FREE_STRING(pszStringSecurityDescriptor);
    LWREG_SAFE_FREE_STRING(pszDumpString);
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_MEMORY(pszEnumValue);
    return NULL;

error:
    goto cleanup;
}


PVOID
MemDbExportToFileThread(
    PVOID ctx)
{
    PMEMDB_FILE_EXPORT_CTX exportCtx = (PMEMDB_FILE_EXPORT_CTX) ctx;
    struct timespec timeOut = {0};
    pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
    REG_DB_CONNECTION regDbConn = {0};

    regDbConn.pMemReg = exportCtx->hKey;
    do
    {
        clock_gettime(CLOCK_REALTIME, &timeOut);
        timeOut.tv_sec += 30;  // Make configurable in the registry :/
        pthread_cond_timedwait(&cv, &mx, &timeOut);

        if (ghMemRegRoot)
        {
            exportCtx->wfp = fopen(MEMDB_EXPORT_FILE, "w");
            if (!exportCtx->wfp)
            {
                // Log error could not open file
            }
            else
            {
                pthread_mutex_lock(&gMemRegDbMutex);
                MemDbRecurseRegistry(
                             NULL,
                             &regDbConn,
                             NULL,
                             pfMemRegExportToFile,
                             exportCtx);
                pthread_mutex_unlock(&gMemRegDbMutex);
                fclose(exportCtx->wfp);
            }
        }
    } while (!exportCtx->bStopThread);
    
    return NULL;
}


NTSTATUS
MemDbExportToFile(
    PSTR exportFile)
{
    NTSTATUS status = 0;
    MEMDB_FILE_EXPORT_CTX exportCtx = {0};
    REG_DB_CONNECTION regDbConn = {0};

    exportCtx.hKey = ghMemRegRoot;
    exportCtx.wfp = fopen(exportFile, "w");
    regDbConn.pMemReg = exportCtx.hKey;
    if (!exportCtx.wfp)
    {
        status = STATUS_OPEN_FAILED;
        BAIL_ON_NT_STATUS(status);
    }
    
    status = MemDbRecurseRegistry(
                 NULL,
                 &regDbConn,
                 NULL,
                 pfMemRegExportToFile,
                 &exportCtx);
    BAIL_ON_NT_STATUS(status);

cleanup:
    if (exportCtx.wfp)
    {
        fclose(exportCtx.wfp);
    }
error:
    goto cleanup;

}


VOID
MemDbStartExportToFileThread(VOID)
{
    NTSTATUS status = 0;
    PMEMDB_FILE_EXPORT_CTX exportCtx = {0};
    PWSTR pwszRootKey = NULL;
    pthread_t hThread;

    status = LW_RTL_ALLOCATE(
                 (PVOID*) &exportCtx,
                 PMEMDB_FILE_EXPORT_CTX,
                 sizeof(MEMDB_FILE_EXPORT_CTX));
    if (status)
    {
        goto cleanup;
    }

    exportCtx->hKey = ghMemRegRoot;
    pthread_create(&hThread, NULL, MemDbExportToFileThread, (PVOID) exportCtx);
    pthread_detach(hThread);

cleanup:
    if (status)
    {
        LWREG_SAFE_FREE_MEMORY(exportCtx);
    }
    LWREG_SAFE_FREE_MEMORY(pwszRootKey);
    return;
}


DWORD pfImportFile(PREG_PARSE_ITEM pItem, HANDLE userContext)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};
    PWSTR pwszSubKey = NULL;
    PWSTR pwszValueName = NULL;
    PWSTR pwszStringData = NULL;
    PMEMDB_IMPORT_FILE_CTX pImportCtx = (PMEMDB_IMPORT_FILE_CTX) userContext;
    PVOID pData = NULL;
    DWORD dwDataLen = 0;
    PREGMEM_VALUE pRegValue = NULL;
    DWORD dataType = 0;
    LWREG_VALUE_ATTRIBUTES tmpAttr = {0};

    if (pItem->type == REG_KEY)
    {
        regDbConn.pMemReg = ghMemRegRoot;

        // Open subkeys that exist
        status = LwRtlWC16StringAllocateFromCString(
                     &pwszSubKey,
                     pItem->keyName);
        BAIL_ON_NT_STATUS(status);

        status = MemDbOpenKey(
                      NULL,
                      &regDbConn,
                      pwszSubKey,
                      &hSubKey);
        if (status == 0)
        {
            regDbConn.pMemReg = hSubKey;
            pImportCtx->hSubKey = hSubKey;
        }

        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            status = MemDbCreateKeyEx(
                         NULL,
                         &regDbConn,
                         pwszSubKey,
                         0, // IN DWORD dwReserved,
                         NULL,  // IN OPTIONAL PWSTR pClass,
                         0, // IN DWORD dwOptions,
                         0, // IN ACCESS_MASK 
                         NULL, // IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE 
                         0, // IN ULONG ulSecDescLength,
                         &hSubKey,
                         NULL); // OUT OPTIONAL PDWORD pdwDisposition
            BAIL_ON_NT_STATUS(status);
            pImportCtx->hSubKey = hSubKey;
        }
        else
        {
            BAIL_ON_NT_STATUS(status);
        }

#ifdef __MEMDB_PRINTF__ /* Debugging only */
printf("pfImportFile: type=%d valueName=%s\n",
       pItem->type,
       pItem->keyName);
#endif
    }
    else if (pItem->valueType == REG_KEY_DEFAULT &&
             !strcmp(pItem->valueName, "@security"))
    {
        hSubKey = pImportCtx->hSubKey;

        /* Add security descriptor to current key node */
        if (pItem->value && pItem->valueLen)
        {
            status = RtlAllocateSecurityDescriptorFromSddlCString(
                         &hSubKey->SecurityDescriptor,
                         &hSubKey->SecurityDescriptorLen,
                         pItem->value,
                         SDDL_REVISION_1);
            BAIL_ON_NT_STATUS(status);
            hSubKey->SecurityDescriptorAllocated = TRUE;
        }
    }
    else 
    {
        status = LwRtlWC16StringAllocateFromCString(
                     &pwszValueName,
                     pItem->valueName);
        BAIL_ON_NT_STATUS(status);
        if (pItem->type == REG_SZ || pItem->type == REG_DWORD ||
            pItem->type == REG_BINARY || pItem->type == REG_MULTI_SZ)
        {
    
            if (pItem->type == REG_SZ)
            {
                status = LwRtlWC16StringAllocateFromCString(
                             &pwszStringData,
                             pItem->value);
                BAIL_ON_NT_STATUS(status);
                pData = pwszStringData;
                dwDataLen = wc16slen(pwszStringData) * 2 + 2;
                dataType = REG_SZ;
            }
            else
            {
                pData = pItem->value,
                dwDataLen = pItem->valueLen;
                dataType = pItem->type;
            }
            status = MemRegStoreAddNodeValue(
                         pImportCtx->hSubKey,
                         pwszValueName,
                         0, // Not used?
                         dataType,
                         pData,
                         dwDataLen);
            BAIL_ON_NT_STATUS(status);
        }
        else if (pItem->type == REG_ATTRIBUTES)
        {
            dataType = pItem->regAttr.ValueType;
            if (pItem->value && pItem->valueLen)
            {
                pData = pItem->value;
                dwDataLen = pItem->valueLen;

                if (dataType == REG_SZ)
                {
                    LWREG_SAFE_FREE_MEMORY(pwszStringData);
                    status = LwRtlWC16StringAllocateFromCString(
                                     &pwszStringData,
                                     pData);
                    BAIL_ON_NT_STATUS(status);
                    dwDataLen = wc16slen(pwszStringData) * 2 + 2;
                    pData = pwszStringData;
                }
            }
            status = MemRegStoreAddNodeValue(
                         pImportCtx->hSubKey,
                         pwszValueName,
                         0, // Not used?
                         dataType,
                         pData,
                         dwDataLen);
            BAIL_ON_NT_STATUS(status);
            status = MemRegStoreFindNodeValue(
                         pImportCtx->hSubKey,
                         pwszValueName,
                         &pRegValue);
            BAIL_ON_NT_STATUS(status);
            tmpAttr = pItem->regAttr;
            if (tmpAttr.ValueType == REG_SZ)
            {
                status = LwRtlWC16StringAllocateFromCString(
                                     &pwszStringData,
                                 pItem->regAttr.pDefaultValue);
                BAIL_ON_NT_STATUS(status);
                tmpAttr.pDefaultValue = (PVOID) pwszStringData;
                tmpAttr.DefaultValueLen = wc16slen(tmpAttr.pDefaultValue) * 2 + 2;
            }
            status = MemRegStoreAddNodeAttribute(
                         pRegValue,
                         &tmpAttr);
            BAIL_ON_NT_STATUS(status);
        }
    }

#ifdef __MEMDB_PRINTF__  /* Debug printf output */
char *subKey = NULL;
LwRtlCStringAllocateFromWC16String(&subKey, pImportCtx->hSubKey->Name);
        printf("pfImportFile: type=%d subkey=[%s] valueName=%s\n",
                pItem->type,
                subKey,
                pItem->valueName ? pItem->valueName : "");
LWREG_SAFE_FREE_STRING(subKey);
#endif

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszStringData);
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    return status;
error:
    goto cleanup;
}

NTSTATUS
MemDbImportFromFile(
    PSTR pszImportFile,
    PFN_REG_CALLBACK pfCallback,
    HANDLE userContext)
{
    DWORD dwError = 0;
    HANDLE parseH = NULL;

    if (access(pszImportFile, R_OK) == -1)
    {
        return 0;
    }
    dwError = RegParseOpen(
                  pszImportFile,
                  pfCallback,
                  userContext,
                  &parseH);
    dwError = RegParseRegistry(parseH);
    BAIL_ON_REG_ERROR(dwError);

    RegParseClose(parseH);

cleanup:
    return 0;

error:
    goto cleanup;
}



NTSTATUS
MemDbClose(
    IN REG_DB_HANDLE hDb)
{
    NTSTATUS status = 0;

    status = MemDbRecurseDepthFirstRegistry(
                 NULL,
                 hDb,
                 NULL,
                 pfDeleteNodeCallback,
                 NULL);
    BAIL_ON_NT_STATUS(status);

    status = MemRegStoreClose(hDb->pMemReg);
    BAIL_ON_NT_STATUS(status);

cleanup:
    return status;

error:
    goto cleanup;
}


PWSTR
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
     
    status = LwRtlWC16StringDuplicate(&pwszTmpFullPath, pwszFullKeyPath);
    BAIL_ON_NT_STATUS(status);

    if (!hDb)
    {
        hParentKey = ghMemRegRoot;
    }
    else
    {
        hParentKey = hDb->pMemReg;
    }
    pwszSubKey = pwszTmpFullPath;
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
    PWSTR pwszTmpFullPath = NULL;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszPtr = NULL;
    BOOLEAN bEndOfString = FALSE;
    
    /*
     * Iterate over subkeys in \ sepearated path.
     */
    status = LwRtlWC16StringDuplicate(&pwszTmpFullPath, pcwszSubKey);
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
        if (status == 0)
        {
            hParentKey = hSubKey;
            *phSubKey = hParentKey;
        }
        pwszSubKey = pwszPtr;

        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            /* New node for current subkey, add it */
            status = MemRegStoreAddNode(
                             hParentKey,
                             pwszSubKey,
                             REGMEM_TYPE_KEY,
                             pSecDescRel,  // SD parameter
                             ulSecDescLength,
                             NULL,
                             &hParentKey);
            BAIL_ON_NT_STATUS(status);
            *phSubKey = hParentKey;
        }
    } while (status == 0 && !bEndOfString);

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
    BOOLEAN bLocked = FALSE;

    BAIL_ON_NT_STATUS(status);
    
    /*
     * Query info about keys
     */
    pthread_mutex_lock(&gMemRegDbMutex);
    bLocked = TRUE;

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
            valueLen = RtlWC16StringNumChars(hKey->Values[indx]->Name);
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
            valueLen = hKey->Values[indx]->DataLen;
            if (valueLen > maxValueLen)
            {
                maxValueLen = valueLen;
            }
        }
        *pcMaxValueLen = maxValueLen;
    }
    if (pcbSecurityDescriptor)
    {
        *pcbSecurityDescriptor = hKey->SecurityDescriptorLen;
    }

cleanup:
    if (bLocked)
    {
        pthread_mutex_unlock(&gMemRegDbMutex);
    }
    return status;

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
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    DWORD keyLen = 0;
    BOOLEAN bLocked = FALSE;

    pthread_mutex_lock(&gMemRegDbMutex);
    bLocked = TRUE;

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
    if (bLocked)
    {
        pthread_mutex_unlock(&gMemRegDbMutex);
    }
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbSetValueEx(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    PREGMEM_VALUE pRegValue = NULL;

    BAIL_ON_NT_STATUS(status);

    hKey = hDb->pMemReg;


    status = MemRegStoreFindNodeValue(
                 hKey,
                 pValueName,
                 &pRegValue);
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {

        status = MemRegStoreAddNodeValue(
                     hKey,
                     pValueName,
                     dwReserved, // Not used?
                     dwType,
                     pData,
                     cbData);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        /* Modify existing node value */
        status = MemRegStoreChangeNodeValue(
                     pRegValue,
                     pData,
                     cbData);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbGetValue(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    MEM_REG_STORE_HANDLE hParentKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    PREGMEM_VALUE hValue = NULL;

    hKey = hDb->pMemReg;

    if (pSubKey)
    {
        /*
         * Find named subnode and use that to find the named value
         */
        hParentKey = hKey;
        status = MemRegStoreFindNodeSubkey(
                     hParentKey,
                     pSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKey = hSubKey;
    }


    /*
     * Find named value within specified node
     */
    status = MemRegStoreFindNodeValue(
                 hKey,
                 pValueName,
                 &hValue);
    BAIL_ON_NT_STATUS(status);

    /*
     * Return data from value node. Storage for return values is
     * passed into this function by the caller.
     */
    *pdwType = hValue->Type;
    if (pcbData)
    {
        if (hValue->DataLen)
        {
            *pcbData = hValue->DataLen;
        }
        else if (hValue->Attributes.DefaultValueLen)
        {
            *pcbData = hValue->Attributes.DefaultValueLen;
        }
          
    }
    if (pData && pcbData)
    {
        if (hValue->Data && hValue->DataLen)
        {
            memcpy(pData, hValue->Data, hValue->DataLen);
        }
        else if (hValue->Attributes.pDefaultValue)
        {
            memcpy(pData,
                   hValue->Attributes.pDefaultValue,
                   hValue->Attributes.DefaultValueLen);
        }
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbEnumValue(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pdwReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    DWORD valueLen = 0;
    BOOLEAN bLocked = FALSE;

    hKey = hDb->pMemReg;
    if (dwIndex >= hKey->ValuesLen)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }
    
    /*
     * Query info about indexed value
     */
    valueLen = RtlWC16StringNumChars(hKey->Values[dwIndex]->Name);
    if (valueLen > *pcchValueName)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    pthread_mutex_lock(&gMemRegDbMutex);
    bLocked = TRUE;

    memcpy(pValueName, hKey->Values[dwIndex]->Name, valueLen * sizeof(WCHAR));
    *pcchValueName = valueLen;
    if (pType)
    {
        *pType = hKey->Values[dwIndex]->Type;
    }
    if (pcbData)
    {
        if (hKey->Values[dwIndex]->Data && hKey->Values[dwIndex]->DataLen)
        {
            *pcbData = hKey->Values[dwIndex]->DataLen;
            if (pData && hKey->Values[dwIndex]->Data)
            {
                memcpy(pData, 
                       hKey->Values[dwIndex]->Data,
                       hKey->Values[dwIndex]->DataLen);
            }
        }
        else if (hKey->Values[dwIndex]->Attributes.pDefaultValue &&
                 hKey->Values[dwIndex]->Attributes.DefaultValueLen > 0)
        {
            *pcbData = hKey->Values[dwIndex]->Attributes.DefaultValueLen;
            if (pData && hKey->Values[dwIndex]->Attributes.pDefaultValue)
            {
                memcpy(pData, 
                       hKey->Values[dwIndex]->Attributes.pDefaultValue,
                       hKey->Values[dwIndex]->Attributes.DefaultValueLen);
            }
        }
    }

cleanup:
    if (bLocked)
    {
        pthread_mutex_unlock(&gMemRegDbMutex);
    }
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbGetKeyAcl(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    OUT PULONG pSecDescLen)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    BOOLEAN bLocked = FALSE;

    BAIL_ON_NT_INVALID_POINTER(hDb);
    hKey = hDb->pMemReg;

    pthread_mutex_lock(&gMemRegDbMutex);
    bLocked = TRUE;
    if (hKey->SecurityDescriptor)
    {
        if (pSecDescLen)
        {
            *pSecDescLen = hKey->SecurityDescriptorLen;
            if (pSecDescRel)
            {
                memcpy(pSecDescRel, hKey->SecurityDescriptor, *pSecDescLen);
            }
        }
    }
cleanup:
    if (bLocked)
    {
        pthread_mutex_unlock(&gMemRegDbMutex);
    }
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbSetKeyAcl(
    IN HANDLE Handle,
    IN REG_DB_HANDLE hDb,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG secDescLen)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    BOOLEAN bLocked = FALSE;

    BAIL_ON_NT_INVALID_POINTER(hDb);
    if (!pSecDescRel || secDescLen == 0)
    {
        goto cleanup;
    }
    BAIL_ON_NT_INVALID_POINTER(pSecDescRel);

    hKey = hDb->pMemReg;
    pthread_mutex_lock(&gMemRegDbMutex);
    bLocked = TRUE;

    if ((hKey->SecurityDescriptor && 
         memcmp(hKey->SecurityDescriptor, pSecDescRel, secDescLen) != 0) ||
        !hKey->SecurityDescriptor)
    {
        status = LW_RTL_ALLOCATE((PVOID*) &hKey->SecurityDescriptor, 
                                 BYTE, 
                                 secDescLen);
        BAIL_ON_NT_STATUS(status);

        memcpy(hKey->SecurityDescriptor, pSecDescRel, secDescLen);
        hKey->SecurityDescriptorLen = secDescLen;
        hKey->SecurityDescriptorAllocated = TRUE;
    }

cleanup:
    if (bLocked)
    {
        pthread_mutex_unlock(&gMemRegDbMutex);
    }
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(hKey->SecurityDescriptor);
    goto cleanup;
}


NTSTATUS
MemDbSetValueAttributes(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    MEM_REG_STORE_HANDLE hParentKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    PREGMEM_VALUE hValue = NULL;

    hKey = hDb->pMemReg;

    if (pSubKey)
    {
        /*
         * Find named subnode and use that to find the named value
         */
        hParentKey = hKey;
        status = MemRegStoreFindNode(
                     hParentKey,
                     pSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKey = hSubKey;
    }

    /*
     * Find named value within specified node
     */
    status = MemRegStoreFindNodeValue(
                 hKey,
                 pValueName,
                 &hValue);
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        status = MemRegStoreAddNodeValue(
                     hKey,
                     pValueName,
                     0, // Not used?
                     pValueAttributes->ValueType,
                     NULL,
                     0);
        BAIL_ON_NT_STATUS(status);  
    }
    status = MemRegStoreFindNodeValue(
                 hKey,
                 pValueName,
                 &hValue);
    BAIL_ON_NT_STATUS(status);

    /*
     * Add attributes to the specified node.
     */
    status = MemRegStoreAddNodeAttribute(
                 hValue,
                 pValueAttributes);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbGetValueAttributes(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    MEM_REG_STORE_HANDLE hParentKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    PREGMEM_VALUE hValue = NULL;

    hKey = hDb->pMemReg;
    if (pSubKey)
    {
        /*
         * Find named subnode and use that to find the named value
         */
        hParentKey = hKey;
        status = MemRegStoreFindNode(
                     hParentKey,
                     pSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKey = hSubKey;
    }

    /*
     * Find named value within specified node
     */
    status = MemRegStoreFindNodeValue(
                 hKey,
                 pValueName,
                 &hValue);
    BAIL_ON_NT_STATUS(status);

    status = MemRegStoreGetNodeValueAttributes(
                 hValue,
                 ppCurrentValue,
                 ppValueAttributes);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbStackInit(
    DWORD dwSize,
    PMEMDB_STACK *retStack)
{
    NTSTATUS status = 0;
    PMEMDB_STACK newStack = NULL;
    PMEMDB_STACK_ENTRY stackData = NULL;

    status = LW_RTL_ALLOCATE((PVOID*) &newStack,
                              PMEMDB_STACK, 
                              sizeof(MEMDB_STACK));
    BAIL_ON_NT_STATUS(status);
    memset(newStack, 0, sizeof(MEMDB_STACK));

    status = LW_RTL_ALLOCATE((PVOID*) &stackData,
                              PMEMDB_STACK_ENTRY,
                              sizeof(MEMDB_STACK_ENTRY) * dwSize);
    BAIL_ON_NT_STATUS(status);
    memset(stackData, 0, sizeof(MEMDB_STACK_ENTRY) * dwSize);

    newStack->stack = stackData;
    newStack->stackSize = dwSize;
    *retStack = newStack;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(newStack);
    LWREG_SAFE_FREE_MEMORY(stackData);
    goto cleanup;
}


VOID
MemDbStackFinish(
    PMEMDB_STACK hStack)
{
    DWORD index = 0;


    for (index=0; index<hStack->stackSize; index++)
    {
        LWREG_SAFE_FREE_MEMORY(hStack->stack[index].pwszSubKeyPrefix);
    }
    LWREG_SAFE_FREE_MEMORY(hStack->stack);
    LWREG_SAFE_FREE_MEMORY(hStack);
}


NTSTATUS
MemDbStackPush(
    PMEMDB_STACK hStack,
    PREGMEM_NODE node,
    PWSTR pwszPrefix)
{
    NTSTATUS status = 0;
    MEMDB_STACK_ENTRY newNode = {0};
    PWSTR pwszPathPrefix = NULL;

    status = LwRtlWC16StringDuplicate(&pwszPathPrefix, pwszPrefix);
    BAIL_ON_NT_STATUS(status);

    newNode.pNode = node;
    newNode.pwszSubKeyPrefix = pwszPathPrefix;

    if (hStack->stackPtr+1 > hStack->stackSize)
    {
        status = ERROR_STACK_OVERFLOW;
        BAIL_ON_NT_STATUS(status);
    }

    hStack->stack[hStack->stackPtr++] = newNode;

error:
    return status;
}


NTSTATUS
MemDbStackPop(
    PMEMDB_STACK hStack,
    PREGMEM_NODE *pNode,
    PWSTR *ppwszPrefix)
{
    NTSTATUS status = 0;

    if (hStack->stackPtr == 0)
    {
        status = ERROR_EMPTY;
        BAIL_ON_NT_STATUS(status);
    }

    hStack->stackPtr--;
    *pNode = hStack->stack[hStack->stackPtr].pNode;
    *ppwszPrefix = hStack->stack[hStack->stackPtr].pwszSubKeyPrefix;
    hStack->stack[hStack->stackPtr].pNode = NULL;
    hStack->stack[hStack->stackPtr].pwszSubKeyPrefix = NULL;

error:
    return status;
}


NTSTATUS
MemDbRecurseRegistry(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pwszOptSubKey,
    IN PVOID (*pfCallback)(MEM_REG_STORE_HANDLE hKey, 
                           PVOID userContext,
                           PWSTR pwszSubKeyPrefix),
    IN PVOID userContext)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    INT32 index = 0;
    PMEMDB_STACK hStack = 0;
    PWSTR pwszSubKeyPrefix = NULL;
    PWSTR pwszSubKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};


    status = MemDbStackInit(512, &hStack);
    BAIL_ON_NT_STATUS(status);
    hKey = hDb->pMemReg;

    if (pwszOptSubKey)
    {
        regDbConn.pMemReg = hKey;
        status = MemDbOpenKey(
                     hRegConnection,
                     &regDbConn,
                     pwszOptSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKey = hSubKey;
    }

    /* Initially populate stack from top level node */
    if (!pwszOptSubKey)
    {
        for (index=hKey->NodesLen-1; index>=0; index--)
        {
            status = LwRtlWC16StringAllocatePrintf(
                         &pwszSubKeyPrefix,
                         "%ws", hKey->SubNodes[index]->Name);
            BAIL_ON_NT_STATUS(status);
            status = MemDbStackPush(
                         hStack,
                         hKey->SubNodes[index],
                         pwszSubKeyPrefix);
            BAIL_ON_NT_STATUS(status);
        }
    }
    else
    {
        status = LwRtlWC16StringAllocatePrintf(
                     &pwszSubKeyPrefix,
                     "%ws", pwszOptSubKey);
        BAIL_ON_NT_STATUS(status);
        status = MemDbStackPush(
                     hStack,
                     hSubKey,
                     pwszSubKeyPrefix);
        BAIL_ON_NT_STATUS(status);
    }

    do
    {
        status = MemDbStackPop(hStack, &hKey, &pwszSubKeyPrefix);
        if (status == 0)
        {
            pfCallback(hKey, (PVOID) userContext, pwszSubKeyPrefix);
            if (hKey->SubNodes && hKey->NodesLen > 0)
            {
                for (index=hKey->NodesLen-1; index>=0; index--)
                {
                    status = LwRtlWC16StringAllocatePrintf(
                                 &pwszSubKey, 
                                 "%ws\\%ws",
                                 pwszSubKeyPrefix,
                                 hKey->SubNodes[index]->Name);
                    BAIL_ON_NT_STATUS(status);
                    status = MemDbStackPush(
                                 hStack,
                                 hKey->SubNodes[index],
                                 pwszSubKey);
                    BAIL_ON_NT_STATUS(status);
                }
            }
        }
        LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
    } while (status != ERROR_EMPTY);

cleanup:
    if (status == ERROR_EMPTY)
    {
        status = 0;
    }

    MemDbStackFinish(hStack);
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbRecurseDepthFirstRegistry(
    IN HANDLE hRegConnection,
    IN REG_DB_HANDLE hDb,
    IN OPTIONAL PCWSTR pwszOptSubKey,
    IN PVOID (*pfCallback)(MEM_REG_STORE_HANDLE hKey, 
                           PVOID userContext,
                           PWSTR pwszSubKeyPrefix),
    IN PVOID userContext)
{
    NTSTATUS status = 0;
    MEM_REG_STORE_HANDLE hKey = NULL;
    INT32 index = 0;
    PMEMDB_STACK hStack = 0;
    PWSTR pwszSubKeyPrefix = NULL;
    PWSTR pwszSubKey = NULL;
    MEM_REG_STORE_HANDLE hSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};


    status = MemDbStackInit(512, &hStack);
    BAIL_ON_NT_STATUS(status);
    hKey = hDb->pMemReg;

    if (pwszOptSubKey)
    {
        regDbConn.pMemReg = hKey;
        status = MemDbOpenKey(
                     hRegConnection,
                     &regDbConn,
                     pwszOptSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKey = hSubKey;
    }

    /* Initially populate stack from top level node */
    if (!pwszOptSubKey)
    {
        for (index=hKey->NodesLen-1; index>=0; index--)
        {
            status = LwRtlWC16StringAllocatePrintf(
                         &pwszSubKeyPrefix,
                         "%ws", hKey->SubNodes[index]->Name);
            BAIL_ON_NT_STATUS(status);
            status = MemDbStackPush(
                         hStack,
                         hKey->SubNodes[index],
                         pwszSubKeyPrefix);
            BAIL_ON_NT_STATUS(status);
            LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
        }
    }
    else
    {
        status = LwRtlWC16StringAllocatePrintf(
                     &pwszSubKeyPrefix,
                     "%ws", pwszOptSubKey);
        BAIL_ON_NT_STATUS(status);
        status = MemDbStackPush(
                     hStack,
                     hSubKey,
                     pwszSubKeyPrefix);
        BAIL_ON_NT_STATUS(status);
        LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
    }

    do
    {
        status = MemDbStackPop(hStack, &hKey, &pwszSubKeyPrefix);
        if (status == 0)
        {
            if (hKey->SubNodes && hKey->NodesLen > 0)
            {
                /* This push should never fail */
                status = MemDbStackPush(hStack, hKey, pwszSubKeyPrefix);
                BAIL_ON_NT_STATUS(status);
                for (index=hKey->NodesLen-1; index>=0; index--)
                {
                    status = LwRtlWC16StringAllocatePrintf(
                                 &pwszSubKey, 
                                 "%ws\\%ws",
                                 pwszSubKeyPrefix,
                                 hKey->SubNodes[index]->Name);
                    BAIL_ON_NT_STATUS(status);
                    status = MemDbStackPush(
                                 hStack,
                                 hKey->SubNodes[index],
                                 pwszSubKey);
                    BAIL_ON_NT_STATUS(status);
                    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
                    LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
                }
            }
            else
            {
                /* This callback must do something to break the recursion */
                pfCallback(hKey, (PVOID) userContext, pwszSubKeyPrefix);
                LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
            }
        }
        else
        {
            BAIL_ON_NT_STATUS(status);
        }
    } while (status != ERROR_EMPTY);

cleanup:
    if (status == ERROR_EMPTY)
    {
        status = 0;
    }

    MemDbStackFinish(hStack);
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
    goto cleanup;
}
