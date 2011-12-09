/*
 * Copyright Likewise Software    2004-2010
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
 *       regshell.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry Shell application
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#include "regshell.h"
#include <locale.h>
#include <pwd.h>
#include "histedit.h"

#if 0
#define _LW_DEBUG 1
#endif
#define REGSHELL_ESC_CHAR '|'

static int gCaughtSignal;

typedef struct _EDITLINE_CLIENT_DATA
{
    int continuation;
    PREGSHELL_PARSE_STATE pParseState;

    /* File name completion Data */
    PSTR pszCompletePrevCmd;
    PSTR pszCompletePrevArg;
    PSTR *ppszCompleteMatches;
    DWORD dwCompleteMatchesLen;
    DWORD dwEnteredTextLen;
    REGSHELL_TAB_COMPLETION_E ePrevState;
} EDITLINE_CLIENT_DATA, *PEDITLINE_CLIENT_DATA;

void
pfnRegShellSignal(int signal)
{
    gCaughtSignal = signal;
}
  


PSTR
RegShellGetRootKey(
    PREGSHELL_PARSE_STATE pParseState)
{
    return pParseState->pszFullRootKeyName ?
               pParseState->pszFullRootKeyName :
               pParseState->pszDefaultRootKeyName;
}


PSTR
RegShellGetDefaultKey(
    PREGSHELL_PARSE_STATE pParseState)
{
    return pParseState->pszFullKeyPath ?
               pParseState->pszFullKeyPath :
               pParseState->pszDefaultKey;
}


DWORD
RegShellListKeys(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;
    DWORD dwSubKeyLen = 0;
    DWORD i = 0;
    PSTR pszSubKey = NULL;
    LW_WCHAR **ppSubKeys = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    dwError = RegShellUtilGetKeys(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  RegShellGetDefaultKey(pParseState),
                  rsItem->keyName,
                  &ppSubKeys,
                  &dwSubKeyLen);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; i<dwSubKeyLen; i++)
    {
        dwError = LwRtlCStringAllocateFromWC16String(&pszSubKey, ppSubKeys[i]);
        BAIL_ON_REG_ERROR(dwError);

#ifndef _LW_DEBUG
        printf("[%s]\n", pszSubKey);
#else
        printf("SubKey %d name is '%s'\n", i, pszSubKey);
#endif
        LWREG_SAFE_FREE_STRING(pszSubKey);
    }
