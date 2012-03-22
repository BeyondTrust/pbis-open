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
 *        export.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry export utilities (export to a win32 compatible registry file)
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *
 */
#include "regshell.h"

static
DWORD
PrintToRegFile(
    IN FILE* fp,
    IN PCSTR pszKeyName,
    IN OPTIONAL PCSTR pszSddlCString,
    IN REG_DATA_TYPE dataType,
    IN PCSTR pszValueName,
    IN REG_DATA_TYPE type,
    IN BOOLEAN bDefault,
    IN PVOID value,
    IN DWORD dwValueLen,
    OUT PREG_DATA_TYPE pPrevType
    );

static
DWORD
ProcessExportedKeyInfo(
    IN HANDLE hReg,
    PREG_EXPORT_STATE pExportState,
    IN HKEY hKey,
    IN PCSTR pszFullKeyName,
    IN OPTIONAL PCSTR pszSddlCstring,
    IN OUT PREG_DATA_TYPE pPrevType
    );


static
DWORD
ProcessSubKeys(
    HANDLE hReg,
    PREG_EXPORT_STATE pExportState,
    HKEY hKey,
    PCSTR pszFullKeyName,
    DWORD dwNumSubKeys,
    DWORD dwMaxSubKeyLen
    );

static
DWORD
ProcessRootKeys(
    HANDLE hReg,
    PREG_EXPORT_STATE pExportState
    );

