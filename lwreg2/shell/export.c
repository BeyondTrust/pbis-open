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
    IN PVOID value,
    IN DWORD dwValueLen,
    OUT PREG_DATA_TYPE pPrevType
    );

static
DWORD
ProcessExportedKeyInfo(
    IN HANDLE hReg,
    IN FILE* fp,
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
    PBYTE pSecDescRel[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE] = {0};
    ULONG ulSecDescLen = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;
    PSTR pszStringSecurityDescriptor = NULL;


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
                                         pExportState->fp,
                                         hKey,
                                         pszKeyName,
                                         pszStringSecurityDescriptor,
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
    IN PVOID value,
    IN DWORD dwValueLen,
    OUT PREG_DATA_TYPE pPrevType
    )
{
    PSTR dumpString = NULL;
    DWORD dumpStringLen = 0;

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
               fprintf(fp, "\r\n%.*s\r\n", dumpStringLen, dumpString);
               break;

           case REG_PLAIN_TEXT:
               if (*pPrevType && *pPrevType != type)
               {
                   printf("\n");
               }
               fprintf(fp, "%*s ", dwValueLen, (PCHAR) value);
               break;

           default:
               fprintf(fp, "%.*s\r\n", dumpStringLen, dumpString);
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
    IN FILE* fp,
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
    PSTR pszValue = NULL;
    REG_DATA_TYPE dataType = REG_NONE;
    BYTE value[MAX_VALUE_LENGTH * 2] = {0};
    DWORD dwValueLen = 0;
    int iCount = 0;
    DWORD dwValuesCount = 0;
    PVOID pValue = NULL;
    PLWREG_VALUE_ATTRIBUTES pValueAttributes = NULL;
    PSTR pszAttrDump = NULL;
    DWORD pszAttrDumpLen = 0;

    dwError = PrintToRegFile(
                          fp,
                          pszFullKeyName,
                          pszSddlCstring,
                          REG_KEY,
                          NULL,
                          REG_KEY,
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
        NULL,
        NULL,
        NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwValuesCount)
    {
        goto cleanup;
    }

    for (iCount = 0; iCount < dwValuesCount; iCount++)
    {
        memset(pwszValueName, 0, MAX_KEY_LENGTH);
        dwValueNameLen = MAX_KEY_LENGTH;
        memset(value, 0, MAX_VALUE_LENGTH);
        dwValueLen = MAX_VALUE_LENGTH;

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
        dwError = RegCStringAllocateFromWC16String(
                      &pszValueName,
                      pwszValueName);
        BAIL_ON_REG_ERROR(dwError);

        /*
         * Trouble here. Can't enumerate schema entries that don't have
         * a "value" set. There is no RegEnumAttributesW() API.
         * Since most values will be derived from "default" an
         * export/rm registry.db import would wipe the schema.
         */
        dwError = LwRegGetValueAttributesW(
                      hReg,
                      hKey,
                      NULL,
                      pwszValueName,
                      NULL,
                      &pValueAttributes);
        if (dataType == REG_SZ)
        {
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
        if (dwError == 0)
        {
            REG_PARSE_ITEM regItem = {0};
            regItem.type = REG_SZ;
            regItem.valueName = pszValueName;
            regItem.valueType = dataType;
            regItem.regAttr = *pValueAttributes;
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
                    regItem.value = pValue;
                }
            }
            else
            {
                regItem.value = value;
                regItem.valueLen = dwValueLen;
            }
            dwError = RegExportAttributeEntries(
                          &regItem,
                          &pszAttrDump,
                          &pszAttrDumpLen);
            BAIL_ON_REG_ERROR(dwError);
            fprintf(fp, "%*s\n", pszAttrDumpLen, pszAttrDump);
        }
        else
        {
            dwError = PrintToRegFile(
                           fp,
                           pszFullKeyName,
                           pszSddlCstring,
                           dataType,
                           pszValueName,
                           dataType,
                           pValue,
                           dwValueLen,
                           pPrevType);
            BAIL_ON_REG_ERROR(dwError);
            LWREG_SAFE_FREE_STRING(pszValueName);
            LWREG_SAFE_FREE_STRING(pszValue);
        }
   }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pszValue);
    memset(value, 0 , MAX_KEY_LENGTH);

    return dwError;

error:
    LWREG_SAFE_FREE_STRING(pszValueName);
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