cleanup:
    if (ppSubKeys)
    {
        for (i=0; i<dwSubKeyLen && ppSubKeys[i]; i++)
        {
            LWREG_SAFE_FREE_MEMORY(ppSubKeys[i]);
        }
    }
    LWREG_SAFE_FREE_MEMORY(ppSubKeys);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellAddKey(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);

    dwError = RegShellUtilAddKey(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  RegShellGetDefaultKey(pParseState),
                  rsItem->keyName,
                  TRUE);

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellDeleteKey(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);

    dwError = RegShellUtilDeleteKey(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  RegShellGetDefaultKey(pParseState),
                  rsItem->keyName);

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellDeleteTree(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;
    PSTR pszKeyName = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);

    pszKeyName = rsItem->keyName;
    if (pszKeyName && pszKeyName[0] == '\\')
    {
        pszKeyName++;
    }

    dwError = RegShellUtilDeleteTree(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  RegShellGetDefaultKey(pParseState),
                  pszKeyName);

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellDeleteValue(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);

    dwError = RegShellUtilDeleteValue(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  RegShellGetDefaultKey(pParseState),
                  rsItem->keyName,
                  rsItem->valueName);
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellSetValue(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{

    LW_PVOID data = NULL;
    DWORD dataLen = 0;
    DWORD dwError = 0;
    DWORD type = 0;
    PSTR pszKeyName = NULL;
    BOOLEAN bFullPath = FALSE;

    pszKeyName = rsItem->keyName;
    if (pszKeyName && pszKeyName[0] == '\\')
    {
        pszKeyName++;
        bFullPath = TRUE;
    }

    dwError = RegShellUtilGetValue(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  !bFullPath ? RegShellGetDefaultKey(pParseState) : NULL,
                  pszKeyName,
                  rsItem->valueName,
                  &type,
                  NULL,
                  NULL);
    if (rsItem->command == REGSHELL_CMD_SET_VALUE)
    {
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        /* Don't allow addition of existing value */
        if (dwError == 0)
        {
            dwError = LWREG_ERROR_DUPLICATE_KEYVALUENAME;
            BAIL_ON_REG_ERROR(dwError);
            
        }
        type = rsItem->type;
    }

    switch (type)
    {
        case REG_MULTI_SZ:
            data = rsItem->args;
            break;
        case REG_SZ:
            data = rsItem->args[0];
            break;
        case REG_DWORD:
        case REG_BINARY:
            data = rsItem->binaryValue;
            dataLen = rsItem->binaryValueLen;
            break;
        default:
            break;
    }
    dwError = RegShellUtilSetValue(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  RegShellGetDefaultKey(pParseState),
                  rsItem->keyName,
                  rsItem->valueName,
                  type,
                  data,
                  dataLen);
cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellDumpByteArray(
    PBYTE pByteArray,
    DWORD dwByteArrayLen)
{
    DWORD i = 0;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pByteArray);

    for (i=0; i<dwByteArrayLen; i++)
    {
        printf("%02x%s", pByteArray[i], (i+1)<dwByteArrayLen ? "," : "");
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellImportFile(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem,
    REGSHELL_UTIL_IMPORT_MODE eMode)
{
    HANDLE parseH = NULL;
    DWORD dwError = 0;
    DWORD lineNum = 0;
    REGSHELL_UTIL_IMPORT_CONTEXT importCtx = {0};
    CHAR cErrorBuf[512] = {0};

    dwError = RegParseOpen(rsItem->args[0], NULL, NULL, &parseH);
    BAIL_ON_REG_ERROR(dwError);

    importCtx.hReg = pParseState->hReg;
    importCtx.eImportMode = eMode;

    dwError = RegParseInstallCallback(
                  parseH,
                  RegShellUtilImportCallback,
                  &importCtx,
                  NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegParseRegistry(parseH);
    BAIL_ON_REG_ERROR(dwError);

    RegParseClose(parseH);

cleanup:
    return dwError;

error:
    RegParseGetLineNumber(parseH, &lineNum);
    sprintf(cErrorBuf, "regshell: import failed (line=%d)", lineNum);
    RegPrintError(cErrorBuf, dwError);
    if (pParseState->eShellMode == REGSHELL_CMD_MODE_INTERACTIVE)
    {
        /* Only smash error when in interactive shell mode */
        dwError = 0;
    }

    goto cleanup;
}


DWORD
RegShellExportFile(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem
    )
{
    DWORD dwError = 0;
    DWORD dwSubKeysCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    char szRegFileName[PATH_MAX + 1] = {0};
    HANDLE hReg = NULL;
    HKEY hSubKey = NULL;
    HKEY hRootKey = NULL;
    PSTR pszFullPath = NULL;
    PSTR pszRootFullPath = NULL;
    PSTR pszDefaultKey = NULL;
    PSTR pszRootKey = NULL;
    PSTR pszTemp = NULL;
    HKEY hRootKeyToRetrieveSd = NULL;
    SECURITY_INFORMATION SecInfoAll = OWNER_SECURITY_INFORMATION
                                     |GROUP_SECURITY_INFORMATION
                                     |DACL_SECURITY_INFORMATION
                                     |SACL_SECURITY_INFORMATION;
    PREG_EXPORT_STATE pExportState = NULL;
    DWORD dwArgc = 0;

    hReg = pParseState->hReg;
    dwError = RegAllocateMemory(sizeof(*pExportState), (LW_PVOID*)&pExportState);
    BAIL_ON_INVALID_POINTER(pExportState);

    if (!strcmp(rsItem->args[dwArgc], "--legacy"))
    {
        pExportState->dwExportFormat = REGSHELL_EXPORT_LEGACY;
        dwArgc++;
    }
    else if (!strcmp(rsItem->args[dwArgc], "--values"))
    {
        pExportState->dwExportFormat = REGSHELL_EXPORT_VALUES;
        dwArgc++;
    }
    if (dwArgc < rsItem->argsCount)
    {
        strcpy(szRegFileName, rsItem->args[dwArgc]);
    }

    RegStripWhitespace(szRegFileName, TRUE, TRUE);

    if (!szRegFileName[0] || !strcmp(szRegFileName, "-"))
    {
        pExportState->fp = stdout;
    }
    else
    {
        pExportState->fp = fopen(szRegFileName, "w");
    }
    if (pExportState->fp == NULL) {
        dwError = errno;
        goto error;
    }

    pExportState->ulRootKeySecDescLen = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;
    memset(pExportState->pRootKeySecDescRel,
           0,
           sizeof(pExportState->pRootKeySecDescRel));

    dwError = RegOpenKeyExA(hReg,
                            NULL,
                            HKEY_THIS_MACHINE,
                            0,
                            KEY_READ | READ_CONTROL,
                            &hRootKeyToRetrieveSd);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegGetKeySecurity(hReg,
                                hRootKeyToRetrieveSd,
                                SecInfoAll,
                                (PSECURITY_DESCRIPTOR_RELATIVE)pExportState->pRootKeySecDescRel,
                                &pExportState->ulRootKeySecDescLen);
    BAIL_ON_REG_ERROR(dwError);

    if (hRootKeyToRetrieveSd)
    {
        RegCloseKey(hReg, hRootKeyToRetrieveSd);
        hRootKeyToRetrieveSd = NULL;
    }

    if (pExportState->ulRootKeySecDescLen == 0)
    {
        dwError = ERROR_INVALID_SECURITY_DESCR;
        BAIL_ON_REG_ERROR(dwError);
    }

    if (rsItem->keyName && *rsItem->keyName)
    {
        dwError = LwRtlCStringDuplicate(
                      &pszRootKey,
                      rsItem->keyName);
        BAIL_ON_REG_ERROR(dwError);
        pszTemp = strchr(pszRootKey, '\\');
        if (pszTemp)
        {
            *pszTemp = '\0';
        }
        dwError = RegOpenKeyExA(hReg,
                                NULL,
                                pszRootKey,
                                0,
                                KEY_READ | READ_CONTROL,
                                &hRootKey);
        if (dwError)
        {
            dwError = RegOpenKeyExA(hReg,
                                    NULL,
                                    HKEY_THIS_MACHINE,
                                    0,
                                    KEY_READ | READ_CONTROL,
                                    &hRootKey);
            BAIL_ON_REG_ERROR(dwError);

            LWREG_SAFE_FREE_STRING(pszRootKey);
            dwError = LwRtlCStringDuplicate(
                          &pszRootKey,
                          HKEY_THIS_MACHINE);
            BAIL_ON_REG_ERROR(dwError);

            pszDefaultKey = strchr(rsItem->keyName, '\\'); 
            if (pszDefaultKey)
            {
                pszDefaultKey++;
            }
            else
            {
                pszDefaultKey = rsItem->keyName;
            }
        }
        else
        {
            pszDefaultKey = pszTemp + 1;
        }
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegShellCanonicalizePath(pszDefaultKey,
                                           NULL,
                                           &pszFullPath,
                                           NULL,
                                           NULL);
        BAIL_ON_REG_ERROR(dwError);
        
        if (pszFullPath && strcmp(pszFullPath, "\\") != 0)
        {
            dwError = RegOpenKeyExA(
                          hReg,
                          hRootKey,
                          pszFullPath+1,
                          0,
                          KEY_READ | READ_CONTROL,
                          &hSubKey);
            BAIL_ON_REG_ERROR(dwError);

        }
        else
        {
            hSubKey = hRootKey;
            hRootKey = NULL;
        }
        dwError = LwRtlCStringAllocatePrintf(
                      &pszRootFullPath,
                      "%s%s",
                      pszRootKey,
                      pszFullPath);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = RegOpenKeyExA(hReg,
                                NULL,
                                HKEY_THIS_MACHINE,
                                0,
                                KEY_READ,
                                &hSubKey);
        BAIL_ON_REG_ERROR(dwError);
        dwError = LwRtlCStringAllocatePrintf(
                  &pszRootFullPath,
                  HKEY_THIS_MACHINE);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegQueryInfoKeyA(
                  hReg,
                  hSubKey,
                  NULL,
                  NULL,
                  NULL,
                  &dwSubKeysCount,
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
                                 hSubKey,
                                 pszRootFullPath,
                                 dwSubKeysCount,
                                 dwMaxSubKeyLen);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (hSubKey)
    {
        RegCloseKey(hReg, hSubKey);
    }
    if (hRootKey)
    {
        RegCloseKey(hReg, hRootKey);
    }
    if (hRootKeyToRetrieveSd)
    {
        RegCloseKey(hReg, hRootKeyToRetrieveSd);
    }
    if (pExportState->fp && pExportState->fp != stdout)
    {
        fclose(pExportState->fp);
        pExportState->fp = NULL;
    }
    if (pExportState)
    {
        RegFreeMemory(pExportState);
    }
    LWREG_SAFE_FREE_STRING(pszRootKey);
    LWREG_SAFE_FREE_STRING(pszFullPath);
    LWREG_SAFE_FREE_STRING(pszRootFullPath);


    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellListValues(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem,
    PDWORD pdwValuesListed)
{
    DWORD dwError = 0;
    DWORD dwValuesLen = 0;
    DWORD i = 0;
    DWORD dwMultiIndex = 0;
    DWORD dwValueNameLenMax = 0;
    DWORD dwValueNameLen = 0;
    DWORD dwValue = 0;
    DWORD dwValuesListed = 0;
    DWORD dwEscapedValueLen = 0;
    PSTR pszValueName = NULL;
    PSTR *ppszMultiStrArray = NULL;
    PSTR pszEscapedValue = NULL;
    PBYTE pData = NULL;
    PREGSHELL_UTIL_VALUE pValues = NULL;
    PSTR pszValueNameEscaped = NULL;
    DWORD dwValueNameEscapedLen = 0;

    dwError = RegShellUtilGetValues(
                  pParseState->hReg,
                  RegShellGetRootKey(pParseState),
                  RegShellGetDefaultKey(pParseState),
                  rsItem->keyName,
                  &pValues,
                  &dwValuesLen);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; i<dwValuesLen; i++)
    {
        dwValueNameLen = wc16slen(pValues[i].pValueName);
        if (dwValueNameLen>dwValueNameLenMax)
        {
            dwValueNameLenMax = dwValueNameLen;
        }
    }
    dwValueNameLenMax++;

    for (i=0; i<dwValuesLen; i++)
    {
        if (dwError == 0)
        {
            LWREG_SAFE_FREE_STRING(pszValueName);
            LWREG_SAFE_FREE_STRING(pszValueNameEscaped);

            dwError = LwRtlCStringAllocateFromWC16String(
                          &pszValueName,
                          pValues[i].pValueName);
            BAIL_ON_REG_ERROR(dwError);
            dwError = RegShellUtilEscapeString(
                          pszValueName,
                          &pszValueNameEscaped,
                          &dwValueNameEscapedLen);
#ifdef _LW_DEBUG
            printf("ListValues: value='%s\n", pszValueNameEscaped);
            printf("ListValues: dataLen='%d'\n", pValues[i].dwDataLen);
#endif

            if (strcmp(pszValueNameEscaped, "@") == 0 &&
                *((PSTR) pValues[i].pData) == '\0')
            {
                continue;
            }
            dwValuesListed++;
            printf("%c  \"%s\"%*s",
                   pValues[i].bValueSet ? '+' : ' ',
                   pszValueNameEscaped,
                   (int) (strlen(pszValueNameEscaped)-dwValueNameLenMax),
                   "");

            switch (pValues[i].type)
            {
                case REG_SZ:
                    dwError = RegShellUtilEscapeString(
                                  pValues[i].pData,
                                  &pszEscapedValue,
                                  &dwEscapedValueLen);
                    BAIL_ON_REG_ERROR(dwError);
                    printf("REG_SZ          \"%s\"\n",
                           pszEscapedValue);
                    LWREG_SAFE_FREE_MEMORY(pszEscapedValue);
                    break;

                case REG_DWORD:
                    memcpy(&dwValue, pValues[i].pData, sizeof(DWORD));
                    printf("REG_DWORD       0x%08x (%u)\n", dwValue, dwValue);
                    break;

                case REG_BINARY:
                    printf("REG_BINARY      ");
                    RegShellDumpByteArray(pValues[i].pData,
                                          pValues[i].dwDataLen);
                    printf("\n");
                    break;

                case REG_MULTI_SZ:
                    dwError = RegByteArrayToMultiStrsA(
                                  pValues[i].pData,
                                  pValues[i].dwDataLen,
                                  &ppszMultiStrArray);
                    BAIL_ON_REG_ERROR(dwError);
                    if (!ppszMultiStrArray[0])
                    {
                        /* 
                         * Just print the type for a reg_multi_sz 
                         * with no values.
                         */
                        printf("%*sREG_MULTI_SZ\n",
                               dwMultiIndex == 0 ? 0 :
                                   dwValueNameLenMax + 2,
                                   "");
                    }
                    else
                    {
                        for (dwMultiIndex=0;
                             ppszMultiStrArray[dwMultiIndex];
                             dwMultiIndex++)
                        {
                            dwError = RegShellUtilEscapeString(
                                          ppszMultiStrArray[dwMultiIndex],
                                          &pszEscapedValue,
                                          &dwEscapedValueLen);
                            BAIL_ON_REG_ERROR(dwError);
                            printf("%*sREG_MULTI_SZ[%d] \"%s\"\n",
                                   dwMultiIndex == 0 ? 0 :
                                       dwValueNameLenMax + 4,
                                       "",
                                   dwMultiIndex,
                                   pszEscapedValue);
                            LWREG_SAFE_FREE_MEMORY(pszEscapedValue);
    
                        }
                    }
                    RegFreeMultiStrsA(ppszMultiStrArray);
                    ppszMultiStrArray = NULL;
                    break;

                default:
                    printf("no handler for datatype %d\n", pValues[i].type);
            }
            LWREG_SAFE_FREE_MEMORY(pData);
        }
    }
cleanup:
    LWREG_SAFE_FREE_MEMORY(pszEscapedValue);
    RegFreeMultiStrsA(ppszMultiStrArray);
    RegShellUtilValueArrayFree(pValues, dwValuesLen);
    if (pdwValuesListed)
    {
        *pdwValuesListed = dwValuesListed;
    }
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_STRING(pszValueNameEscaped);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellProcessCmd(
    PREGSHELL_PARSE_STATE pParseState,
    DWORD argc,
    PSTR *argv)
{

    DWORD dwError = 0;
    DWORD dwOpenRootKeyError = 0;
    DWORD dwValuesListed = 0;
    PREGSHELL_CMD_ITEM rsItem = NULL;
    PCSTR pszErrorPrefix = NULL;
    PSTR pszPwd = NULL;
    PSTR pszToken = NULL;
    PSTR pszKeyName = NULL;
    PSTR pszNewKeyName = NULL;
    PSTR pszNewDefaultKey = NULL;
    PSTR pszStrtokState = NULL;
    PSTR pszRootKeyName = NULL;
    PSTR pszFullKeyName = NULL;
    BOOLEAN bChdirOk = TRUE;
    HKEY hRootKey = NULL;
    CHAR szError[128] = {0};

    dwError = RegShellCmdParse(pParseState, argc, argv, &rsItem);
    if (dwError == 0)
    {
#ifdef _LW_DEBUG
        RegShellDumpCmdItem(rsItem);
#endif
        switch (rsItem->command)
        {
            case REGSHELL_CMD_LIST_KEYS:
                pszErrorPrefix = "list_keys: failed ";
                dwError = RegShellListKeys(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_LIST:
            case REGSHELL_CMD_DIRECTORY:
                pszErrorPrefix = "list: failed ";
                pszRootKeyName = RegShellGetRootKey(pParseState);
                pszFullKeyName = RegShellGetDefaultKey(pParseState);
                if (pszRootKeyName)
                {
                    printf("\n[%s%s%s%s%s]\n",
                        pszRootKeyName,
                        pszFullKeyName ? "\\" : "",
                        pszFullKeyName ?  pszFullKeyName : "",
                        rsItem->keyName ? "\\" : "",
                        rsItem->keyName ?  rsItem->keyName : "");
                }

                if (pszRootKeyName)
                {
                    dwError = RegShellListValues(
                                  pParseState,
                                  rsItem,
                                  &dwValuesListed);
                    BAIL_ON_REG_ERROR(dwError);
                }
                if (dwValuesListed > 0)
                {
                    printf("\n");
                }
                dwError = RegShellListKeys(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                printf("\n");
                break;

            case REGSHELL_CMD_ADD_KEY:
                pszErrorPrefix = "add_key: failed";
                dwError = RegShellAddKey(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_KEY:
                pszErrorPrefix = "delete_key: failed ";
                dwError = RegShellDeleteKey(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_VALUE:
                pszErrorPrefix = "delete_value: failed ";
                dwError = RegShellDeleteValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_TREE:
                pszErrorPrefix = "delete_tree: failed ";
                dwError = RegShellDeleteTree(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_SET_VALUE:
                pszErrorPrefix = "set_value: failed ";
                dwError = RegShellSetValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_ADD_VALUE:
                pszErrorPrefix = "add_value: failed ";
                dwError = RegShellSetValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_HELP:
                RegShellUsage(argv[0]);
                break;

            case REGSHELL_CMD_CHDIR:
                dwError = LwRtlCStringDuplicate(&pszNewKeyName, &argv[2][1]);
                BAIL_ON_REG_ERROR(dwError);
                pszNewKeyName [strlen(pszNewKeyName) - 1] = '\0';
                pszKeyName = pszNewKeyName;
                pszToken = strtok_r(pszKeyName, "\\", &pszStrtokState);
                if (!pszToken)
                {
                    /*
                     * Handle special case where the only thing provided in
                     * the path is one or more \ characters (e.g [\\\]). In
                     * this case, strtok_r() return NULL when parsing
                     * a non-zero length pszKeyName string. This is essentially
                     * the cd \ case
                     */
                    LWREG_SAFE_FREE_MEMORY(pParseState->pszDefaultKey);
                    LWREG_SAFE_FREE_MEMORY(pParseState->pszDefaultRootKeyName);
                    dwError = 0;
                    goto cleanup;
                }

                if (pParseState->pszDefaultKey)
                {
                    /*
                     * Another special case when the path begins with a / or \.
                     * Force the current working directory to root.
                     */
                    if (pszNewKeyName[0] != '/' && pszNewKeyName[0] != '\\')
                    {
                        dwError = LwRtlCStringDuplicate(
                                      &pszNewDefaultKey,
                                      pParseState->pszDefaultKey);
                        BAIL_ON_REG_ERROR(dwError);
                    }
                }


                while (pszToken)
                {
                    pszKeyName = NULL;
                    dwOpenRootKeyError = RegOpenKeyExA(
                                  pParseState->hReg,
                                  NULL,
                                  pszToken,
                                  0,
                                  KEY_READ,
                                  &hRootKey);
                    if (dwOpenRootKeyError == 0)
                    {
                        RegCloseKey(pParseState->hReg, hRootKey);
                        LWREG_SAFE_FREE_MEMORY(pParseState->pszDefaultRootKeyName);
                        LWREG_SAFE_FREE_STRING(pParseState->pszDefaultKey);
                        LWREG_SAFE_FREE_STRING(pszNewDefaultKey);
                        dwError = LwRtlCStringDuplicate(
                                      &pParseState->pszDefaultRootKeyName,
                                      pszToken);
                        BAIL_ON_REG_ERROR(dwError);
                    }
                    else if (strcmp(pszToken, "..") == 0)
                    {
                        if (pszNewDefaultKey)
                        {
                            pszPwd = strrchr(pszNewDefaultKey,
                                             '\\');
                            if (pszPwd)
                            {
                                    pszPwd[0] = '\0';
                            }
                            else
                            {
                                LWREG_SAFE_FREE_MEMORY(pszNewDefaultKey);
                            }
                        }
                    }
                    else if (strcmp(pszToken, ".") == 0)
                    {
                        /* This is a no-op */
                    }
                    else if (strcmp(pszToken, "...") == 0)
                    {
                        /* This is a broken path! */
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                        BAIL_ON_REG_ERROR(dwError);

                    }
                    else if (pszNewDefaultKey)
                    {
                        /* Append this token to current relative path */
                        dwError = RegAllocateMemory(
                                      sizeof(*pszPwd) * (strlen(pszToken) +
                                          strlen(pszNewDefaultKey)+3),
                                      (PVOID*)&pszPwd);
                        BAIL_ON_REG_ERROR(dwError);

                        strcpy(pszPwd, pszNewDefaultKey);
                        strcat(pszPwd, "\\");
                        strcat(pszPwd, pszToken);
                        dwError = RegShellIsValidKey(
                                      pParseState->hReg,
                                      pParseState->pszDefaultRootKeyName,
                                      pszPwd);
                        if (dwError)
                        {
                            LwRegGetErrorString(dwError,
                                                szError,
                                                sizeof(szError)-1);
                            printf("cd: key not valid '%s' [%s]\n",
                                   pszToken, szError);
                            LWREG_SAFE_FREE_MEMORY(pszPwd);
                            pszPwd = NULL;
                            bChdirOk = FALSE;
                            dwError = 0;
                            break;
                        }
                        else
                        {
                            LWREG_SAFE_FREE_MEMORY(pszNewDefaultKey);
                            pszNewDefaultKey = pszPwd;
                            pszPwd = NULL;
                        }
                    }
                    else
                    {
                        if (dwOpenRootKeyError ==
                            LWREG_ERROR_NO_SUCH_KEY_OR_VALUE &&
                            !pParseState->pszDefaultRootKeyName)
                        {
                            pParseState->pszDefaultRootKeyName = 
                                strdup(HKEY_THIS_MACHINE);
                        }
                              
                        dwError = RegShellIsValidKey(
                                      pParseState->hReg,
                                      pParseState->pszDefaultRootKeyName,
                                      pszToken);
                        if (dwError)
                        {
                            LwRegGetErrorString(dwError,
                                                szError,
                                                sizeof(szError)-1);
                            printf("cd: key not valid '%s' [%s]\n",
                                   pszToken, szError);

                            bChdirOk = FALSE;
                            dwError = 0;
                            break;
                        }
                        else
                        {
                            LWREG_SAFE_FREE_MEMORY(pszNewDefaultKey);
                            dwError = LwRtlCStringDuplicate(
                                          &pszNewDefaultKey,
                                          pszToken);
                            BAIL_ON_REG_ERROR(dwError);
                        }
                    }
                    pszToken = strtok_r(pszKeyName, "\\", &pszStrtokState);
                }
                if (bChdirOk)
                {
                    if (pParseState->pszDefaultKey)
                    {
                        LWREG_SAFE_FREE_MEMORY(pParseState->pszDefaultKey);
                    }
                    pParseState->pszDefaultKey = pszNewDefaultKey;
                    pszNewDefaultKey = NULL;
                }
                else
                {
                    LWREG_SAFE_FREE_MEMORY(pszNewDefaultKey);
                }
                break;

            case REGSHELL_CMD_PWD:
                if (RegShellGetRootKey(pParseState))
                {
                    printf("[%s\\%s]\n\n",
                            RegShellGetRootKey(pParseState),
                            RegShellGetDefaultKey(pParseState) ? 
                                RegShellGetDefaultKey(pParseState) : "");
                }
                else
                {
                    printf("'%s'\n\n", "\\");
                }
                break;

            case REGSHELL_CMD_QUIT:
                exit(0);
                break;

            case REGSHELL_CMD_LIST_VALUES:
                pszErrorPrefix = "list_values: failed ";
                dwError = RegShellListValues(pParseState, rsItem, NULL);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_IMPORT:
                dwError = RegShellImportFile(
                              pParseState,
                              rsItem,
                              REGSHELL_UTIL_IMPORT_OVERWRITE);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_UPGRADE:
                dwError = RegShellImportFile(
                              pParseState,
                              rsItem,
                              REGSHELL_UTIL_IMPORT_UPGRADE);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_CLEANUP:
                dwError = RegShellImportFile(
                              pParseState,
                              rsItem,
                              REGSHELL_UTIL_IMPORT_CLEANUP);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_EXPORT:
                dwError = RegShellExportFile(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_SET_HIVE:
                if (argc < 3)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    goto error;
                }

                LWREG_SAFE_FREE_STRING(pParseState->pszDefaultRootKeyName);
                dwError = LwRtlCStringDuplicate(&pParseState->pszDefaultRootKeyName, argv[2]);
                BAIL_ON_REG_ERROR(dwError);
                LWREG_SAFE_FREE_STRING(pParseState->pszDefaultKey);
                break;

            default:
                break;
        }
    }
    else
    {
        printf("%s: error parsing command, invalid syntax\n", argv[0]);
        RegShellUsage(argv[0]);
    }

cleanup:
    RegShellCmdParseFree(rsItem);
    LWREG_SAFE_FREE_STRING(pszNewKeyName);
    LWREG_SAFE_FREE_STRING(pszNewDefaultKey);

    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellInitParseState(
    PREGSHELL_PARSE_STATE *ppParseState)
{
    DWORD dwError = 0;
    PREGSHELL_PARSE_STATE pParseState = NULL;

    BAIL_ON_INVALID_POINTER(ppParseState);

    dwError = RegAllocateMemory(sizeof(*pParseState), (PVOID*)&pParseState);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenServer(&pParseState->hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegLexOpen(&pParseState->lexHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegIOBufferOpen(&pParseState->ioHandle);
    BAIL_ON_REG_ERROR(dwError);

    *ppParseState = pParseState;

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_STRING(pParseState->pszDefaultRootKeyName);
    RegCloseServer(pParseState->hReg);
    RegLexClose(pParseState->lexHandle);
    RegIOClose(pParseState->ioHandle);
    LWREG_SAFE_FREE_MEMORY(pParseState);
    goto cleanup;
}


DWORD
RegShellCloseParseState(
    PREGSHELL_PARSE_STATE pParseState)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_POINTER(pParseState);

    RegLexClose(pParseState->lexHandle);
    RegIOClose(pParseState->ioHandle);
    LWREG_SAFE_FREE_STRING(pParseState->pszDefaultRootKeyName);
    LWREG_SAFE_FREE_STRING(pParseState->pszDefaultKey);
    LWREG_SAFE_FREE_STRING(pParseState->pszDefaultKeyCompletion);
    LWREG_SAFE_FREE_STRING(pParseState->pszFullRootKeyName);
    LWREG_SAFE_FREE_STRING(pParseState->pszFullKeyPath);
    LWREG_SAFE_FREE_STRING(pParseState->tabState.pszCommand);
    LWREG_SAFE_FREE_STRING(pParseState->tabState.pszRootKey);
    LWREG_SAFE_FREE_STRING(pParseState->tabState.pszSubKey);
    LWREG_SAFE_FREE_STRING(pParseState->tabState.pszInputLine);
    LWREG_SAFE_FREE_STRING(pParseState->tabState.pszPrevLine);
    LWREG_SAFE_FREE_STRING(pParseState->tabState.pszCurLine);
    RegCloseServer(pParseState->hReg);
    LWREG_SAFE_FREE_MEMORY(pParseState);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellStrcmpLen(
    PSTR pszMatchStr,
    PSTR pszHaystackStr,
    DWORD dwMatchMaxLen,
    PDWORD pdwExtentLen)
{
    DWORD index = 0;
    DWORD dwMaxMatch = 0;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pszMatchStr);
    BAIL_ON_INVALID_POINTER(pszHaystackStr);
    BAIL_ON_INVALID_POINTER(pdwExtentLen);

    dwMaxMatch = *pdwExtentLen;
    for (index = 0;
         pszMatchStr[index] &&
         pszHaystackStr[index] &&
         pszMatchStr[index] == pszHaystackStr[index] &&
         index < dwMaxMatch;
         index++)
    {
        ;
    } 
    if (dwMatchMaxLen)
    {
        if (pszHaystackStr[dwMatchMaxLen] == '\0')
        {
            *pdwExtentLen = index;
        }
        else
        {
            dwError = LWREG_ERROR_INVALID_CONTEXT;
        }
    }
    else
    {
        *pdwExtentLen = index;
    }
    
cleanup:
    return dwError;
error:
    goto cleanup;
}


void RegShellFreeStrList(
    PSTR *ppszList)
{
    DWORD i = 0;
 
    if (ppszList)
    {
        for (i=0; ppszList[i]; i++)
        {
            LWREG_SAFE_FREE_STRING(ppszList[i]);
        }
        LWREG_SAFE_FREE_MEMORY(ppszList);
    }
}


DWORD
RegShellCompletionMatch(
    PSTR pszMatchStr,
    PWSTR *ppSubKeys,
    DWORD dwSubKeyLen,
    PSTR pszDefaultRootKeyName,
    PSTR **pppMatchArgs,
    PDWORD pdwMatchArgsLen,
    PDWORD pdwMatchCommonIndex,
    PDWORD pdwMatchCommonLen)
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwMatchArgsLen = 0;
    DWORD dwMinCommonLen = INT32_MAX;
    DWORD dwMaxCommonLen = 0;
    DWORD dwPrevMaxCommonLen = 0;
    DWORD dwMinCommonLenIndex = 0;
    DWORD dwMaxCommonLenIndex = 0;
    DWORD dwMatchMaxLen = 0;
    DWORD dwStrLen = 0;
    BOOLEAN bBackslashEnd = FALSE;
    PSTR pszSubKey = NULL;
    PSTR pszTmpSubKey = NULL;

    PSTR *ppMatchArgs = NULL;
    PCSTR pszPtr = NULL;


    BAIL_ON_INVALID_POINTER(pszMatchStr);
    BAIL_ON_INVALID_POINTER(ppSubKeys);
    BAIL_ON_INVALID_POINTER(pppMatchArgs);
    BAIL_ON_INVALID_POINTER(pdwMatchCommonIndex);
    BAIL_ON_INVALID_POINTER(pdwMatchCommonLen);
    if (dwSubKeyLen > 0)
    {
        dwError = RegAllocateMemory(
                      sizeof(*ppMatchArgs) * (dwSubKeyLen + 1),
                      (PVOID*)&ppMatchArgs);
        BAIL_ON_REG_ERROR(dwError);
    }
 
    dwStrLen = strlen(pszMatchStr);
    if (dwStrLen>0 && pszMatchStr[dwStrLen-1] == '\\')
    {
        dwMatchMaxLen = dwStrLen-1;
        pszMatchStr[dwMatchMaxLen] = '\0';
    }

    for (i=0; i<dwSubKeyLen; i++)
    {
        dwError = LwRtlCStringAllocateFromWC16String(&pszSubKey, ppSubKeys[i]);
        BAIL_ON_REG_ERROR(dwError);

        pszTmpSubKey = strrchr(pszSubKey, '\\');
        if (pszTmpSubKey)
        {
            pszTmpSubKey++;
        }
        else
        {
            pszTmpSubKey = pszSubKey;
        }
        pszPtr = NULL;
        if (pszMatchStr && *pszMatchStr)
        {
            RtlCStringFindSubstring(pszTmpSubKey, pszMatchStr, FALSE, &pszPtr);
        }
        if (pszPtr && pszPtr == pszTmpSubKey)
        {
            dwError = LwRtlCStringDuplicate(
                          &ppMatchArgs[dwMatchArgsLen], pszTmpSubKey);
            BAIL_ON_REG_ERROR(dwError);
            dwStrLen = strlen(pszTmpSubKey);
            if (dwStrLen < dwMinCommonLen)
            {
                dwMinCommonLen = dwStrLen;
                dwMinCommonLenIndex = dwMatchArgsLen;
            }
            dwMatchArgsLen++;
        }
        else if (!pszMatchStr || strlen(pszMatchStr) == 0)
        {
            /* Treat empty match string as wildcard, match everything */
            dwError = LwRtlCStringDuplicate(
                          &ppMatchArgs[dwMatchArgsLen], pszTmpSubKey);
            BAIL_ON_REG_ERROR(dwError);
            dwMatchArgsLen++;
        }
        LWREG_SAFE_FREE_STRING(pszSubKey);
    }

    /*
     * Use dwMinCommonLenIndex as needle to find the longest
     * common string among the strings that matched pszMatchStr
     */
    dwStrLen = INT32_MAX;
    if (ppMatchArgs && ppMatchArgs[dwMinCommonLenIndex])
    {
        if (!bBackslashEnd)
        {
            dwMaxCommonLen = strlen(ppMatchArgs[dwMinCommonLenIndex]);
        }
    }
    for (i=0; i<dwMatchArgsLen; i++)
    {
        dwError = RegShellStrcmpLen(
                      ppMatchArgs[dwMinCommonLenIndex],
                      ppMatchArgs[i],
                      dwMatchMaxLen,
                      &dwMaxCommonLen);
        if (dwError == 0 && dwMaxCommonLen != dwPrevMaxCommonLen)
        {
            dwMaxCommonLenIndex = i;
            dwPrevMaxCommonLen = dwMaxCommonLen;
        }
                      
    }
    *pppMatchArgs = ppMatchArgs;
    *pdwMatchArgsLen = dwMatchArgsLen;
    *pdwMatchCommonIndex = dwMaxCommonLenIndex;
    *pdwMatchCommonLen = dwPrevMaxCommonLen;
cleanup:
    return dwError;

error:
    RegShellCmdlineParseFree( dwMatchArgsLen, ppMatchArgs);
    goto cleanup;
}




void RegShellSetInputLine(EditLine *el, PSTR pszInput)
{
    el_insertstr(el, pszInput);
}


void RegShellSetSubStringInputLine(
    EditLine *el, 
    PSTR pszSubString,
    PSTR pszInput,
    PDWORD pdwLenAppended)
{
    DWORD dwLenSubString = 0;
    DWORD dwLenInput = 0;
    DWORD dwLenAppended = 0;

    if (pszInput)
    {
        if (pszSubString)
        {
            dwLenSubString = strlen(pszSubString);
        }
        dwLenInput = strlen(pszInput);

        if (dwLenSubString < dwLenInput)
        {
            el_insertstr(el, &pszInput[dwLenSubString]);
            dwLenAppended = strlen(&pszInput[dwLenSubString]);
        }
    }
    else
    {
        /* No substring prefix, so just set substring as the complete value */
        el_insertstr(el, pszSubString);
        dwLenAppended = strlen(pszSubString);
    }
    *pdwLenAppended = dwLenAppended;
}


VOID
RegShellPrependStringInput(EditLine *inel, PSTR pszPrefix, PSTR pszCursor)
{
    PSTR pszTmp = NULL;
    PSTR pszSaveCursor = NULL;
    LineInfo *el = (LineInfo *) el_line(inel);

    if (pszCursor && *pszCursor)
    {
        pszTmp = strstr(el->buffer, pszCursor);
    }
    if (pszTmp)
    {
        pszSaveCursor = pszTmp;
        el->cursor = pszTmp;
    }
    el_insertstr(inel, pszPrefix);

    if (pszTmp)
    {
        el->cursor = el->lastchar;
    }
}


DWORD RegShellCompleteGetInput(EditLine *el, PSTR *ppszLine)
{

    const LineInfo *lineInfoCtx = el_line(el);
    DWORD dwError = CC_ERROR;
    DWORD dwLineLen = 0;
    PSTR pszLine = NULL;

    dwLineLen = lineInfoCtx->cursor - lineInfoCtx->buffer;
    if (dwLineLen == 0)
    {
        return dwError;
    }
    dwError = RegAllocateMemory(sizeof(*pszLine) * (dwLineLen+1),
                               (PVOID*)&pszLine);
    BAIL_ON_REG_ERROR(dwError);
    strncat(pszLine, lineInfoCtx->buffer, dwLineLen);
    *ppszLine = pszLine;

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD RegShellSplitCmdParam(
    PSTR pszInput,
    PSTR *ppszOutCmd,
    PSTR *ppszOutParam)
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    DWORD dwCmdLen = 0;
    PSTR pszTmp = NULL;
    PSTR pszOutCmd = NULL;
    PSTR pszOutParam = NULL;

    BAIL_ON_INVALID_POINTER(pszInput);
    BAIL_ON_INVALID_POINTER(ppszOutCmd);
    BAIL_ON_INVALID_POINTER(ppszOutParam);
    dwLen = strlen((char *) pszInput); 
    if (dwLen == 0)
    {
        dwError = EINVAL;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RtlCStringDuplicate(&pszOutCmd, pszInput);
    BAIL_ON_REG_ERROR(dwError);

    pszTmp = pszOutCmd;
    while (*pszTmp && !isspace((int) *pszTmp))
    {
        pszTmp++;
    }
    *pszTmp = '\0';
    dwCmdLen = pszTmp - pszOutCmd;

    if (dwCmdLen+1 < dwLen)
    {
        pszTmp++;
        while (*pszTmp && isspace((int) *pszTmp))
        {
            pszTmp++;
        }
        if (*pszTmp)
        {
            dwError = RtlCStringDuplicate(&pszOutParam, pszTmp);
            BAIL_ON_REG_ERROR(dwError);
        }
    }

    /* Force default "parameter" of an empty string */
    if (!pszOutParam)
    {
        dwError = RtlCStringDuplicate(&pszOutParam, "");
        BAIL_ON_REG_ERROR(dwError);
    }
cleanup:
    *ppszOutCmd = pszOutCmd;
    *ppszOutParam = pszOutParam;
    return dwError;

error:
    goto cleanup;
}


void RegShellCompletePrint(EditLine *el, PREGSHELL_PARSE_STATE pParseState)
{
    el_set(el, EL_REFRESH);
}


DWORD
RegShellGetLongestValidKey(
    HANDLE hReg,
    PSTR pszRootKey,
    PSTR pszDefaultKey,
    PSTR pszInSubKey,
    PSTR *ppszRetSubKey,
    PSTR *ppszResidualSubKey)
{

    DWORD dwError = 0;
    DWORD dwStrLen = 0;
    PSTR pszPath = NULL;
    PSTR pszTmp = NULL;
    PSTR pszResidual = NULL;

    if (pszDefaultKey)
    {
        /* Splice together pwd and input from command line */
        if (pszInSubKey && *pszInSubKey == '\\')
        {
            /* Nuke out leading \ */
            pszInSubKey++;
        }

        /* strtok modifies input parameter, so make copy */
        dwError = RtlCStringAllocateAppendPrintf(
                      &pszPath, "%s%s%s",
                      pszDefaultKey,
                      pszInSubKey ? "\\" : "",
                      pszInSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = LwRtlCStringDuplicate(&pszPath, pszInSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    /*
     * p=Valid SubKeyPath; r=Residual subject to further analysis
     * Input: Ser
     *        Services
     *        Services\
     *        Services\lsa
     *        Services\lsass\Par
     *        Services\lsss\Par
     * Result:
     *        p=NULL, r=Ser
     *        p=Services, r=NULL
     *        p=Services, r=NULL
     *        p=Services, r=lsa
     *        p=Services\lsass, r=Par
     *        p=NULL, r=NULL dwError=40700
     *        
     */


    /* 1: Test pszPath for validity */
    dwError = RegShellIsValidKey(
                  hReg,
                  pszRootKey,
                  pszPath);

    if (dwError)
    {
        /* 
         * Is there a \ separator in pszPath? No, then pszPath is the
         * return residual value.
         */
        pszTmp = strrchr(pszPath, '\\');
        if (!pszTmp)
        {
            pszResidual = pszPath;
            pszPath = NULL;
            dwError = 0;
        }
        else
        {
            dwError = LwRtlCStringDuplicate(
                          &pszResidual,
                          &pszTmp[1]);
            BAIL_ON_REG_ERROR(dwError);
            *pszTmp = '\0';
  
            /*
             * Test modified pszPath (less residual) for validity. If this is
             * not a valid subkey, then the entire path is bogus.
             */
            dwError = RegShellIsValidKey(
                          hReg,
                          pszRootKey,
                          pszPath);
            if (dwError)
            {
                LWREG_SAFE_FREE_STRING(pszResidual);
                LWREG_SAFE_FREE_STRING(pszPath);
            }
        }
    }
    else
    {
        /* 
         * A valid path ending in \ means this IS the subkey.
         * A valid path without a terminating \ could be ambiguous,
         * so return the stuff after \ as residual for subsequent
         * prefix matching by the caller.
         */
        dwStrLen = strlen(pszPath);
        if (dwStrLen > 0 && pszPath[dwStrLen-1] != '\\')
        {
            pszTmp = strrchr(pszPath, '\\');
            if (pszTmp)
            {
                dwError = LwRtlCStringDuplicate(
                              &pszResidual,
                              &pszTmp[1]);
                BAIL_ON_REG_ERROR(dwError);
                *pszTmp = '\0';
            }
        }
    }
    
    *ppszRetSubKey = pszPath;
    *ppszResidualSubKey = pszResidual;
cleanup:
    return dwError;

error:
    goto cleanup;
}




DWORD
RegShellIsValidRootKey(
    HANDLE hReg,
    PSTR pszRootKey)
{
    DWORD dwError = 0;
    HKEY hRootKey = NULL;

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKey, 0, KEY_READ, &hRootKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (hRootKey)
    {
        RegCloseKey(hReg, hRootKey);
    }
    return dwError;

error:
    goto cleanup;
}


/* Add \ line termination if one does not already exist */
void
RegShellInputLineTerminate(
    EditLine *el, 
    PSTR pszInLine)
{
    DWORD dwLen = 0;

    if (pszInLine)
    {
        dwLen = strlen(pszInLine);
        if (dwLen > 0 && pszInLine[dwLen-1] != '\\')
        {
            RegShellSetInputLine(el, "\\");
        }
    }
    
}


/* Return a value when pszSubKey doesn't match the pszDefaultKey */
DWORD
RegShellGetSubKeyValue(
    PSTR pszDefaultKey,
    PSTR pszSubKey,
    PSTR *ppszRetSubKey)
{
    DWORD dwError = 0;
    DWORD dwDefaultKeyLen = 0;
    PSTR pszRetSubKey = NULL;
    PSTR pszSubMatch = NULL;

    if (!pszDefaultKey && pszSubKey)
    {
        dwError = LwRtlCStringDuplicate(
                      &pszRetSubKey,
                      pszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (!pszDefaultKey || !pszSubKey)
    {
        return dwError;
    }
    else if (strcmp(pszDefaultKey, pszSubKey) == 0)
    {
        return dwError;
    }
    else
    {
        pszSubMatch = strstr(pszSubKey, pszDefaultKey);
        if (pszSubMatch == pszSubKey)
        {
            dwDefaultKeyLen = strlen(pszDefaultKey);
            if (dwDefaultKeyLen > 0)
            {
                pszRetSubKey = &pszSubKey[dwDefaultKeyLen];
                if (*pszRetSubKey && *pszRetSubKey == '\\')
                {
                    pszRetSubKey++;
                }
                dwError = LwRtlCStringDuplicate(
                              &pszRetSubKey,
                              pszRetSubKey);
                BAIL_ON_REG_ERROR(dwError);
            }
        }
    }

 
cleanup:
    *ppszRetSubKey = pszRetSubKey;

    return dwError;
error:
    goto cleanup;
}


unsigned char
pfnRegShellCompleteCallback(
    EditLine *el,
    int ch)
{
    BOOLEAN bProcessingCommand = TRUE;
    BOOLEAN bHasRootKey = FALSE;
    DWORD dwError = CC_ERROR;
    DWORD dwRootKeysCount = 0;
    DWORD i = 0;
    DWORD dwMatchArgsLen = 0;
    DWORD dwMatchBestIndex = 0;
    DWORD dwMatchBestLen = 0;
    DWORD dwSubKeyLen = 0;
    DWORD dwLenAppended = 0;
    PREGSHELL_PARSE_STATE pParseState = NULL;
    PEDITLINE_CLIENT_DATA cldata = NULL;
    PSTR pszInLine = NULL;
    PWSTR *ppwszRootKeys = NULL;
    PSTR pszCommand = NULL;
    PSTR pszTmp = NULL;
    PSTR pszInRootKey = NULL;
    PSTR pszRootKey = NULL;
    PSTR pszParam = NULL;
    PSTR pszParamSave = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszResidualSubKey = NULL;
    PWSTR *ppwszSubKeys = NULL;
    PSTR *ppMatchArgs = NULL;

    el_get(el, EL_CLIENTDATA, (void *) &cldata);
    BAIL_ON_INVALID_HANDLE(cldata);
    BAIL_ON_INVALID_HANDLE(cldata->pParseState);
    BAIL_ON_INVALID_HANDLE(cldata->pParseState->hReg);

    pParseState = cldata->pParseState;

    dwError = RegShellCompleteGetInput(el, &pszInLine);
    BAIL_ON_REG_ERROR(dwError);

    /* Nothing changed in the input since the tab was tapped last */
    if (cldata->pszCompletePrevCmd && pszInLine && 
        cldata->ePrevState == pParseState->tabState.eTabState &&
        !strcmp(cldata->pszCompletePrevCmd, pszInLine) &&
        pParseState->dwTabPressCount > 0)
    {
        goto cleanup;
    }
    pParseState->dwTabPressCount++;
    

    pParseState->tabState.eTabState = REGSHELL_TAB_CMD;
    do 
    {
        switch (pParseState->tabState.eTabState)
        {
            case REGSHELL_TAB_CMD:
                dwError = RegShellSplitCmdParam(
                              pszInLine,
                              &pszCommand,
                              &pszParam);
                if (dwError == 0)
                {
                    pParseState->tabState.eTabState =
                        REGSHELL_TAB_BRACKET_PREFIX;
                
                    LWREG_SAFE_FREE_STRING(pParseState->tabState.pszCommand);
                    pParseState->tabState.pszCommand = pszCommand;
                }
                else
                {
                    BAIL_ON_REG_ERROR(dwError);
                }
                break;
    
            case REGSHELL_TAB_BRACKET_PREFIX:
                if (pszParam && pszParam[0] == '[')
                {
                    dwError = LwRtlCStringDuplicate(
                                  &pszParamSave,
                                  &pszParam[1]);
                    BAIL_ON_REG_ERROR(dwError);
                    LWREG_SAFE_FREE_STRING(pszParam);
                    pszParam = pszParamSave;
                    pszParamSave = NULL;
                    pParseState->bBracketPrefix = TRUE;
                }
                pParseState->tabState.eTabState = REGSHELL_TAB_ROOT_KEY;
                break;

            case REGSHELL_TAB_ROOT_KEY:
                pszTmp = strchr(pszParam, '\\'); 
                if (!pszTmp)
                {
                    /* Maybe just the undecorated root key, so \ terminate */
                    dwError = RegShellIsValidRootKey(
                                  pParseState->hReg,
                                  pszParam);
                    if (dwError == 0)
                    {
                        RegShellInputLineTerminate(el, pszInLine);
                        bProcessingCommand = FALSE;
                        pParseState->dwTabPressCount = 0;
                    }
                }

                /* First, check for valid root key in pParseState */
                dwError = RegShellIsValidRootKey(
                              pParseState->hReg,
                              RegShellGetRootKey(pParseState));
                if (dwError == 0)
                {
                    /* Root key is valid, move to the next level... */
                    pParseState->tabState.eTabState = REGSHELL_TAB_SUBKEY;
                    bHasRootKey = TRUE;
                }
    
                /* 
                 * Determine if valid root key was provided in input
                 * Separate root key from remainder of input parameters.
                 */
                if (pszParam)
                {
                    dwError = LwRtlCStringDuplicate(
                                  &pszInRootKey,
                                  pszParam);
                    BAIL_ON_REG_ERROR(dwError);
                    pszTmp = strchr(pszInRootKey, '\\'); 
                    if (pszTmp)
                    {
                        *pszTmp++ = '\0';
                        if (*pszTmp)
                        {
                            dwError = LwRtlCStringDuplicate(
                                          &pszParamSave,
                                          pszTmp);
                            BAIL_ON_REG_ERROR(dwError);
                        }
                    }
    
                    dwError = RegShellIsValidRootKey(
                                  pParseState->hReg,
                                  pszInRootKey);
                    if (dwError == 0)
                    {
                        pszRootKey = pszInRootKey;
                        LWREG_SAFE_FREE_STRING(pszParam);
                        pszParam = pszParamSave;
                    }
                    else
                    {
                        LWREG_SAFE_FREE_STRING(pszParamSave);
                    }
                }
    
                if (!RegShellGetRootKey(pParseState))
                {
                    if (dwError)
                    {
                        /* Above test for valid root key failed... */
                        dwError = LwRegEnumRootKeysW(
                                      pParseState->hReg,
                                      &ppwszRootKeys,
                                      &dwRootKeysCount);
                        BAIL_ON_REG_ERROR(dwError);
            
                        dwError = RegShellCompletionMatch(
                                      pszInRootKey ? pszInRootKey : "",
                                      ppwszRootKeys,
                                      dwRootKeysCount,
                                      NULL,
                                      &ppMatchArgs,
                                      &dwMatchArgsLen,
                                      &dwMatchBestIndex,
                                      &dwMatchBestLen);
                        if (dwError)
                        {
                            continue;
                        }
                                      
                        if (dwMatchArgsLen > 1)
                        {
                            for (i=0; i<dwMatchArgsLen; i++)
                            {
                                printf("%s\t", ppMatchArgs[i]);
                            }
                            printf("\n");
                            bProcessingCommand = FALSE;
                            break;
                        }
                        else if (dwMatchArgsLen == 1)
                        {
                         
                            pszRootKey = ppMatchArgs[0];
                            bProcessingCommand = FALSE;
                        }
                        else
                        {
                            /*
                             * No root key was provided, so prefix one onto the
                             * existing command line. Must re-read the line from
                             * libedit and parse cmd/args after this modification.
                             */
                            LWREG_SAFE_FREE_STRING(pParseState->pszFullRootKeyName);
                            pParseState->pszFullRootKeyName = 
                                strdup(HKEY_THIS_MACHINE);
                            pszTmp = (pszParam[0] == '\\') ?
                                HKEY_THIS_MACHINE : HKEY_THIS_MACHINE "\\";
                            RegShellPrependStringInput(
                                el, 
                                pszTmp,
                                pszParam);
                            LWREG_SAFE_FREE_STRING(pszInLine);
                            dwError = RegShellCompleteGetInput(el, &pszInLine);
                            BAIL_ON_REG_ERROR(dwError);
                            LWREG_SAFE_FREE_STRING(pszCommand);
                            LWREG_SAFE_FREE_STRING(pszParam);
                            
                            dwError = RegShellSplitCmdParam(
                                      pszInLine,
                                      &pszCommand,
                                      &pszParam);
                            BAIL_ON_REG_ERROR(dwError);
                            pParseState->tabState.pszCommand = pszCommand;
                            continue;  // ick, get rid of this.
                        }
                    }
    
                    /* Fill in matching root key on command line */
                    RegShellSetSubStringInputLine(el,
                        pszInRootKey ? pszInRootKey : "",
                        pszRootKey,
                        &dwLenAppended);
                    pParseState->dwTabPressCount = 0;
                    if (dwLenAppended > 0)
                    {
                        /* Add a \ termination only if characters were added */
                        RegShellSetInputLine(el, "\\");
                    }
        
                    /* Save root key in parse state context */
                    LWREG_SAFE_FREE_STRING(pParseState->pszFullRootKeyName);
                    dwError = LwRtlCStringDuplicate(
                                  &pParseState->pszFullRootKeyName, 
                                  pszRootKey);
                    BAIL_ON_REG_ERROR(dwError);
                }
    
                LWREG_SAFE_FREE_STRING(pszInLine);
                dwError = RegShellCompleteGetInput(el, &pszInLine);
                BAIL_ON_REG_ERROR(dwError);
                pParseState->tabState.eTabState = REGSHELL_TAB_SUBKEY;
                break;
    
            case REGSHELL_TAB_SUBKEY:
                if (pszParam)
                {
                    if (pParseState->pszDefaultKey && pszParam[0] == '\\')
                    {
                        LWREG_SAFE_FREE_STRING(pParseState->pszDefaultKey);
                    }
                }

                if (pszParam)
                {
                    dwError = RegShellGetLongestValidKey(
                                      pParseState->hReg,
                                      RegShellGetRootKey(pParseState),
                                      pParseState->pszDefaultKey,
                                      pszParam,
                                      &pszSubKey,
                                      &pszResidualSubKey);
                    /* Failed because subkey path is not valid */
                    if (dwError)
                    {
                        el_beep(el);
                        bProcessingCommand = FALSE;
                        break;
                    }
                }

                dwError = RegShellUtilGetKeys(
                              pParseState->hReg,
                              RegShellGetRootKey(pParseState),
                              NULL,
                              pszSubKey,
                              &ppwszSubKeys,
                              &dwSubKeyLen);
                BAIL_ON_REG_ERROR(dwError);

                /* 
                 * Partial key, find best match. Found unique matching key
                 * when dwMatchArgsLen == 1 
                 */
                dwMatchArgsLen = 0;
                dwError = RegShellCompletionMatch(
                              pszResidualSubKey,
                              ppwszSubKeys,
                              dwSubKeyLen,
                              RegShellGetRootKey(pParseState),
                              &ppMatchArgs,
                              &dwMatchArgsLen,
                              &dwMatchBestIndex,
                              &dwMatchBestLen);
                if (dwMatchArgsLen == 1)
                {
                    /* First element in return list is the subkey match. */
                    RegShellSetSubStringInputLine(el,
                        pszResidualSubKey,
                        ppMatchArgs[0],
                        &dwLenAppended);
                    LWREG_SAFE_FREE_STRING(pszInLine);
                    dwError = RegShellCompleteGetInput(el, &pszInLine);
                    BAIL_ON_REG_ERROR(dwError);
                    RegShellInputLineTerminate(el, pszInLine);
                    pParseState->dwTabPressCount = 0;
                }
                else if (dwMatchArgsLen > 0)
                {
                    /* Display list of parameters that match */
                    printf("\n");
                    for (i=0; i<dwMatchArgsLen; i++)
                    {
                        printf("%s\n", ppMatchArgs[i]);
                    }
                    ppMatchArgs[dwMatchBestIndex][dwMatchBestLen] = '\0';
                    RegShellSetSubStringInputLine(el,
                        pszResidualSubKey,
                        ppMatchArgs[dwMatchBestIndex],
                        &dwLenAppended);
                    printf("\n");
                    el_beep(el);
                    bProcessingCommand = FALSE;
                    break;
                }
                else if (dwSubKeyLen > 0)
                {
                    /* Nothing matches, dump full list of subkeys */
                    printf("\n");
                    for (i=0; i<dwSubKeyLen; i++)
                    {  
                        LWREG_SAFE_FREE_STRING(pszSubKey);
                        dwError = LwRtlCStringAllocateFromWC16String(
                                      &pszSubKey, ppwszSubKeys[i]);
                        BAIL_ON_REG_ERROR(dwError);
                        printf("%s\n", pszSubKey);
                        LWREG_SAFE_FREE_STRING(pszSubKey);
                    }
                    printf("\n");
                    el_beep(el);
                }
                bProcessingCommand = FALSE;
                break;

            default:
                bProcessingCommand = FALSE;
                break;
        }
    }
    while (bProcessingCommand);
    cldata->ePrevState = pParseState->tabState.eTabState;
    LWREG_SAFE_FREE_STRING(pszInLine);
    dwError = RegShellCompleteGetInput(el, &pszInLine);
    BAIL_ON_REG_ERROR(dwError);

    LWREG_SAFE_FREE_STRING(cldata->pszCompletePrevCmd);
    cldata->pszCompletePrevCmd = pszInLine;
    pszInLine = NULL;

cleanup:
    for (i=0; i<dwRootKeysCount; i++)
    {
        LWREG_SAFE_FREE_MEMORY(ppwszRootKeys[i]);
    }
    LWREG_SAFE_FREE_MEMORY(ppwszRootKeys);
    for (i=0; i<dwSubKeyLen; i++)
    {
        LWREG_SAFE_FREE_MEMORY(ppwszSubKeys[i]);
    }
    LWREG_SAFE_FREE_MEMORY(ppwszSubKeys);
    LWREG_SAFE_FREE_STRING(pszInRootKey);
    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_STRING(pszParam);
    LWREG_SAFE_FREE_STRING(pszResidualSubKey);
    LWREG_SAFE_FREE_STRING(pszInLine);
    RegShellFreeStrList(ppMatchArgs);
    RegShellCompletePrint(el, pParseState);
    return dwError;

error:
    goto cleanup;
}


static char *
pfnRegShellPromptCallback(EditLine *el)
{
    static char promptBuf[1024] = "";
    EDITLINE_CLIENT_DATA *cldata = NULL;

    el_get(el, EL_CLIENTDATA, (void *) &cldata);
    snprintf(promptBuf, sizeof(promptBuf), "\n%s%s%s%s ",
             cldata->pParseState->pszDefaultRootKeyName ?
                 cldata->pParseState->pszDefaultRootKeyName : "",
             cldata->pParseState->pszDefaultKey ? "\\" : "",
             cldata->pParseState->pszDefaultKey ?
             cldata->pParseState->pszDefaultKey : "\\",
             cldata->continuation ? ">>>" : ">");
    return promptBuf;
}


DWORD
RegShellExecuteCmdLine(
    PREGSHELL_PARSE_STATE pParseState,
    PSTR pszCmdLine,
    DWORD dwCmdLineLen)
{
    DWORD dwError = 0;
    DWORD dwNewArgc = 0;
    PSTR *pszNewArgv = NULL;

    dwError = RegIOBufferSetData(
                  pParseState->ioHandle,
                  pszCmdLine,
                  dwCmdLineLen);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellCmdlineParseToArgv(
                  pParseState,
                  &dwNewArgc,
                  &pszNewArgv);
    if (dwError == 0)
    {
        dwError = RegShellProcessCmd(pParseState,
                                     dwNewArgc,
                                     pszNewArgv);
    }
    if (dwError)
    {
        RegPrintError("regshell", dwError);
        dwError = 0;
    }
    RegShellCmdlineParseFree(dwNewArgc, pszNewArgv);
    LWREG_SAFE_FREE_STRING(pParseState->pszFullRootKeyName);
    LWREG_SAFE_FREE_STRING(pParseState->pszFullKeyPath);

cleanup:
    return dwError;

error:
    goto cleanup;

}


void
RegShellHandleSignalEditLine(int *signal, void *ctx)
{
#ifdef SIGWINCH
    if (*signal == SIGWINCH)
    {
        el_set((EditLine *) ctx, EL_REFRESH);
    }
#endif
    *signal = 0;
}


DWORD
RegShellProcessInteractiveEditLine(
    FILE *readFP,
    PREGSHELL_PARSE_STATE pParseState,
    PSTR pszProgramName)
{
    History *hist = NULL;
    HistEvent ev;
    EDITLINE_CLIENT_DATA el_cdata = {0};
    int num = 0;
    int ncontinuation = 0;
    int rv = 0;

    const char *buf = NULL;
    EditLine *el = NULL;
    BOOLEAN bHistFirst = FALSE;
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCmdLineLen = 0;
    DWORD dwEventNum = 0;
    PSTR pszCmdLine = NULL;
    PSTR pszNewCmdLine = NULL;
    PSTR pszNumEnd = NULL;
    PSTR pszHistoryFileDir = NULL;
    PSTR pszHistoryFileName = NULL;
    const char *hist_str = NULL;
    struct passwd *userPwdEntry = NULL;

    hist = history_init();
    history(hist, &ev, H_SETSIZE, 100);

    el = el_init(pszProgramName, stdin, stdout, stderr);

    /* Make configurable in regshellrc file */
    el_set(el, EL_EDITOR, "emacs");

    /* Signal handling in editline seems not to function... */
    el_set(el, EL_SIGNAL, 0);

#ifdef EL_ESC_CHAR
    /* Set escape character from \ to | */
    el_set(el, EL_ESC_CHAR, (int) REGSHELL_ESC_CHAR);
#endif

    /* Editline prompt function; display info from pParseState */
    el_set(el, EL_PROMPT, pfnRegShellPromptCallback);

    /* Set regshell context */
    el_cdata.pParseState = pParseState;
    el_set(el, EL_CLIENTDATA, (void *) &el_cdata);

    /* Setup history context, and load previous history file */
    el_set(el, EL_HIST, history, hist);

    /* Build fully qualified path for history file */
    userPwdEntry = getpwuid(getuid());
    if (userPwdEntry)
    {
        pszHistoryFileDir = userPwdEntry->pw_dir;
    }
    if (!pszHistoryFileDir)
    {
        pszHistoryFileDir = "/tmp";
    }

    dwError = RegAllocateMemory(
                  strlen(pszHistoryFileDir) + sizeof("/.regshell_history"),
                  (PVOID) &pszHistoryFileName);
    BAIL_ON_REG_ERROR(dwError);

    strcpy(pszHistoryFileName, pszHistoryFileDir);
    strcat(pszHistoryFileName, "/.regshell_history");
    
    /* Retrieve this from user's home directory */
    history(hist, &ev, H_LOAD, pszHistoryFileName);

    /*
     * Bind j, k in vi command mode to previous and next line, instead
     * of previous and next history.
     */
    el_set(el, EL_BIND, "-a", "k", "ed-prev-line", NULL);
    el_set(el, EL_BIND, "-a", "j", "ed-next-line", NULL);

    /*
     * Register complete function callback
     */
    el_set(el, 
           EL_ADDFN,
           "ed-complete",
           "Complete argument",
           pfnRegShellCompleteCallback);
    el_set(el, EL_BIND, "^I", "ed-complete", NULL);

    /*
     * Source the user's defaults file.
     */
    el_source(el, NULL);

    while ((buf = el_gets(el, &num))!=NULL && num!=0)
    {
        if (gCaughtSignal > 0)
        {
            RegShellHandleSignalEditLine(&gCaughtSignal, (LW_PVOID) el);
        }
        if (num>1 && buf[num-2] == REGSHELL_ESC_CHAR)
        {
            ncontinuation = 1;
        }
        else
        {
            ncontinuation = 0;
        }
        pParseState->tabState.eTabState = REGSHELL_TAB_CMD;
#if 0
printf("\n\n got line '%.*s'\n\n", num, buf);
#endif

#if 1 /* This mess needs to be put into a completion cleanup function */
        LWREG_SAFE_FREE_STRING(el_cdata.pParseState->pszDefaultKeyCompletion);
        for (i=0; el_cdata.ppszCompleteMatches && i<el_cdata.dwCompleteMatchesLen; i++)
        {
            LWREG_SAFE_FREE_STRING(el_cdata.ppszCompleteMatches[i]);
        }
        LWREG_SAFE_FREE_MEMORY(el_cdata.ppszCompleteMatches);
        LWREG_SAFE_FREE_STRING(el_cdata.pszCompletePrevCmd);
        el_cdata.dwEnteredTextLen = 0;
#endif


        el_cdata.continuation = ncontinuation;

        dwError = RegAllocateMemory(
                      sizeof(*pszNewCmdLine) * (num + 3),
                      (PVOID*)&pszNewCmdLine);
        BAIL_ON_REG_ERROR(dwError);

        if (ncontinuation)
        {
            num -= 2;
        }
        strncat(pszNewCmdLine, buf, num);

        /*
         * Put a terminating ] if a [ was provided, but only if one does not
         * already exist. This is needed to pass the syntax check of 
         * a subkey parameter.
         */

        if (pParseState->bBracketPrefix &&
            num > 0 &&
            pszNewCmdLine[num-2] != ']')
        {
            strcpy(&pszNewCmdLine[num-1], "]\n");
            num++;
        }

        dwCmdLineLen = num;
        pszCmdLine = pszNewCmdLine;
        pszNewCmdLine = NULL;
        pParseState->bBracketPrefix = FALSE;
        if (ncontinuation)
        {
            ncontinuation = 0;
            continue;
        }


        /*
         * Process history command recall (!nnn | !command syntax)
         */
        if (pszCmdLine[0] =='!')
        {
            /* !nnn case */
            if (isdigit((int)pszCmdLine[1]))
            {
                dwEventNum = strtol(&pszCmdLine[1], &pszNumEnd, 0);
                rv = history(hist, &ev, H_NEXT_EVENT, dwEventNum);
                if (rv == -1)
                {
                    printf("regshell: %d: event not found\n", dwEventNum);
                }
            }
            else
            {
                /*
                 * Handle !! command recall. !! recalls previous command,
                 * !!stuff appends "stuff" to end of previous command.
                 */
                if (pszCmdLine[1] == '!')
                {
                    rv = history(hist, &ev, H_FIRST);
                    if (rv == -1)
                    {
                        printf("regshell: !!: event not found\n");
                    }
                    else
                    {
                        bHistFirst = TRUE;
                    }
                }
                else
                {
                    /*
                     * !command case. Searches history for last occurrence
                     * of "command" in history list, and expands command line
                     * with that command.
                     */
                    hist_str = &pszCmdLine[1];
                    rv = history(hist, &ev, H_PREV_STR, hist_str);
                    if (rv == -1)
                    {
                        printf("regshell: %s: event not found\n", hist_str);
                    }
                }
            }

            if (rv == 0)
            {
                dwCmdLineLen = 0;

                if (bHistFirst)
                {
                    dwCmdLineLen += strlen(&pszCmdLine[2]);
                }
                else if (pszNumEnd)
                {
                    dwCmdLineLen += strlen(pszNumEnd);
                }
                dwCmdLineLen += strlen(ev.str);
                dwCmdLineLen++;

                dwError = RegAllocateMemory(
                              sizeof(*pszNewCmdLine) * dwCmdLineLen,
                              (PVOID*)&pszNewCmdLine);
                BAIL_ON_REG_ERROR(dwError);

                strcpy(pszNewCmdLine, ev.str);
                dwCmdLineLen = strlen(pszNewCmdLine);
                if (pszNewCmdLine[dwCmdLineLen-1] == '\n')
                {
                    pszNewCmdLine[dwCmdLineLen-1] = '\0';
                }

                if (bHistFirst)
                {
                    strcat(pszNewCmdLine, &pszNewCmdLine[2]);
                }
                else if (pszNumEnd)
                {
                    strcat(pszNewCmdLine, pszNumEnd);
                }
                dwCmdLineLen = strlen(pszNewCmdLine);
                LWREG_SAFE_FREE_STRING(pszCmdLine);
                pszCmdLine = pszNewCmdLine;
                pszNewCmdLine = NULL;

                /*
                 * Display command from history list
                 */
                printf("%s\n", pszCmdLine);
            }
        }

        if (pszCmdLine && pszCmdLine[0] != '\n')
        {
            rv = history(hist, &ev, H_ENTER, pszCmdLine);
            if (strcmp(pszCmdLine, "history\n") == 0)
            {
                for (rv = history(hist, &ev, H_LAST);
                     rv != -1;
                     rv = history(hist, &ev, H_PREV))
                {
                    fprintf(stdout, "%4d %s", ev.num, ev.str);
                }
            }
            else
            {
                dwError = RegShellExecuteCmdLine(
                              pParseState,
                              pszCmdLine,
                              dwCmdLineLen);
            }
        }
        LWREG_SAFE_FREE_STRING(pszCmdLine);
        dwCmdLineLen = 0;
    }

    /* Save current regshell history */
    history(hist, &ev, H_SAVE, pszHistoryFileName);

cleanup:
    LWREG_SAFE_FREE_STRING(pszHistoryFileName);
    el_end(el);
    history_end(hist);
    return dwError;

error:
    goto cleanup;
}



DWORD
RegShellProcessInteractive(
    FILE *readFP,
    PREGSHELL_PARSE_STATE parseState)
{
    CHAR cmdLine[8192] = {0};
    PSTR pszCmdLine = NULL;
    DWORD dwError = 0;
    PSTR pszTmpStr = NULL;
    BOOLEAN bDoPrompt = TRUE;
    BOOLEAN bFoundComment = FALSE;

    /* Interactive shell */
    do
    {
        if (bDoPrompt)
        {
            printf("%s%s%s> ",
                   parseState->pszDefaultRootKeyName ?
                       parseState->pszDefaultRootKeyName : "\\",
                   parseState->pszDefaultKey ? "\\" : "",
                   parseState->pszDefaultKey ?
                   parseState->pszDefaultKey : "\\");
            fflush(stdout);
        }
        pszCmdLine = fgets(cmdLine, sizeof(cmdLine)-1, readFP);
        if (pszCmdLine)
        {
            if (strlen(cmdLine) == 1)
            {
                if (readFP != stdin)
                {
                    bDoPrompt = FALSE;
                }
                continue;
            }

            /* Ignore leading white space or # comment on lines */
            for (pszTmpStr=cmdLine;
                 (int)*pszTmpStr && isspace((int)*pszTmpStr);
                 pszTmpStr++)
            {
                ;
            }

            if (pszTmpStr && *pszTmpStr == '#')
            {
                printf("%s%s", bFoundComment ? "" : "\n", cmdLine);
                bDoPrompt = FALSE;
                bFoundComment = TRUE;
                continue;
            }

            if (cmdLine[strlen(cmdLine)-1] == '\n')
            {
               cmdLine[strlen(cmdLine)-1] = '\0';
            }
            if (readFP != stdin)
            {
                printf("%s\n", cmdLine);
            }
            bDoPrompt = TRUE;
            bFoundComment = FALSE;

            dwError = RegShellExecuteCmdLine(
                          parseState,
                          cmdLine,
                          strlen(cmdLine));
            BAIL_ON_REG_ERROR(dwError);
        }
    } while (!feof(readFP));
cleanup:
    return dwError;

error:
    if (dwError)
    {
        RegPrintError("regshell", dwError);
    }
    goto cleanup;
}


int main(int argc, char *argv[])
{
    DWORD dwError = 0;
    PSTR pszInFile = NULL;
    PREGSHELL_PARSE_STATE parseState = NULL;
    FILE *readFP = stdin;
    DWORD indx = 0;
    struct sigaction action;

    setlocale(LC_ALL, "");
    dwError = RegShellInitParseState(&parseState);
    BAIL_ON_REG_ERROR(dwError);


    memset(&action, 0, sizeof(action));
    action.sa_handler = pfnRegShellSignal;
    action.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &action, NULL) < 0)
    {
        dwError = RegMapErrnoToLwRegError(errno);
        BAIL_ON_REG_ERROR(dwError);
    }

#ifdef SIGWINCH
    if (sigaction(SIGWINCH, &action, NULL) < 0)
    {
        dwError = RegMapErrnoToLwRegError(errno);
        BAIL_ON_REG_ERROR(dwError);
    }
#endif

    indx = 1;
    while (argc>1 && argv[indx][0] == '-')
    {
        if (strcmp(argv[indx], "--file") == 0 ||
            strcmp(argv[indx], "-f") == 0)
        {
            argc--;
            indx++;
            if (argc>1)
            {
                pszInFile = argv[indx++];
                argc--;
            }
            if (pszInFile)
            {
                readFP = fopen(pszInFile, "r");
                if (!readFP)
                {
                    fprintf(stderr, "Error opening file '%s'\n", pszInFile);
                    dwError = RegMapErrnoToLwRegError(errno);
                    BAIL_ON_REG_ERROR(dwError);
                }
            }
            parseState->eShellMode = REGSHELL_CMD_MODE_CMDFILE;
            dwError = RegShellProcessInteractive(readFP, parseState);
            BAIL_ON_REG_ERROR(dwError);
            if (readFP != stdin)
            {
                fclose(readFP);
            }
            dwError = 0;
            goto cleanup;
        }
        else if (strcmp(argv[indx], "--help") == 0 ||
            strcmp(argv[indx], "-h") == 0)
        {
            RegShellUsage(argv[0]);
            dwError = 0;
            goto cleanup;
        }
        else if (argv[indx][0] == '-')
        {
            printf("ERROR: unknown option %s\n", argv[indx]);
            RegShellUsage(argv[0]);
            dwError = 0;
            goto cleanup;
        }
    }

    if (argc == 1)
    {
        parseState->eShellMode = REGSHELL_CMD_MODE_INTERACTIVE;
        dwError = RegShellProcessInteractiveEditLine(
                      readFP,
                      parseState,
                      argv[0]);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        parseState->eShellMode = REGSHELL_CMD_MODE_CMDLINE;
        dwError = RegShellProcessCmd(parseState, argc, argv);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    RegShellCloseParseState(parseState);
    return dwError ? 1 : 0;

error:
    if (dwError)
    {
        RegPrintError("regshell", dwError);
    }
    goto cleanup;
}