DWORD
RegShellUtilExport(
    HANDLE hReg,
    PREG_EXPORT_STATE pExportState,
    HKEY hKey,
    PCSTR pszKeyName,
    DWORD dwNumSubKeys,
    DWORD dwMaxSubKeyLen
    )
{
    DWORD dwError = 0;
    REG_DATA_TYPE prevType = REG_NONE;
    SECURITY_INFORMATION SecInfoAll = OWNER_SECURITY_INFORMATION
                                     |GROUP_SECURITY_INFORMATION
                                     |DACL_SECURITY_INFORMATION
                                     |SACL_SECURITY_INFORMATION;
    PBYTE pSecDescRel = NULL;
    ULONG ulSecDescLen = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;
    PSTR pszStringSecurityDescriptor = NULL;

    dwError = RegGetKeySecurity(hReg,
                                hKey,
                                SecInfoAll,
                                NULL,
                                &ulSecDescLen);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegAllocateMemory(ulSecDescLen, (PVOID)&pSecDescRel);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegGetKeySecurity(hReg,
                                hKey,
                                SecInfoAll,
                                (PSECURITY_DESCRIPTOR_RELATIVE)pSecDescRel,
                                &ulSecDescLen);
    BAIL_ON_REG_ERROR(dwError);

    if (LwRtlCStringIsEqual(pszKeyName,HKEY_THIS_MACHINE,TRUE) ||
        ulSecDescLen != pExportState->ulRootKeySecDescLen ||
        !LwRtlEqualMemory(pSecDescRel, pExportState->pRootKeySecDescRel, ulSecDescLen))
    {
        dwError = RegNtStatusToWin32Error(
                      RtlAllocateSddlCStringFromSecurityDescriptor(
                          &pszStringSecurityDescriptor,
                         (PSECURITY_DESCRIPTOR_RELATIVE)pSecDescRel,
                          SDDL_REVISION_1,
                          SecInfoAll)
                         );
        BAIL_ON_REG_ERROR(dwError);
    }

    if (hKey)
    {
        dwError = ProcessExportedKeyInfo(hReg,
                                         pExportState,
                                         hKey,
                                         pszKeyName,
                                         pExportState->dwExportFormat == 1 ?
                                             NULL : pszStringSecurityDescriptor,
                                         &prevType);
        BAIL_ON_REG_ERROR(dwError);
    }
    if (hKey && dwNumSubKeys != 0)
    {
        dwError = ProcessSubKeys(hReg,
                                 pExportState,
                                 hKey,
                                 pszKeyName,
                                 dwNumSubKeys,
                                 dwMaxSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (hKey == NULL && dwNumSubKeys == 0)
    {
        dwError = ProcessRootKeys(hReg,
                                  pExportState);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (hKey == NULL && dwNumSubKeys != 0)
    {
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pszStringSecurityDescriptor)
    {
        RegFreeString(pszStringSecurityDescriptor);
    }
    LWREG_SAFE_FREE_MEMORY(pSecDescRel);

    if (pSecDescRel)
    {
        RegMemoryFree(pSecDescRel);
        pSecDescRel = NULL;
    }

    return dwError;

error:
    goto cleanup;
}



static
DWORD
PrintToRegFile(
    IN FILE* fp,
    IN PCSTR pszKeyName,
    IN OPTIONAL PCSTR pszSddlCString,
    IN REG_DATA_TYPE dataType,
    IN PCSTR pszValueName,
    IN REG_DATA_TYPE type,
    IN BOOLEAN bDefault,
    IN PVOID value,
    IN DWORD dwValueLen,
    OUT PREG_DATA_TYPE pPrevType
    )
{
    PSTR dumpString = NULL;
    DWORD dumpStringLen = 0;
    PSTR pszStart = NULL;
    PSTR pszEnd = NULL;
    PSTR pszComment = bDefault ? "#" : "";

    RegExportEntry(pszKeyName,
                   pszSddlCString,
                   dataType,
                   pszValueName,
                   type,
                   value,
                   dwValueLen,
                   &dumpString,
                   &dumpStringLen);

   if (dumpStringLen > 0 && dumpString)
   {
       switch (type)
       {
           case REG_KEY:
               fprintf(fp, "\r\n%s%.*s\r\n", 
                       pszComment, dumpStringLen, dumpString);
               break;

           case REG_MULTI_SZ:
               pszStart = dumpString;
               pszEnd = strchr(pszStart, '\n');
               while (pszEnd)
               {
                   fprintf(fp, "%s%.*s\r\n", 
                           pszComment, (int) (pszEnd-pszStart), pszStart);
                   pszStart = pszEnd+1;
                   pszEnd = strchr(pszStart, '\n');
               }
               if (pszStart && *pszStart)
               {
                   fprintf(fp, "%s%s\r\n", 
                           pszComment, pszStart);
               }
               break;

           case REG_PLAIN_TEXT:
               if (*pPrevType && *pPrevType != type)
               {
                   printf("\n");
               }
               fprintf(fp, "%s%*s ", pszComment, dwValueLen, (PCHAR) value);
               break;

           default:
               fprintf(fp, "%s%.*s\r\n", pszComment, dumpStringLen, dumpString);
               break;
       }
   }
   fflush(stdout);
   *pPrevType = type;

   if (dumpString)
   {
       RegMemoryFree(dumpString);
       dumpString = NULL;
   }

   return 0;
}

static
DWORD
ProcessExportedKeyInfo(
    IN HANDLE hReg,
    PREG_EXPORT_STATE pExportState,
    IN HKEY hKey,
    IN PCSTR pszFullKeyName,
    IN OPTIONAL PCSTR pszSddlCstring,
    IN OUT PREG_DATA_TYPE pPrevType
    )
{
    DWORD dwError = 0;
    DWORD dwError2 = 0;
    DWORD dwValueNameLen = MAX_KEY_LENGTH;
    WCHAR pwszValueName[MAX_KEY_LENGTH];   // buffer for subkey name WCHAR format
    PSTR  pszValueName = NULL; // buffer for subkey name
    PSTR  pszValueNameEscaped = NULL; // buffer for subkey name
    PSTR pszValue = NULL;
    REG_DATA_TYPE dataType = REG_NONE;
    BYTE *value = NULL;
    DWORD dwValueLen = 0;
    DWORD dwValueNameEscapedLen = 0;
    DWORD dwValueLenMax = 0;
    int iCount = 0;
    DWORD dwValuesCount = 0;
    PVOID pValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;
    PSTR pszAttrDump = NULL;
    DWORD pszAttrDumpLen = 0;
    PLWREG_CURRENT_VALUEINFO pCurrValueInfo = NULL;
    REG_PARSE_ITEM regItem = {0};
    FILE *fp = pExportState->fp;
    BOOLEAN bValueSet = FALSE;

    dwError = PrintToRegFile(
                          fp,
                          pszFullKeyName,
                          pszSddlCstring,
                          REG_KEY,
                          NULL,
                          REG_KEY,
                          FALSE,
                          NULL,
                          0,
                          pPrevType);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegQueryInfoKeyW(
        hReg,
        hKey,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &dwValuesCount,
        NULL,
        &dwValueLenMax,
        NULL,
        NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwValuesCount)
    {
        goto cleanup;
    }

    dwError = RegAllocateMemory(dwValueLenMax, (PVOID*)&value);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < dwValuesCount; iCount++)
    {
        memset(pwszValueName, 0, MAX_KEY_LENGTH);
        dwValueNameLen = MAX_KEY_LENGTH;
        memset(value, 0, dwValueLenMax);
        dwValueLen = dwValueLenMax;
        RegSafeFreeCurrentValueInfo(&pCurrValueInfo);

        dwError = RegEnumValueW((HANDLE)hReg,
                                hKey,
                                iCount,
                                pwszValueName,
                                &dwValueNameLen,
                                NULL,
                                &dataType,
                                value,
                                &dwValueLen);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_STRING(pszValueName);
        LWREG_SAFE_FREE_STRING(pszValueNameEscaped);
        dwError = RegCStringAllocateFromWC16String(
                      &pszValueName,
                      pwszValueName);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegShellUtilEscapeString(
                      pszValueName,
                      &pszValueNameEscaped,
                      &dwValueNameEscapedLen);
        BAIL_ON_REG_ERROR(dwError);

        if (dataType == REG_SZ)
        {
            LWREG_SAFE_FREE_STRING(pszValue);
            dwError2 = RegCStringAllocateFromWC16String(
                          &pszValue,
                          (PWSTR) value);
            BAIL_ON_REG_ERROR(dwError2);
            pValue = pszValue;
        }
        else
        {
            pValue = value;
        }

        LWREG_SAFE_FREE_MEMORY(pszAttrDump);
        dwError = LwRegGetValueAttributesW(
                      hReg,
                      hKey,
                      NULL,
                      pwszValueName,
                      &pCurrValueInfo,
                      &pValueAttributes);
        if (pExportState->dwExportFormat == REGSHELL_EXPORT_LEGACY)
        {
            bValueSet = pCurrValueInfo || (dwError && dwValueLen) ?
                            FALSE : TRUE;
            /* Export "legacy" format */
            dwError = PrintToRegFile(
                                  fp,
                                  pszFullKeyName,
                                  NULL,
                                  REG_SZ,
                                  pszValueNameEscaped,
                                  dataType,
                                  bValueSet,
                                  pValue,
                                  dwValueLen,
                                  pPrevType);
        }
        else if (pExportState->dwExportFormat == REGSHELL_EXPORT_VALUES)
        {
            /* Export only values that override default */
            if ((dwError == 0 && pCurrValueInfo) ||
                (dwError && dwValueLen))
            {
                memset(&regItem, 0, sizeof(regItem));
                regItem.type = REG_SZ;
                regItem.valueName = pszValueNameEscaped;
                if (pCurrValueInfo)
                {
                    dataType = pCurrValueInfo->dwType;
                    pValue = pCurrValueInfo->pvData;
                    dwValueLen = pCurrValueInfo->cbData;
                } 
                else
                {
                    pValue = value;
                }

                regItem.valueType = dataType;
                regItem.regAttr.ValueType = regItem.valueType;

                if (dataType == REG_SZ)
                {
                    if (pValue)
                    {
                        dwError = RegCStringAllocateFromWC16String(
                                      (PSTR *) &regItem.value,
                                      (PWSTR) pValue);
                        BAIL_ON_REG_ERROR(dwError);
                        regItem.valueLen = strlen(regItem.value);
                    }
                    pValue = regItem.value;
                    dwValueLen = strlen((PSTR) value);
                }
                else
                {
                    regItem.value = pValue;
                    regItem.valueLen = dwValueLen;
                }

                /* Export "legacy" format */
                dwError = PrintToRegFile(
                                      fp,
                                      pszFullKeyName,
                                      NULL,
                                      REG_SZ,
                                      pszValueNameEscaped,
                                      dataType,
                                      FALSE,
                                      pValue,
                                      dwValueLen,
                                      pPrevType);
            }
            else
            {
                /* 
                 * Don't care if there isn't a schema entry when 
                 * exporting values, so ignore error from 
                 * RegGetValueAttributesW().
                 */ 
                dwError = 0;
            }
        }
        else if (dwError == 0)
        {
            /* Export value and attribute information */
            memset(&regItem, 0, sizeof(regItem));
            regItem.type = REG_SZ;
            regItem.valueName = pszValueNameEscaped;
            regItem.valueType = dataType;
            if (pValueAttributes)
            {
                regItem.regAttr = *pValueAttributes;
            }
            /* Might have to do something with MULTI_SZ */
            if (dataType == REG_SZ)
            {
                if (regItem.regAttr.pDefaultValue)
                {
                    dwError = RegCStringAllocateFromWC16String(
                                  (PSTR *) &regItem.regAttr.pDefaultValue,
                                  (PWSTR) regItem.regAttr.pDefaultValue);
                    BAIL_ON_REG_ERROR(dwError);
                    regItem.regAttr.DefaultValueLen =
                        strlen(regItem.regAttr.pDefaultValue);
                    regItem.value = pCurrValueInfo ? pValue : NULL;
                }
            }
            else
            {
                regItem.value = pCurrValueInfo ? value : NULL;
                regItem.valueLen = dwValueLen;
            }
            dwError = RegExportAttributeEntries(
                          &regItem,
                          &pszAttrDump,
                          &pszAttrDumpLen);
            BAIL_ON_REG_ERROR(dwError);
            LWREG_SAFE_FREE_STRING(regItem.regAttr.pDefaultValue);
            fprintf(fp, "%*s\n", pszAttrDumpLen, pszAttrDump);
        }
        else
        {
            /* 
             * Create a "fake" PITEM, populate it with legacy
             * valueName=dataValue information, and use 
             * RegExportAttributeEntries() to generate the export data.
             */
            memset(&regItem, 0, sizeof(regItem));
            regItem.type = REG_SZ;
            regItem.valueName = pszValueNameEscaped;
            regItem.valueType = dataType;
            regItem.regAttr.ValueType = dataType;
            regItem.value = pValue;
            regItem.valueLen = dwValueLen;

            dwError = RegExportAttributeEntries(
                          &regItem,
                          &pszAttrDump,
                          &pszAttrDumpLen);
            BAIL_ON_REG_ERROR(dwError);
            fprintf(fp, "%*s\n", pszAttrDumpLen, pszAttrDump);
        }
   }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pszAttrDump);
    LWREG_SAFE_FREE_MEMORY(pszValue);
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_STRING(pszValueNameEscaped);
    LWREG_SAFE_FREE_STRING(pszValue);
    LWREG_SAFE_FREE_STRING(regItem.regAttr.pDefaultValue);
    RegSafeFreeCurrentValueInfo(&pCurrValueInfo);
    memset(value, 0, dwValueLenMax);
    LWREG_SAFE_FREE_MEMORY(value);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
ProcessSubKeys(
    HANDLE hReg,
    PREG_EXPORT_STATE pExportState,
    HKEY hKey,
    PCSTR pszFullKeyName,
    DWORD dwNumSubKeys,
    DWORD dwMaxSubKeyLen
    )
{
    DWORD dwError = 0;
    int iCount = 0;
    DWORD dwSubKeyLen = dwMaxSubKeyLen+1;
    PWSTR pwszSubKey = NULL;   // buffer for subkey name
    PSTR pszSubKey = NULL;
    PSTR pszFullSubKeyName = NULL;
    HKEY hSubKey = NULL;
    DWORD dwNumSubSubKeys = 0;
    DWORD dwMaxSubSubKeyLen = 0;

    // Get the subkeys and values under this key from registry
    for (iCount = 0; iCount < dwNumSubKeys; iCount++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;

        dwError = RegAllocateMemory(sizeof(*pwszSubKey) * dwSubKeyLen, (PVOID*)&pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegEnumKeyExW((HANDLE)hReg,
                                hKey,
                                iCount,
                                pwszSubKey,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
            hReg,
            hKey,
            pwszSubKey,
            0,
            KEY_READ,
            &hSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegQueryInfoKeyA(
            (HANDLE) hReg,
            hSubKey,
            NULL,
            NULL,
            NULL,
            &dwNumSubSubKeys,
            &dwMaxSubSubKeyLen,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);
        BAIL_ON_REG_ERROR(dwError);

        // Get the pszFullSubKeyName
        dwError = RegCStringAllocateFromWC16String(&pszSubKey, pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCStringAllocatePrintf(
                  &pszFullSubKeyName,
                  "%s\\%s",
                  pszFullKeyName,
                  pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegShellUtilExport(
                        hReg,
                        pExportState,
                        hSubKey,
                        pszFullSubKeyName,
                        dwNumSubSubKeys,
                        dwMaxSubSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);

        if (hSubKey)
        {
            dwError = RegCloseKey((HANDLE) hReg,
                                  hSubKey);
            BAIL_ON_REG_ERROR(dwError);
            hSubKey = NULL;
        }

        LWREG_SAFE_FREE_STRING(pszFullSubKeyName);
        LWREG_SAFE_FREE_STRING(pszSubKey);
        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
        dwNumSubSubKeys = 0;
        dwMaxSubSubKeyLen = 0;
        pszFullSubKeyName = NULL;
        pszSubKey = NULL;
        pwszSubKey = NULL;
    }

cleanup:
    if (hSubKey)
    {
        RegCloseKey((HANDLE) hReg, hSubKey);
        hSubKey = NULL;
    }

    LWREG_SAFE_FREE_STRING(pszFullSubKeyName);
    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    dwNumSubKeys = 0;

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ProcessRootKeys(
    HANDLE hReg,
    PREG_EXPORT_STATE pExportState
    )
{
    DWORD dwError = 0;
    PSTR* ppszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    DWORD iCount = 0;
    HKEY hRootKey = NULL;
    DWORD dwNumSubKeys = 0;
    DWORD dwMaxSubKeyLen = 0;


    dwError = RegEnumRootKeysA(hReg,
                              &ppszRootKeyNames,
                              &dwNumRootKeys);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwNumRootKeys)
    {
        goto cleanup;
    }

    for (iCount = 0; iCount < dwNumRootKeys; iCount++)
    {
        dwError = RegOpenKeyExA((HANDLE) hReg,
                                 NULL,
                                 ppszRootKeyNames[iCount],
                                 0,
                                 KEY_READ,
                                 &hRootKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegQueryInfoKeyA(
            (HANDLE) hReg,
            hRootKey,
            NULL,
            NULL,
            NULL,
            &dwNumSubKeys,
            &dwMaxSubKeyLen,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegShellUtilExport(hReg,
                                     pExportState,
                                     hRootKey,
                                     ppszRootKeyNames[iCount],
                                     dwNumSubKeys,
                                     dwMaxSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);

        if (hRootKey)
        {
            dwError = RegCloseKey((HANDLE) hReg,
                                          hRootKey);
            BAIL_ON_REG_ERROR(dwError);
            hRootKey = NULL;
        }
        dwNumSubKeys = 0;
        dwMaxSubKeyLen = 0;
    }

cleanup:
    RegFreeStringArray(ppszRootKeyNames, dwNumRootKeys);
    if (hRootKey)
    {
       RegCloseKey((HANDLE) hReg, hRootKey);
    }

    return dwError;

error:

    goto cleanup;
}
