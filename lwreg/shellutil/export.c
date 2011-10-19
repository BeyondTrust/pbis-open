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
#include "includes.h"

static
DWORD
ProcessSubKeys(
    HANDLE hReg,
    FILE* fp,
    HKEY hKey,
    PSTR pszFullKeyName,
    DWORD dwNumSubKeys,
    DWORD dwMaxSubKeyLen
    );


DWORD RegExportBinaryTypeToString(
    REG_DATA_TYPE token,
    PSTR tokenStr,
    BOOLEAN dumpFormat)
{
    DWORD dwError = 0;
    static char *typeStrs[][2] = {
        { "hex(0):", "REG_NONE" },
        { "REG_SZ", "REG_SZ"},
        { "hex(2):", "REG_EXPAND_SZ" },
        { "hex:", "REG_BINARY" },
        { "dword:", "REG_DWORD" },
        { "dwordbe:", "REG_DWORD_BIG_ENDIAN" },
        { "link:", "REG_LINK" },
        { "hex(7):", "REG_MULTI_SZ" },
        { "hex(8):", "REG_RESOURCE_LIST" },
        { "hex(9):", "REG_FULL_RESOURCE_DESCRIPTOR" },
        { "hex(a):", "REG_RESOURCE_REQUIREMENTS_LIST" },
        { "hex(b):", "REG_QUADWORD" },
        { "unknown12:", "REG_UNKNOWN12" },
        { "unknown13:", "REG_UNKNOWN13" },
        { "unknown14:", "REG_UNKNOWN14" },
        { "unknown15:", "REG_UNKNOWN15" },
        { "unknown16:", "REG_UNKNOWN16" },
        { "unknown17:", "REG_UNKNOWN17" },
        { "unknown18:", "REG_UNKNOWN18" },
        { "unknown19:", "REG_UNKNOWN19" },
        { "unknown20:", "REG_UNKNOWN20" },
        { "REG_KEY", "REG_KEY" },
        { "REG_KEY_DEFAULT", "REG_KEY_DEFAULT" },
        { "REG_PLAIN_TEXT", "REG_PLAIN_TEXT" },
        { "REG_UNKNOWN", "REG_UNKNOWN" },
        { "sza:", "REG_STRING_ARRAY" }, /* Maps to REG_MULTI_SZ */
    };

    BAIL_ON_INVALID_POINTER(tokenStr);

    if (token < ((sizeof(typeStrs)/sizeof(char *))/2))
    {
        if (dumpFormat)
        {
            strcpy(tokenStr, typeStrs[token][0]);
        }
        else
        {
            strcpy(tokenStr, typeStrs[token][1]);
        }
    }
    else
    {
        sprintf(tokenStr, "ERROR: No Such Token %d", token);
    }


cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RegExportEntry(
    PSTR keyName,
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD dwError = 0;
    switch (type)
    {
        case REG_BINARY:
        case REG_NONE:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_RESOURCE_LIST:
        case REG_FULL_RESOURCE_DESCRIPTOR:
        case REG_RESOURCE_REQUIREMENTS_LIST:
        case REG_QWORD:
            dwError = RegExportBinaryData(valueType,
                                          valueName,
                                          type,
                                          value,
                                          valueLen,
                                          dumpString,
                                          dumpStringLen);
            break;
        case REG_DWORD:
            dwError = RegExportDword(valueType,
                                     valueName,
                                     *((PDWORD) value),
                                     dumpString,
                                     dumpStringLen);
            break;

        case REG_KEY:
            dwError = RegExportRegKey(keyName,
                                      dumpString,
                                      dumpStringLen);
            break;

        case REG_SZ:
            dwError = RegExportString(valueType,
                                      valueName,
                                      value,
                                      dumpString,
                                      dumpStringLen);
            break;
        case REG_PLAIN_TEXT:
        default:
            dwError = RegExportPlainText((PCHAR) value,
                                        dumpString,
                                        dumpStringLen);
    }
    return dwError;
}


DWORD
RegExportDword(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    DWORD value,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(valueName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    /*
     *  "name"=1234ABCD\r\n\0
     *  14: *  ""=1234ABCD\r\n\0
     */
    bufLen = strlen(valueName) + 20;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    if (valueType == REG_KEY_DEFAULT)
    {
        *dumpStringLen = sprintf(dumpBuf, "@=dword:%08x",
                                 value);
    }
    else
    {
        *dumpStringLen = sprintf(dumpBuf, "\"%s\"=dword:%08x",
                                 valueName,
                                 value);
    }

    *dumpString = dumpBuf;
cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegExportRegKey(
    PSTR keyName,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(keyName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    /*
     *  [key_name]\r\n\0
     *  5:  []\r\n\0
     */
    bufLen = strlen(keyName) + 5;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    *dumpStringLen = sprintf(dumpBuf, "[%s]", keyName);
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD RegExportString(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    PSTR value,
    PSTR *dumpString,
    PDWORD retDumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    PSTR valueEscName = NULL;
    DWORD dwError = 0;
    DWORD dumpStringLen = 0;
    DWORD dwEscapeStringLen = 0;

    BAIL_ON_INVALID_POINTER(valueName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(retDumpStringLen);

    /*
     */
    dwError = RegShellUtilEscapeString(
                  value,
                  &valueEscName,
                  &dwEscapeStringLen);
    BAIL_ON_REG_ERROR(dwError);

    bufLen = strlen(valueName) + dwEscapeStringLen + 8;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    if (valueType == REG_KEY_DEFAULT)
    {
        dumpStringLen = sprintf(dumpBuf, "@=\"%s\"",
                            valueEscName);
    }
    else
    {
        dumpStringLen = sprintf(dumpBuf, "\"%s\"=\"%s\"",
                            valueName,
                            valueEscName);
    }
    LWREG_SAFE_FREE_MEMORY(valueEscName);
    *retDumpStringLen = dumpStringLen;
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegExportPlainText(
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    bufLen = strlen(value) + 8;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    *dumpStringLen = sprintf(dumpBuf, "%s", (PCHAR) value);
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegExportBinaryData(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    UCHAR *value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    DWORD formatLines = 0;
    DWORD indx = 0;
    DWORD dwError = 0;
    DWORD linePos = 0;
    PSTR dumpBuf = NULL;
    PSTR fmtCursor = NULL;
    UCHAR *pValue = NULL;
    BOOLEAN firstHex = FALSE;

    CHAR typeName[128];

    RegExportBinaryTypeToString(type, typeName, TRUE);
    /* 5 extra for " "= \\n characters on first line */
    bufLen = strlen(valueName) + strlen(typeName) + 6;

    /* 4 extra characters per line: Prefix "  " spaces, suffix \ and \r\n */
    formatLines = valueLen / 25 + 1;
    bufLen += valueLen * 3 + (formatLines * 5) + 1;

    dwError = RegAllocateMemory(sizeof(*dumpBuf) * bufLen, (PVOID*)&dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    /* Format binary prefix */
    fmtCursor = dumpBuf;
    if (valueType == REG_KEY_DEFAULT)
    {
        fmtCursor += sprintf(fmtCursor, "@=%s", typeName);
    }
    else
    {
        fmtCursor += sprintf(fmtCursor, "\"%s\"=%s",
                             valueName, typeName);
    }

    pValue = (UCHAR *) value;
    linePos = fmtCursor - dumpBuf;
    indx = 0;
    while (indx < valueLen)
    {
        while(((linePos + 3)<REGEXPORT_LINE_WIDTH && indx<valueLen) ||
               !firstHex)
        {
            firstHex = TRUE;
            fmtCursor += sprintf(fmtCursor, "%02x,", pValue[indx]);
            linePos += 3;
            indx++;
        }
        if (indx < valueLen)
        {
            fmtCursor += sprintf(fmtCursor, "\\\r\n  ");
            linePos = 2;
        }
        else
        {
            fmtCursor[-1] = '\0';
            linePos = 0;
        }

    }

    *dumpString = dumpBuf;
    *dumpStringLen = fmtCursor - dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
PrintToRegFile(
    IN FILE* fp,
    IN PSTR pszKeyName,
    IN REG_DATA_TYPE dataType,
    IN PSTR pszValueName,
    IN REG_DATA_TYPE type,
    IN PVOID value,
    IN DWORD dwValueLen,
    OUT PREG_DATA_TYPE pPrevType
    )
{
    PSTR dumpString = NULL;
    DWORD dumpStringLen = 0;

    RegExportEntry(pszKeyName,
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
    IN PSTR pszFullKeyName,
    IN OUT PREG_DATA_TYPE pPrevType
    )
{
    DWORD dwError = 0;
    DWORD dwValueNameLen = MAX_KEY_LENGTH;
    CHAR valueName[MAX_KEY_LENGTH];   // buffer for subkey name
    REG_DATA_TYPE dataType = REG_UNKNOWN;
    BYTE value[MAX_VALUE_LENGTH] = {0};
    DWORD dwValueLen = 0;
    int iCount = 0;
    DWORD dwValuesCount = 0;

    dwError = PrintToRegFile(
                          fp,
                          pszFullKeyName,
                          REG_KEY,
                          NULL,
                          REG_KEY,
                          NULL,
                          0,
                          pPrevType);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegQueryInfoKeyA(
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
       memset(valueName, 0, MAX_KEY_LENGTH);
       dwValueNameLen = MAX_KEY_LENGTH;
       memset(value, 0, MAX_VALUE_LENGTH);
       dwValueLen = MAX_VALUE_LENGTH;

       dwError = RegEnumValueA((HANDLE)hReg,
                               hKey,
                               iCount,
                               valueName,
                               &dwValueNameLen,
                               NULL,
                               &dataType,
                               value,
                               &dwValueLen);
       BAIL_ON_REG_ERROR(dwError);

       dwError = PrintToRegFile(
                      fp,
                      pszFullKeyName,
                      dataType,
                      valueName,
                      dataType,
                      value,
                      dwValueLen,
                      pPrevType);
       BAIL_ON_REG_ERROR(dwError);
   }

cleanup:
    memset(valueName, 0 , MAX_KEY_LENGTH);
    memset(value, 0 , MAX_KEY_LENGTH);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ProcessSubKeys(
    HANDLE hReg,
    FILE* fp,
    HKEY hKey,
    PSTR pszFullKeyName,
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
                        fp,
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
    FILE* fp
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
                                     fp,
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

DWORD
RegShellUtilExport(
    HANDLE hReg,
    FILE* fp,
    HKEY hKey,
    PSTR pszKeyName,
    DWORD dwNumSubKeys,
    DWORD dwMaxSubKeyLen
    )
{
    DWORD dwError = 0;
    REG_DATA_TYPE prevType = REG_UNKNOWN;

    if (hKey)
    {
        dwError = ProcessExportedKeyInfo(hReg,
                                         fp,
                                         hKey,
                                         pszKeyName,
                                         &prevType);
        BAIL_ON_REG_ERROR(dwError);
    }
    if (hKey && dwNumSubKeys != 0)
    {
        dwError = ProcessSubKeys(hReg,
                                 fp,
                                 hKey,
                                 pszKeyName,
                                 dwNumSubKeys,
                                 dwMaxSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (hKey == NULL && dwNumSubKeys == 0)
    {
        dwError = ProcessRootKeys(hReg,
                                  fp);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (hKey == NULL && dwNumSubKeys != 0)
    {
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    goto cleanup;
}
