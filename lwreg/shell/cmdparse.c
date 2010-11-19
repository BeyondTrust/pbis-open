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
 *       cmdparse .c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry Shell command-line parser functionalty
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */


/*
 * regshell (Registry Shell) command grammar
 *
 * list_keys ["[KeyName]"]
 * list_values ["[KeyName]"]
 * add_key "[KeyName]"
 * cd "[KeyName]"
 * delete_key "[KeyName]"
 * delete_tree "[KeyName]"
 * set_value ["[KeyName]"] "ValueName" "Value" ["Value2"] ["Value3"] [...]
 * add_value ["[KeyName]"] "ValueName" type "Value" ["Value2"] ["Value3"] [...]
 *
 * Note: "KeyName" format is [HKEY_THIS_MACHINE/Subkey1/SubKey2]. ["[KeyName]"]
 * means the key parameter is optional.
 *
 * Token sequence from reglex for above comands:
 * list_keys:   REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * list_values: REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * add_key:     REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * add_value:   REGLEX_PLAIN_TEXT [REGLEX_REG_KEY]
 *                  REGLEX_REG_SZ|REGLEX_PLAIN_TEXT REGLEX_PLAIN_TEXT
 *                  REGLEX_REG_SZ ...
 * cd:          REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * delete_key:  REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * delete_tree: REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * set_value:   REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 *                  REGLEX_REG_SZ|REGLEX_PLAIN_TEXT
 *                  REGLEX_PLAIN_TEXT REGLEX_REG_SZ ...
 *
 * Supported types:
 *   REG_SZ
 *   REG_DWORD
 *   REG_MULTISZ
 *   REG_BINARY
 *
 *
 *     BNF Grammar for Registry Shell
 *
 * <regshell_command_line> ::=
 *     <regshell_command_verb> | <regshell_command_verb> <regshell_keyname> |
 *     <regshell_command_verb> <regshell_value_name>
 *     <regshell_value_type> <regshell_values>
 *
 * <regshell_command_verb> ::= "list_keys" | "list_values" | "add_key" |
 *                             "add_value" | "cd" | "delete_key" |
 *                             "delete_tree" | "set_value"
 *
 * <regshell_key_name> ::= "[" <key_path> "]"
 *   <key_path> ::= <name_chars> <end_of_line>
 *
 * <regshell_value_name> ::= <name_chars>
 *
 * <name_chars> ::= <name_char> <name_chars>
 * <name_char> ::= [a-zA-Z0-9\\/_-{}\*\| \$+=#<>~&®]
 * <end_of_line> ::= "\r\n" | "\r"
 *
 * <regshell_value_type> ::= REG_SZ | REG_DWORD | REG_MULTISZ | REG_BINARY
 *
 * <regshell_values> ::= <value_data> <regshell_values>
 *   <value_data> ::= "'" <value_chars> "'" <end_of_line>
 *   <value_chars> ::= <value_char> <value_chars>
 *   <value_char> ::= UCS2-CHAR | UTF-8-CHAR
 */


#if 0
#define _LW_DEBUG
#endif
#include "regshell.h"

typedef enum _REGSHELL_HEXSTRING_STATE_E
{
    REGSHELL_HEX_START,
    REGSHELL_HEX_LEADING_SPACE,
    REGSHELL_HEX_HEXDIGIT,
    REGSHELL_HEX_SPACE_SEPARATOR,
    REGSHELL_HEX_COMMA_SEPARATOR,
    REGSHELL_HEX_END,
    REGSHELL_HEX_ERROR,
    REGSHELL_HEX_STOP,
} REGSHELL_HEXSTRING_STATE_E;


typedef enum _REGSHELL_CMDLINE_STATE_E
{
    REGSHELL_CMDLINE_STATE_FIRST = 0,
    REGSHELL_CMDLINE_STATE_VERB,
    REGSHELL_CMDLINE_STATE_LIST_KEYS,
    REGSHELL_CMDLINE_STATE_LIST_KEYS_REGKEY,
    REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP,

    REGSHELL_CMDLINE_STATE_CD,
    REGSHELL_CMDLINE_STATE_CD_REGKEY,
    REGSHELL_CMDLINE_STATE_CD_STOP,

    REGSHELL_CMDLINE_STATE_ADDKEY,
    REGSHELL_CMDLINE_STATE_ADDKEY_REGKEY,
    REGSHELL_CMDLINE_STATE_ADDKEY_STOP,

    REGSHELL_CMDLINE_STATE_ADDVALUE,
    REGSHELL_CMDLINE_STATE_ADDVALUE_KEYNAME,
    REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME,
    REGSHELL_CMDLINE_STATE_ADDVALUE_TYPE,
    REGSHELL_CMDLINE_STATE_ADDVALUE_OPTION,
    REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE,
    REGSHELL_CMDLINE_STATE_ADDVALUE_STOP,

    REGSHELL_CMDLINE_STATE_IMPORT,
    REGSHELL_CMDLINE_STATE_IMPORT_FILE,
    REGSHELL_CMDLINE_STATE_IMPORT_STOP,
} REGSHELL_CMDLINE_STATE_E, *PREGSHELL_CMDLINE_STATE_E;


static REGSHELL_CMD_ID shellCmds[] = {
    { "", REGSHELL_CMD_NONE                   },
    { "list_keys", REGSHELL_CMD_LIST_KEYS     },
    { "dir", REGSHELL_CMD_DIRECTORY           },
    { "ls", REGSHELL_CMD_LIST                 },
    { "list_values", REGSHELL_CMD_LIST_VALUES },
    { "add_key", REGSHELL_CMD_ADD_KEY         },
    { "add_value", REGSHELL_CMD_ADD_VALUE     },
    { "cd", REGSHELL_CMD_CHDIR                },
    { "delete_key", REGSHELL_CMD_DELETE_KEY   },
    { "delete_value", REGSHELL_CMD_DELETE_VALUE },
    { "delete_tree", REGSHELL_CMD_DELETE_TREE },
    { "set_value", REGSHELL_CMD_SET_VALUE     },
    { "set_hive", REGSHELL_CMD_SET_HIVE       },
    { "pwd", REGSHELL_CMD_PWD                 },
    { "help", REGSHELL_CMD_HELP               },
    { "exit", REGSHELL_CMD_QUIT               },
    { "quit", REGSHELL_CMD_QUIT               },
    { "import", REGSHELL_CMD_IMPORT           },
    { "export", REGSHELL_CMD_EXPORT           },
    { "upgrade", REGSHELL_CMD_UPGRADE         },
    { "cleanup", REGSHELL_CMD_CLEANUP         },
};


DWORD
RegShellCmdEnumToString(
    REGSHELL_CMD_E cmdEnum,
    PCHAR cmdString,
    DWORD dwCmdStringLen)
{
    DWORD dwError = 0;

    if (cmdEnum<sizeof(shellCmds)/sizeof(shellCmds[0]))
    {
        cmdString[0] = '\0';
        strncat(cmdString, shellCmds[cmdEnum].pszCommand, dwCmdStringLen-1);
    }
    else
    {
        snprintf(cmdString, dwCmdStringLen, "Invalid Command: %d", cmdEnum);
        dwError = LWREG_ERROR_INVALID_CONTEXT;
    }
    return dwError;
}


DWORD
RegShellCmdStringToEnum(
    PCHAR cmdString,
    PREGSHELL_CMD_E pCmdEnum)
{


    DWORD i = 0;
    DWORD dwError = LWREG_ERROR_INVALID_CONTEXT;
    REGSHELL_CMD_E cmd = REGSHELL_CMD_NONE;

    BAIL_ON_INVALID_POINTER(pCmdEnum);

    for (i=0; i<sizeof(shellCmds)/sizeof(shellCmds[0]); i++)
    {
        if (strcasecmp(cmdString, shellCmds[i].pszCommand) == 0)
        {
            cmd = shellCmds[i].eCommand;
            dwError = 0;
            break;
        }
    }
    *pCmdEnum = cmd;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCmdParseCommand(
    REGSHELL_CMD_E cmd,
    PREGSHELL_CMD_ITEM *pCmdItem)
{
    DWORD dwError = 0;
    PREGSHELL_CMD_ITEM pNewCmdItem = NULL;

    BAIL_ON_INVALID_POINTER(pCmdItem);

    dwError = RegAllocateMemory(sizeof(*pNewCmdItem), (PVOID*)&pNewCmdItem);
    BAIL_ON_REG_ERROR(dwError);

    pNewCmdItem->command = cmd;
    *pCmdItem = pNewCmdItem;

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pNewCmdItem);
    goto cleanup;
}


DWORD
RegShellCmdParseFree(
    PREGSHELL_CMD_ITEM pCmdItem)
{
    DWORD dwError = 0;
    DWORD i = 0;
    BAIL_ON_INVALID_POINTER(pCmdItem);

    LWREG_SAFE_FREE_MEMORY(pCmdItem->keyName);
    LWREG_SAFE_FREE_MEMORY(pCmdItem->valueName);
    LWREG_SAFE_FREE_MEMORY(pCmdItem->binaryValue);

    for (i=0; i<pCmdItem->argsCount; i++)
    {
        LWREG_SAFE_FREE_MEMORY(pCmdItem->args[i]);
    }
    LWREG_SAFE_FREE_MEMORY(pCmdItem->args);
    LWREG_SAFE_FREE_MEMORY(pCmdItem);

cleanup:
    return dwError;

error:
    goto cleanup;
}


/*
 * Inputs:
 * pszKeyName
 *   [HKEY_THIS_MACHINE\sub1\sub2\sub3]
 *   sub1\sub2
 *   [sub1\sub2\]
 * cmd
 *   REGSHELL_CMD_CHDIR
 *
 * Outputs:
 * pParseState->pszFullRootKeyName 
 *    "HKEY_THIS_MACHINE"
 *
 * pCmdItem
 * pCmdItem->keyName
 *    sub1\sub2\sub3
 */
DWORD
RegShellCmdParseKeyName(
    PREGSHELL_PARSE_STATE pParseState,
    REGSHELL_CMD_E cmd,
    PSTR pszInKeyName,
    PREGSHELL_CMD_ITEM *pRetCmdItem)
{
    BOOLEAN bFoundRootKey = FALSE;
    DWORD dwError = 0;
    DWORD dwRootKeyError = 0;
    DWORD dwRootKeyLen = 0;
    HKEY hRootKey = NULL;
    PREGSHELL_CMD_ITEM pCmdItem = NULL;
    PSTR pszCursor = NULL;
    PSTR pszKeyName = NULL; /* Working copy of original pszInKeyName */
    PSTR pszRootKeyName = NULL;
    PSTR pszSubKey = NULL;
    
    BAIL_ON_INVALID_POINTER(pParseState);
    BAIL_ON_INVALID_POINTER(pszInKeyName);

    dwError = RegShellCmdParseCommand(cmd, &pCmdItem);
    BAIL_ON_REG_ERROR(dwError);

    /* Find and strip off [] from around key when present */
    pszKeyName = pszInKeyName;
    if (*pszKeyName == '[')
    {
        pszKeyName++;
    }
    dwError = RegCStringDuplicate(&pszKeyName, pszKeyName);
    BAIL_ON_REG_ERROR(dwError);
    
    pszCursor = pszKeyName + strlen(pszKeyName) - 1;
    if (*pszCursor == ']')
    {
        *pszCursor-- = '\0';
    }

    /* Rip off trailing \ characters from key path */
    while (pszCursor > pszKeyName && *pszCursor == '\\')
    {
        *pszCursor-- = '\0';
    }

    /* 
     * Find \ separator, and if found, determine if stuff before \
     * is a valid root key. If no \ is found, test the stuff passed in
     * for being a root key anyway.
     */
    pszCursor = strchr(pszKeyName, '\\');
    if (pszCursor)
    {
        dwRootKeyLen = pszCursor - pszKeyName;
    }
    else
    {
        dwRootKeyLen = strlen(pszKeyName);
    }
    dwError = RegCStringAllocatePrintf(
                  &pszRootKeyName,
                  "%.*s",
                  dwRootKeyLen,
                  pszKeyName);
    BAIL_ON_REG_ERROR(dwError);
    dwRootKeyError = RegOpenKeyExA(
                         pParseState->hReg,
                         NULL,
                         pszRootKeyName,
                         0,
                         KEY_READ,
                         &hRootKey);
    if (dwRootKeyError == 0)
    {
        /* Valid root key found */
        RegCloseKey(pParseState->hReg, hRootKey);
    
        LWREG_SAFE_FREE_STRING(pParseState->pszFullRootKeyName);
        pParseState->pszFullRootKeyName = pszRootKeyName;
        pszRootKeyName = NULL;
        bFoundRootKey = TRUE;
    }
    else {
        LWREG_SAFE_FREE_STRING(pszRootKeyName);
    }

    if (!bFoundRootKey)
    {
        /* 
         * Path provided does not start with a root key.
         * Assume this is a subkey, and use it as such.
         */
        dwError = RegCStringDuplicate(&pszSubKey, pszKeyName);
        BAIL_ON_REG_ERROR(dwError);

        /* Use default HKEY_THIS_MACHINE root key */
        LWREG_SAFE_FREE_STRING(pParseState->pszFullRootKeyName);
        dwError = RegCStringDuplicate(
                      &pParseState->pszFullRootKeyName,
                      HKEY_THIS_MACHINE);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (pszCursor)
    {
        /* The stuff following the valid root key is the subkey */
        dwError = RegCStringDuplicate(&pszSubKey, pszCursor);
    }

    pCmdItem->keyName = pszSubKey;
    *pRetCmdItem = pCmdItem;

cleanup:
    LWREG_SAFE_FREE_STRING(pszKeyName);
    LWREG_SAFE_FREE_STRING(pszRootKeyName);
    return dwError;

error:
    LWREG_SAFE_FREE_STRING(pszSubKey);
    RegShellCmdParseFree(pCmdItem);
    goto cleanup;
}


DWORD
RegShellParseStringType(
    PCHAR pszType,
    PREG_DATA_TYPE pType,
    PDWORD pBackendType)
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD backendType = 0;
    REG_DATA_TYPE type = REG_NONE;

    static REGSHELL_REG_TYPE_ENTRY typeStrs[] = {
        { "REG_DWORD", REG_DWORD, RRF_RT_REG_DWORD             },
        { "REG_SZ", REG_SZ, RRF_RT_REG_SZ                      },
        { "REG_BINARY", REG_BINARY, RRF_RT_REG_BINARY          },
        { "REG_NONE", REG_NONE, RRF_RT_REG_NONE                },
        { "REG_EXPAND_SZ", REG_EXPAND_SZ, RRF_RT_REG_EXPAND_SZ },
        { "REG_MULTI_SZ", REG_MULTI_SZ, RRF_RT_REG_MULTI_SZ    },
        { "REG_RESOURCE_LIST", REG_RESOURCE_LIST, RRF_RT_ANY   },
        { "REG_FULL_RESOURCE_DESCRIPTOR",
          REG_FULL_RESOURCE_DESCRIPTOR,
          RRF_RT_ANY                                           },
        { "REG_RESOURCE_REQUIREMENTS_LIST",
           REG_RESOURCE_REQUIREMENTS_LIST ,
           RRF_RT_ANY                                          },
        { "REG_QUADWORD", REG_QWORD, RRF_RT_QWORD              },
        { "REG_KEY", REG_KEY, RRF_RT_REG_SZ                    },
        { "REG_KEY_DEFAULT", REG_KEY_DEFAULT, RRF_RT_REG_SZ    },
        { "REG_PLAIN_TEXT", REG_PLAIN_TEXT, RRF_RT_REG_SZ      },
    };

    BAIL_ON_INVALID_POINTER(pType);

    for (i=0; i<sizeof(typeStrs)/sizeof(REGSHELL_REG_TYPE_ENTRY); i++)
    {
        if (strcasecmp(pszType, typeStrs[i].pszType) == 0)
        {
            type = typeStrs[i].type;
            backendType = typeStrs[i].backendType;
            break;
        }
    }
    *pType = type;
    if (pBackendType)
    {
        *pBackendType = backendType;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


/*
 * Return the longest contiguous extent of hex digits before
 * a separator character or the end of string is found
 */
DWORD
RegShellHexStringExtentLen(
    PSTR pszHexString,
    PDWORD pdwHexLen)
{
    DWORD dwIndx = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pszHexString);

    for (dwIndx=0; pszHexString[dwIndx]; dwIndx++)
    {
        if (!isxdigit((int) pszHexString[dwIndx]))
        {
            break;
        }
        dwCount++;
    }

    *pdwHexLen = dwCount;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellImportDwordString(
    PCHAR pszHexString,
    PUCHAR *ppBinaryValue,
    PDWORD pBinaryValueLen)
{
    DWORD dwError = 0;
    DWORD dwBase = 10;
    PDWORD pdwValue = NULL;
    PSTR pszEndValue = NULL;
    PSTR pszPtr = NULL;
    DWORD dwErrno = 0;
    unsigned long int ulValue = 0;

    BAIL_ON_INVALID_POINTER(pszHexString);

    /* Ignore leading/trailing whitespace */
    while (*pszHexString && isspace((int) *pszHexString))
    {
        pszHexString++;
    }
    ulValue = strtoul(pszHexString, &pszEndValue, 0);

    pszPtr = pszEndValue;
    while (pszPtr && *pszPtr)
    {
        if (!isspace((int) *pszPtr))
        {
            /* 
             * Trap an invalid character was present in the string 
             * number value, ignore trailing white space.
             */
            dwError = RegMapErrnoToLwRegError(EINVAL);
            BAIL_ON_REG_ERROR(dwError);
        }
        pszPtr++;
    }
    if (pszEndValue)
    {
        *pszEndValue = '\0';
    }

    if (strncmp(pszHexString, "0x", 2) == 0)
    {
        dwBase = 16;
    }

    /*
     * Both 0xffffffff and 4294967295 are 10 characters long
     * so definitely know this is an overflow case.
     */
    if (strlen(pszHexString) > 10)
    {
        dwError = RegMapErrnoToLwRegError(ERANGE);
        BAIL_ON_REG_ERROR(dwError);
    }

    /*
     * Explicitly set errno to 0 here. When previous value entered
     * was a legitimate overflow (e.g 4294967297), errno is set.
     * Now ULONG_MAX (4294967295) is entered. errno is not set, because
     * no error occurred, but errno is still the value from the previous
     * error, and the legitimate ULONG_MAX value is flagged as an error.
     */
    errno = 0;
    ulValue = strtoul(pszHexString, &pszEndValue, dwBase);
    dwErrno = errno;
    if (ulValue == ULONG_MAX && dwErrno == ERANGE)
    {
        dwError = RegMapErrnoToLwRegError(dwErrno);
        BAIL_ON_REG_ERROR(dwError);
    }

    /* For 64-bit systems, catch overflow for DWORD (UINT32_MAX) */
    if (ulValue > LW_MAXDWORD)
    {
        dwError = RegMapErrnoToLwRegError(ERANGE);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegAllocateMemory(sizeof(DWORD), (PVOID*)&pdwValue);
    BAIL_ON_REG_ERROR(dwError);

    *pdwValue = (DWORD) ulValue;
    *ppBinaryValue = (PUCHAR) pdwValue;
    *pBinaryValueLen = sizeof(DWORD);
    
cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pdwValue);
    goto cleanup;
}


DWORD
RegShellImportBinaryString(
    PCHAR pszHexString,
    PUCHAR *ppBinaryValue,
    PDWORD pBinaryValueLen)
{
    DWORD dwError = 0;
    CHAR pszHexArray[3];
    REGSHELL_HEXSTRING_STATE_E state = REGSHELL_HEX_START;
    PCHAR cursor = NULL;
    DWORD hexDigitCount = 0;
    DWORD hexDigitModCount = 0;
    PUCHAR binaryValue = NULL;
    DWORD binaryValueLen = 0;
    DWORD dwHexStringLen = 0;

    BAIL_ON_INVALID_POINTER(pszHexString);
    BAIL_ON_INVALID_POINTER(ppBinaryValue);
    BAIL_ON_INVALID_POINTER(pBinaryValueLen);

    dwError = RegAllocateMemory(sizeof(UCHAR) * (strlen(pszHexString) / 2+1), (PVOID*)&binaryValue);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellHexStringExtentLen(
                  pszHexString,
                  &dwHexStringLen);
    BAIL_ON_REG_ERROR(dwError);

    if (dwHexStringLen % 2)
    {
        hexDigitModCount = 1;
    }
    cursor = pszHexString;
    do
    {
        if (*cursor == '\0')
        {
            if (hexDigitCount > 0)
            {
                state = REGSHELL_HEX_END;
            }
            else
            {
                break;
            }
        }

        switch (state)
        {
            case REGSHELL_HEX_START:
                if (hexDigitModCount)
                {
                    pszHexArray[0] = '0';
                    pszHexArray[1] = '\0';
                }
                hexDigitCount = hexDigitModCount;
                hexDigitModCount = 0;
                if (*cursor == ' ')
                {
                    state = REGSHELL_HEX_LEADING_SPACE;
                }
                else if (isxdigit((int)*cursor))
                {
                    state = REGSHELL_HEX_HEXDIGIT;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_LEADING_SPACE:
                if (*cursor == ' ')
                {
                    cursor++;
                }
                else if (isxdigit((int)*cursor))
                {
                    state = REGSHELL_HEX_HEXDIGIT;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_HEXDIGIT:
                if (isxdigit((int)*cursor))
                {
                    pszHexArray[hexDigitCount++] = *cursor++;
                    pszHexArray[hexDigitCount] = '\0';
                    if (hexDigitCount == 2 && isxdigit((int)*cursor))
                    {
                        state = REGSHELL_HEX_END;
                    }
                }
                else if (*cursor == ' ')
                {
                    state = REGSHELL_HEX_SPACE_SEPARATOR;
                }
                else if (*cursor == ',')
                {
                    state = REGSHELL_HEX_COMMA_SEPARATOR;
                    cursor++;
                }
                else if (*cursor == '\n')
                {
                    dwError = RegShellHexStringExtentLen(
                                  cursor,
                                  &dwHexStringLen);
                    BAIL_ON_REG_ERROR(dwError);
                    if (dwHexStringLen % 2)
                    {
                        hexDigitModCount = 1;
                    }
                    state = REGSHELL_HEX_END;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_SPACE_SEPARATOR:
                if (*cursor == ' ')
                {
                    cursor++;
                }
                else if (isxdigit((int)*cursor))
                {
                    dwError = RegShellHexStringExtentLen(
                                  cursor,
                                  &dwHexStringLen);
                    BAIL_ON_REG_ERROR(dwError);
                    if (dwHexStringLen % 2)
                    {
                        hexDigitModCount = 1;
                    }
                    state = REGSHELL_HEX_END;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_COMMA_SEPARATOR:
                if (*cursor == ' ')
                {
                    state = REGSHELL_HEX_SPACE_SEPARATOR;
                }
                else if (isxdigit((int)*cursor))
                {
                    dwError = RegShellHexStringExtentLen(
                                  cursor,
                                  &dwHexStringLen);
                    BAIL_ON_REG_ERROR(dwError);
                    if (dwHexStringLen % 2)
                    {
                        hexDigitModCount = 1;
                    }
                    state = REGSHELL_HEX_END;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_END:
                if (hexDigitCount < 2)
                {
                    pszHexArray[1] = pszHexArray[0];
                    pszHexArray[0] = ' ';
                }
                binaryValue[binaryValueLen] =
                    (UCHAR) strtoul(pszHexArray, NULL, 16);
                binaryValueLen++;

                hexDigitCount = 0;
                pszHexArray[hexDigitCount] = '\0';

                if (*cursor == '\0' || *cursor == '\n')
                {
                    state = REGSHELL_HEX_STOP;
                }
                else
                {
                    state = REGSHELL_HEX_START;
                }
                break;

            case REGSHELL_HEX_ERROR:
                printf("RegShellImportBinaryString: ERROR: "
                       "invalid character '%c'\n", *cursor);
                state = REGSHELL_HEX_STOP;
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_HEX_STOP:
            default:
               break;
        }
    } while (state != REGSHELL_HEX_STOP);
    if (binaryValueLen > 0)
    {
        *ppBinaryValue = binaryValue;
        *pBinaryValueLen = binaryValueLen;
    }
    else
    {
        LWREG_SAFE_FREE_MEMORY(binaryValue);
    }

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(binaryValue);
    goto cleanup;
}


DWORD
RegShellCmdParseValueName(
    PREGSHELL_PARSE_STATE pParseState,
    REGSHELL_CMD_E cmd,
    DWORD argc,
    PCHAR *argv,
    PREGSHELL_CMD_ITEM *pRetCmdItem)
{
    DWORD dwError = 0;
    DWORD argCount = 0;
    DWORD i = 0;
    DWORD binaryValueLen = 0;
    DWORD argIndx = 2;
    BOOLEAN bFullPath = FALSE;
    PSTR pszValue = NULL;
    PSTR pszType = NULL;
    PSTR pszString = NULL;
    PSTR pszKeyName = NULL;
    PUCHAR binaryValue = NULL;
    PBYTE multiString = NULL;
    ssize_t multiStringLen = 0;
    PREGSHELL_CMD_ITEM pCmdItem = NULL;

    BAIL_ON_INVALID_POINTER(argv);
    BAIL_ON_INVALID_POINTER(pRetCmdItem);

    if (argv[2][0] == '[')
    {
        dwError = RegShellCmdParseKeyName(
                      pParseState,
                      cmd,
                      argv[2],
                      &pCmdItem);
        BAIL_ON_REG_ERROR(dwError);
        argIndx = 3;
    }
    else
    {
        dwError = RegShellCmdParseCommand(
                      cmd,
                      &pCmdItem);
        BAIL_ON_REG_ERROR(dwError);
    }
    BAIL_ON_INVALID_POINTER(pCmdItem);

    pszValue = argv[argIndx++];

    /* format: add_value [[subkeyname]] value_name */
    if (pCmdItem->command == REGSHELL_CMD_DELETE_VALUE)
    {
        dwError = RegCStringDuplicate(&pCmdItem->valueName, pszValue);
        BAIL_ON_REG_ERROR(dwError);
        *pRetCmdItem = pCmdItem;
        return 0;
    }
    if (pCmdItem->command != REGSHELL_CMD_SET_VALUE)
    {
        pszType = argv[argIndx++];
    }
    else
    {
        pszKeyName = pCmdItem->keyName;
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
                      pszValue,
                      &pCmdItem->type,
                      NULL,
                      NULL);
        BAIL_ON_REG_ERROR(dwError);
    }
    if (argIndx >= argc)
    {
        dwError = LWREG_ERROR_INVALID_CONTEXT;
        goto error;
    }
    BAIL_ON_REG_ERROR(dwError);

    argCount = argc - argIndx;
    dwError = RegCStringDuplicate(&pCmdItem->valueName, pszValue);
    BAIL_ON_REG_ERROR(dwError);

    if (pszType)
    {
        dwError = RegShellParseStringType(
                      pszType,
                      &pCmdItem->type,
                      &pCmdItem->backendType);
        BAIL_ON_REG_ERROR(dwError);
    }

    switch (pCmdItem->type)
    {
        case REG_BINARY:
            if (argCount == 0)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            break;
        case REG_DWORD:
            if (argCount != 1)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            break;

        case REG_SZ:
            if (argCount > 1)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            break;
        default:
            break;
    }

    if (argCount > 0)
    {
        dwError = RegAllocateMemory(sizeof(*pCmdItem->args) * (argCount + 1), (PVOID*)&pCmdItem->args);
        BAIL_ON_REG_ERROR(dwError);
        pCmdItem->argsCount = argCount;

        for (i=0; i<argCount; i++)
        {
            dwError = RegCStringDuplicate(&pCmdItem->args[i], argv[i+argIndx]);
            BAIL_ON_REG_ERROR(dwError);
            pCmdItem->args[i+1] = NULL;
            BAIL_ON_REG_ERROR(dwError);
        }
    }

    if (pCmdItem->binaryValue)
    {
        RegMemoryFree(pCmdItem->binaryValue);
        pCmdItem->binaryValue = NULL;
    }

    switch (pCmdItem->type)
    {
        case REG_DWORD:

            dwError = RegShellImportDwordString(
                            pCmdItem->args[0],
                            &binaryValue,
                            &binaryValueLen);
            BAIL_ON_REG_ERROR(dwError);

            pCmdItem->binaryValue = binaryValue;
            pCmdItem->binaryValueLen = sizeof(DWORD);
            binaryValue = NULL;
            break;

        case REG_BINARY:
            dwError = RegShellImportBinaryString(
                            pCmdItem->args[0],
                            &binaryValue,
                            &binaryValueLen);
            BAIL_ON_REG_ERROR(dwError);

            pCmdItem->binaryValue = binaryValue;
            pCmdItem->binaryValueLen = binaryValueLen;
            binaryValue = NULL;
            break;

        case REG_SZ:
            dwError = RegCStringDuplicate(&pszString, pCmdItem->args[0]);
            BAIL_ON_REG_ERROR(dwError);
            pCmdItem->binaryValue = (PUCHAR) pszString;
            pCmdItem->binaryValueLen = strlen(pszString);
            pszString = NULL;
            BAIL_ON_REG_ERROR(dwError);
            break;

        case REG_MULTI_SZ:
            dwError = RegMultiStrsToByteArray(pCmdItem->args,
                                              &multiString,
                                              &multiStringLen);
            BAIL_ON_REG_ERROR(dwError);

            pCmdItem->binaryValue = (PUCHAR) multiString;
            pCmdItem->binaryValueLen = multiStringLen;
            multiString = NULL;
            break;

        default:
            break;
    }

    *pRetCmdItem = pCmdItem;
    pCmdItem = NULL;

cleanup:
    LWREG_SAFE_FREE_STRING(pszString);
    LWREG_SAFE_FREE_MEMORY(binaryValue);
    LWREG_SAFE_FREE_MEMORY(multiString);
    RegShellCmdParseFree(pCmdItem);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellDumpCmdItem(
    PREGSHELL_CMD_ITEM rsItem)
{
    CHAR tokenName[128];
    PSTR pszDumpData = NULL;
    DWORD dumpDataLen = 0;
    DWORD i;
    DWORD dwError = 0;
    PSTR *outMultiSz = NULL;


    BAIL_ON_INVALID_POINTER(rsItem);

    RegShellCmdEnumToString(rsItem->command, tokenName, sizeof(tokenName));
    printf("DumpCmd: command=%s\n", tokenName);
    if (rsItem->keyName)
    {
        printf("DumpCmd: keyName=%s\n", rsItem->keyName);
    }
    if (rsItem->valueName)
    {
        printf("DumpCmd: valueName=%s\n", rsItem->valueName);
    }
    if (rsItem->type != REG_NONE)
    {
        RegExportBinaryTypeToString(
            rsItem->type,
            tokenName,
            FALSE);
        printf("DumpCmd: type=%s\n", tokenName);
    }
    for (i=0; i<rsItem->argsCount; i++)
    {
        printf("DumpCmd: args[%d]='%s'\n", i, rsItem->args[i]);
    }

    switch (rsItem->type)
    {
        case REG_SZ:
            printf("DumpCmd: value = '%s'\n", (CHAR *) rsItem->binaryValue);
            break;

        case REG_DWORD:
            printf("DumpCmd: value = '%08x'\n", *(DWORD*) rsItem->binaryValue);
            break;

        case REG_BINARY:
            dwError = RegExportBinaryData(
                          REG_SZ,
                          "test_value",
                          REG_BINARY,
                          rsItem->binaryValue,
                          rsItem->binaryValueLen,
                          &pszDumpData,
                          &dumpDataLen);
            BAIL_ON_REG_ERROR(dwError);
            printf("RegShellDumpCmdItem: '%s'\n", pszDumpData);
            break;

        case REG_MULTI_SZ:
            dwError = RegByteArrayToMultiStrs(
                          rsItem->binaryValue,
                          rsItem->binaryValueLen,
                          &outMultiSz);
            BAIL_ON_REG_ERROR(dwError);

            for (i=0; outMultiSz[i]; i++)
            {
                printf("DumpCmd: outMultiSz[%d] = '%s'\n",
                       i, outMultiSz[i]);
            }


        default:
            break;
    }

cleanup:
    if (outMultiSz)
    {
        for (i=0; outMultiSz[i]; i++)
        {
            LWREG_SAFE_FREE_STRING(outMultiSz[i]);
        }
        LWREG_SAFE_FREE_MEMORY(outMultiSz);
    }
    LWREG_SAFE_FREE_STRING(pszDumpData);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellPszIsValidKey(
    PREGSHELL_PARSE_STATE pParseState,
    PSTR pszKey,
    PBOOLEAN pbIsValid)
{

    DWORD dwError = 0;
    PSTR pszRootKey = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszPtr = NULL;
    
    BAIL_ON_INVALID_POINTER(pParseState);
    BAIL_ON_INVALID_POINTER(pszKey);
    BAIL_ON_INVALID_POINTER(pbIsValid);

    if (*pszKey == '[' && strchr(pszKey, '\\'))
    {
        /*  Assume stuff between [ and \ is root key */
        dwError = RegCStringDuplicate(
                      (LW_PVOID) &pszRootKey, &pszKey[1]);
        BAIL_ON_REG_ERROR(dwError);
        pszPtr = strchr(pszRootKey, '\\');
        *pszPtr++ = '\0';
        
        dwError = RegCStringDuplicate(
                      (LW_PVOID) &pszSubKey, pszPtr);
        BAIL_ON_REG_ERROR(dwError);
        pszPtr = strrchr(pszSubKey, ']');
        if (pszPtr)
        {
            *pszPtr = '\0';
        }
    }
    else
    {
        /* Assume value is subkey */
        dwError = RegCStringDuplicate(
                      (LW_PVOID) &pszSubKey, pszKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    dwError = RegShellIsValidKey(
                  pParseState->hReg,
                  pszRootKey,
                  pszSubKey);
    *pbIsValid = (dwError == 0) ? TRUE : FALSE;
    
cleanup:
    LWREG_SAFE_FREE_STRING(pszRootKey);
    LWREG_SAFE_FREE_STRING(pszSubKey);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCmdParse(
    PREGSHELL_PARSE_STATE pParseState,
    DWORD argc,
    PCHAR *argv,
    PREGSHELL_CMD_ITEM *parsedCmd)
{
    REGSHELL_CMD_E cmd = 0;
    DWORD dwError = 0;
    PSTR pszCommand = NULL;
    PSTR pszSubKeyName = "";
    PREGSHELL_CMD_ITEM pCmdItem = NULL;
    DWORD dwArgc = 2;
    DWORD dwNewArgc = 0;
    DWORD dwExportFormat = 0;
    BOOLEAN bIsValidKey = FALSE;

    BAIL_ON_INVALID_POINTER(argv);

    if (argc < 2)
    {
        dwError = LWREG_ERROR_INVALID_CONTEXT;
        goto error;
    }
    pszCommand = argv[1];

    dwError = RegShellCmdStringToEnum(pszCommand, &cmd);
    switch (cmd)
    {
        /*
         * Commands with no arguments
         */
        case REGSHELL_CMD_HELP:
        case REGSHELL_CMD_PWD:
            if (argc > 2)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            else
            {
                dwError = RegShellCmdParseCommand(cmd, &pCmdItem);
            }
            break;

        case REGSHELL_CMD_EXPORT:
            if (argc > 2)
            {
                if (!strcmp("--legacy", argv[dwArgc]))
                {
                    dwExportFormat = REGSHELL_EXPORT_LEGACY;
                    dwArgc++;
                }
                else if (!strcmp("--values", argv[dwArgc]))
                {
                    dwExportFormat = REGSHELL_EXPORT_VALUES;
                    dwArgc++;
                }
            }

            dwError = RegShellPszIsValidKey(
                          pParseState,
                          argv[dwArgc],
                          &bIsValidKey);
            if (dwError == 0 && bIsValidKey && dwArgc < argc)
            {
                pszSubKeyName = argv[dwArgc++];
            }

            dwError = RegShellCmdParseKeyName(
                          pParseState,
                          cmd,
                          pszSubKeyName,
                          &pCmdItem);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegAllocateMemory(
                          sizeof(PSTR) * argc,
                          (PVOID*)&pCmdItem->args);
            BAIL_ON_REG_ERROR(dwError);

            if (dwExportFormat == REGSHELL_EXPORT_LEGACY)
            {
                dwError = RegCStringDuplicate(
                              (LW_PVOID) &pCmdItem->args[dwNewArgc++],
                              "--legacy");
                BAIL_ON_REG_ERROR(dwError);
            }
            else if (dwExportFormat == REGSHELL_EXPORT_VALUES)
            {
                dwError = RegCStringDuplicate(
                              (LW_PVOID) &pCmdItem->args[dwNewArgc++],
                              "--values");
                BAIL_ON_REG_ERROR(dwError);
            }

            if (dwArgc < argc)
            {
                dwError = RegCStringDuplicate(
                              (LW_PVOID) &pCmdItem->args[dwNewArgc++],
                              argv[dwArgc++]);
                BAIL_ON_REG_ERROR(dwError);
            }
            else
            {
                dwError = RegCStringDuplicate(
                              (LW_PVOID) &pCmdItem->args[dwNewArgc++],
                              "-");
                BAIL_ON_REG_ERROR(dwError);
            }
            pCmdItem->argsCount = dwNewArgc;
            break;

        case REGSHELL_CMD_LIST_KEYS:
        case REGSHELL_CMD_LIST:
        case REGSHELL_CMD_DIRECTORY:
        case REGSHELL_CMD_LIST_VALUES:
        case REGSHELL_CMD_QUIT:
            if (argc > 2)
            {
                dwError = RegShellCmdParseKeyName(
                              pParseState,
                              cmd,
                              argv[2],
                              &pCmdItem);
            }
            else
            {
                dwError = RegShellCmdParseCommand(cmd, &pCmdItem);
            }
            break;

        case REGSHELL_CMD_IMPORT:
        case REGSHELL_CMD_UPGRADE:
        case REGSHELL_CMD_CLEANUP:
        case REGSHELL_CMD_SET_HIVE:
            if (argc != 3)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseCommand(cmd, &pCmdItem);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegAllocateMemory(sizeof(PSTR) * 2, (PVOID*)&pCmdItem->args);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegCStringDuplicate((LW_PVOID) &pCmdItem->args[0], argv[2]);
            BAIL_ON_REG_ERROR(dwError);
            pCmdItem->argsCount = 1;
            break;

        /*
         * Commands that take a KeyName argument
         *   command [HKLM_LIKEWISE/...]
         */
        case REGSHELL_CMD_ADD_KEY:
        case REGSHELL_CMD_CHDIR:
        case REGSHELL_CMD_DELETE_KEY:
        case REGSHELL_CMD_DELETE_TREE:
            if (argc != 3)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseKeyName(
                          pParseState,
                          cmd,
                          argv[2],
                          &pCmdItem);
            break;

        case REGSHELL_CMD_DELETE_VALUE:
            if (argc > 4)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseValueName(
                          pParseState,
                          cmd,
                          argc,
                          argv,
                          &pCmdItem);
            break;
       /*
        * Commands that take ValueName/REG_TYPE/Values_List
        *   add_value "ValueName" type "Value" ["Value2"] ["Value3"] [...]
        */
        case REGSHELL_CMD_ADD_VALUE:
            if (argc < 5)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
        case REGSHELL_CMD_SET_VALUE:
            if (argc < 4)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseValueName(
                          pParseState,
                          cmd,
                          argc,
                          argv,
                          &pCmdItem);
            break;

        default:
            dwError = LWREG_ERROR_INVALID_CONTEXT;
            break;
    }

    *parsedCmd = pCmdItem;
cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pCmdItem);
    goto cleanup;
}

DWORD
RegShellAllocKey(
    PREGSHELL_PARSE_STATE pParseState,
    PSTR pszKeyName,
    PSTR *pszNewKey)
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR pszRetKey = NULL;

    dwLen = strlen(pszKeyName) + 3;  // 3: [ ] and \0
    dwError = RegAllocateMemory(dwLen, (PVOID) &pszRetKey);
    BAIL_ON_REG_ERROR(dwError);

    pszRetKey[0] = '\0';
    if (!pParseState->bBracketPrefix)
    {
        strncat(pszRetKey, "[", dwLen--);
        strncat(pszRetKey, pszKeyName, dwLen);
        dwLen -= strlen(pszKeyName);
        strncat(pszRetKey, "]", dwLen--);
    }
    else
    {
        strncat(pszRetKey, pszKeyName, dwLen);
    }

cleanup:
    *pszNewKey = pszRetKey;
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pszRetKey);
    goto cleanup;
}


DWORD
RegShellCmdlineParseToArgv(
    PREGSHELL_PARSE_STATE pParseState,
    PDWORD pdwNewArgc,
    PSTR **pszNewArgv)
{
    DWORD dwError = 0;
    DWORD attrSize = 0;
    DWORD dwArgc = 1;
    DWORD dwAllocSize = 1;
    DWORD dwBinaryDataOffset = 0;
    DWORD dwLen = 0;
    DWORD dwValueLen = 0;
    DWORD dwKeyNameOffset = 0;
    BOOLEAN eof = FALSE;
    BOOLEAN stop = FALSE;
    PSTR pszAttr = NULL;
    PSTR pszPrevAttr = NULL;
    PSTR *pszArgv = NULL;
    PSTR *pszArgvRealloc = NULL;
    PSTR pszBinaryData = NULL;
    PSTR pszKeyName = NULL;
    REGLEX_TOKEN token = 0;
    REGSHELL_CMDLINE_STATE_E state = 0;
    REG_DATA_TYPE valueType = REG_NONE;
    REGSHELL_CMD_E cmdEnum = 0;
    PREGSHELL_CMD_ITEM pCmdItem = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState->ioHandle);
    BAIL_ON_INVALID_HANDLE(pParseState->lexHandle);
    BAIL_ON_INVALID_POINTER(pdwNewArgc);
    BAIL_ON_INVALID_POINTER(pszNewArgv);

    dwError = RegLexGetToken(pParseState->ioHandle,
                             pParseState->lexHandle,
                             &token,
                             &eof);
    BAIL_ON_REG_ERROR(dwError);
    stop = eof;
    do
    {
        switch (state)
        {
            case REGSHELL_CMDLINE_STATE_FIRST:
                if (token == REGLEX_PLAIN_TEXT)
                {
                    state = REGSHELL_CMDLINE_STATE_VERB;
                }
                else
                {
                    /* Syntax error */
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_VERB:
                RegLexGetAttribute(pParseState->lexHandle, &attrSize, &pszAttr);
                if (attrSize > 0)
                {
                    dwError = RegShellCmdStringToEnum(pszAttr, &cmdEnum);
                    if (dwError)
                    {
                        /* Syntax error */
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                        break;
                    }
                }

                if (cmdEnum == REGSHELL_CMD_LIST_KEYS ||
                    cmdEnum == REGSHELL_CMD_LIST ||
                    cmdEnum == REGSHELL_CMD_DIRECTORY)
                {

                    d_printf(("RegShellCmdlineParseToArgv: list_keys found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS;
                    dwArgc++;
                    dwAllocSize = dwArgc+1;
                }
                else if (cmdEnum == REGSHELL_CMD_LIST ||
                         cmdEnum == REGSHELL_CMD_DIRECTORY)
                {
                    d_printf(("RegShellCmdlineParseToArgv: list found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS;
                    dwArgc++;
                    dwAllocSize = dwArgc+1;
                }
                else if (cmdEnum == REGSHELL_CMD_LIST_VALUES)
                {
                    d_printf(("RegShellCmdlineParseToArgv: list_values found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS;
                    dwArgc++;
                    dwAllocSize = dwArgc+1;
                }
                else if (cmdEnum == REGSHELL_CMD_CHDIR)
                {
                    d_printf(("RegShellCmdlineParseToArgv: cd found\n"));
                    state = REGSHELL_CMDLINE_STATE_CD;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 1;
                }
                else if (cmdEnum == REGSHELL_CMD_DELETE_KEY)
                {
                    d_printf(("RegShellCmdlineParseToArgv: delete_key found\n"));
                    state = REGSHELL_CMDLINE_STATE_CD;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 1;
                }
                else if (cmdEnum == REGSHELL_CMD_DELETE_TREE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: delete_tree found\n"));
                    state = REGSHELL_CMDLINE_STATE_CD;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 1;
                }
                else if (cmdEnum == REGSHELL_CMD_HELP)
                {
                    d_printf(("RegShellCmdlineParseToArgv: help found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    dwArgc += 1;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_PWD)
                {
                    d_printf(("RegShellCmdlineParseToArgv: pwd found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    dwArgc += 1;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_QUIT)
                {
                    d_printf(("RegShellCmdlineParseToArgv: exit found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    dwArgc += 1;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_ADD_KEY)
                {
                    d_printf(("RegShellCmdlineParseToArgv: add_key found\n"));
                    state = REGSHELL_CMDLINE_STATE_ADDKEY;
                    dwArgc += 2;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_ADD_VALUE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: add_value found\n"));
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                    dwArgc += 1;
                    /*
                     * Realloc pszArgv as needed for additional args,
                     * as there is no way to know how many arguments
                     * will follow for REG_MULTI_SZ.
                     */
                    dwAllocSize = dwArgc + 64;
                }
                else if (cmdEnum == REGSHELL_CMD_DELETE_VALUE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: delete_value found\n"));
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 3;
                }
                else if (cmdEnum == REGSHELL_CMD_SET_VALUE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: set_value found\n"));
                    dwArgc += 1;
                    /*
                     * Realloc pszArgv as needed for additional args,
                     * as there is no way to know how many arguments
                     * will follow for REG_MULTI_SZ.
                     */
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                    dwAllocSize = dwArgc + 64;
                }
                else if (cmdEnum == REGSHELL_CMD_SET_HIVE)
                {
                    dwAllocSize = 4;
                    dwArgc = 2;
                    state = REGSHELL_CMDLINE_STATE_IMPORT;
                }
                else if (cmdEnum == REGSHELL_CMD_IMPORT ||
                         cmdEnum == REGSHELL_CMD_UPGRADE ||
                         cmdEnum == REGSHELL_CMD_CLEANUP)
                {
                    dwAllocSize = 4;
                    dwArgc = 2;
                    state = REGSHELL_CMDLINE_STATE_IMPORT;
                }
                else if (cmdEnum == REGSHELL_CMD_EXPORT)
                {
                    dwAllocSize = 4;
                    dwArgc = 2;
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                }
                else
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    break;
                }

                dwError = RegAllocateMemory(sizeof(*pszArgv) * dwAllocSize, (PVOID*)&pszArgv);
                BAIL_ON_REG_ERROR(dwError);

                dwError = RegCStringDuplicate(&pszArgv[0], "regshell");
                BAIL_ON_REG_ERROR(dwError);
                dwError = RegCStringDuplicate(&pszArgv[1], pszAttr);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMDLINE_STATE_LIST_KEYS:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                /*
                 * This parameter is optional, but there must have been a
                 * default key specified with a previous 'cd' command.
                 */
                if (!eof)
                {
                    if (token == REGLEX_REG_KEY ||
                        token == REGLEX_PLAIN_TEXT)
                    {
                        dwArgc++;
                        state = REGSHELL_CMDLINE_STATE_LIST_KEYS_REGKEY;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                }
                else
                {
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    stop = eof;
                }
                break;

            case REGSHELL_CMDLINE_STATE_LIST_KEYS_REGKEY:
                RegLexGetAttribute(pParseState->lexHandle,
                                   &attrSize,
                                   &pszAttr);
                RegShellAllocKey(pParseState, pszAttr, &pszArgv[dwArgc-1]);
                BAIL_ON_REG_ERROR(dwError);
                state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                break;

            case REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                stop = eof;
                if (!eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_CD:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                if (eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    stop = eof;
                }
                else
                {
                    if (token == REGLEX_REG_KEY ||
                        token == REGLEX_KEY_PREFIX ||
                        token == REGLEX_PLAIN_TEXT)
                    {
                        state = REGSHELL_CMDLINE_STATE_CD_REGKEY;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                }
                break;


            case REGSHELL_CMDLINE_STATE_CD_REGKEY:
                dwError = RegShellAllocKey(pParseState, pszAttr, &pszArgv[2]);
                BAIL_ON_REG_ERROR(dwError);
                dwArgc += 1;

                state = REGSHELL_CMDLINE_STATE_CD_STOP;
                break;

            case REGSHELL_CMDLINE_STATE_CD_STOP:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                stop = eof;
                if (!eof && !pParseState->bBracketPrefix)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_ADDKEY:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                if (eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    stop = eof;
                }
                else
                {
                    if (token == REGLEX_REG_KEY ||
                        token == REGLEX_PLAIN_TEXT)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDKEY_REGKEY;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                }
                break;

            case REGSHELL_CMDLINE_STATE_ADDKEY_REGKEY:
                dwError = RegShellAllocKey(pParseState, pszAttr, &pszArgv[2]);
                BAIL_ON_REG_ERROR(dwError);
                state = REGSHELL_CMDLINE_STATE_ADDKEY_STOP;
                break;

            case REGSHELL_CMDLINE_STATE_ADDKEY_STOP:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                stop = eof;
                if (!eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_ADDVALUE:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                if (eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                else if (cmdEnum == REGSHELL_CMD_EXPORT &&
                         token == REGLEX_PLAIN_TEXT)
                {
                    dwError = RegCStringDuplicate(&pszPrevAttr, pszAttr);
                    BAIL_ON_REG_ERROR(dwError);

                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    if (eof)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME;
                    }
                    else
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_KEYNAME;
                    }
                    RegLexUnGetToken(pParseState->lexHandle);
                }
                else if (token == REGLEX_REG_KEY)
                {
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE_KEYNAME;
                }
                else if (token == REGLEX_REG_SZ ||
                         token == REGLEX_PLAIN_TEXT ||
                         token == REGLEX_KEY_NAME_DEFAULT)
                {
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME;
                }
                else if (token == REGLEX_DASH)
                {
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (token != REGLEX_DASH)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME;
                        RegLexUnGetToken(pParseState->lexHandle);
                    }
                    else if (eof)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME;
                    }
                    else
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_OPTION;
                    }
                }
                else
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_ADDVALUE_OPTION:
                /* Handle export --legacy option */
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                if (dwError || eof || token != REGLEX_PLAIN_TEXT)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    BAIL_ON_REG_ERROR(dwError);
                }

                RegLexGetAttribute(pParseState->lexHandle,
                                   &attrSize,
                                   &pszAttr);
                if (cmdEnum == REGSHELL_CMD_EXPORT && 
                    !strcmp("legacy", pszAttr))
                {
                    dwError = RegCStringDuplicate(
                                  &pszArgv[dwArgc++],
                                  "--legacy");
                    BAIL_ON_REG_ERROR(dwError);
                }
                else if (cmdEnum == REGSHELL_CMD_EXPORT && 
                    !strcmp("values", pszAttr))
                {
                    dwError = RegCStringDuplicate(
                                  &pszArgv[dwArgc++],
                                  "--values");
                    BAIL_ON_REG_ERROR(dwError);
                }
                else
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    BAIL_ON_REG_ERROR(dwError);
                }
                state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                break;
                         
                case REGSHELL_CMDLINE_STATE_ADDVALUE_KEYNAME:
                    if (pszPrevAttr)
                    {
                        pszAttr = pszPrevAttr;
                    }
                    else
                    {
                        RegLexGetAttribute(pParseState->lexHandle,
                                           &attrSize,
                                           &pszAttr);
                    }
                    dwError = RegShellCmdParseKeyName(
                                  pParseState,
                                  cmdEnum,
                                  pszAttr,
                                  &pCmdItem);
                    LWREG_SAFE_FREE_MEMORY(pszPrevAttr);
                    BAIL_ON_REG_ERROR(dwError);
                    if (pCmdItem->keyName)
                    {
                        dwError = RegShellAllocKey(pParseState, pCmdItem->keyName, &pszArgv[dwArgc++]);
                        BAIL_ON_REG_ERROR(dwError);
                    }

                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else if (token == REGLEX_REG_SZ ||
                             token == REGLEX_PLAIN_TEXT ||
                             token == REGLEX_KEY_NAME_DEFAULT)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME;
                    }
                    else if (token == REGLEX_DASH)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE;
                        RegLexUnGetToken(pParseState->lexHandle);
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }

                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME:
                    RegLexGetAttribute(pParseState->lexHandle,
                                       &attrSize,
                                       &pszAttr);
                    dwError = RegCStringDuplicate(&pszArgv[dwArgc++], pszAttr);
                    BAIL_ON_REG_ERROR(dwError);

                    if (cmdEnum == REGSHELL_CMD_SET_VALUE)
                    {
                        if (pCmdItem)
                        {
                            dwError = RegCStringDuplicate(&pszKeyName, pCmdItem->keyName ? pCmdItem->keyName : "");
                            BAIL_ON_REG_ERROR(dwError);
                            if (pszKeyName[0] == '\\')
                            {
                                dwKeyNameOffset++;
                            }
                        }
                        dwError = RegShellUtilGetValue(
                                      NULL,
                                      RegShellGetRootKey(pParseState),
                                      RegShellGetDefaultKey(pParseState),
                                      &pszKeyName[dwKeyNameOffset],
                                      pszAttr,
                                      &valueType,
                                      NULL,
                                      &dwValueLen);
                        BAIL_ON_REG_ERROR(dwError);

                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE;
                        break;
                    }

                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);

                    /* add_value type is next */
                    if (cmdEnum == REGSHELL_CMD_DELETE_VALUE ||
                        cmdEnum == REGSHELL_CMD_EXPORT)
                    {
                        stop = TRUE;
                    }
                    else if (eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else if (token == REGLEX_REG_SZ ||
                             token == REGLEX_PLAIN_TEXT ||
                             token == REGLEX_REG_MULTI_SZ ||
                             token == REGLEX_REG_BINARY ||
                             token == REGLEX_REG_DWORD)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_TYPE;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_TYPE:
                    RegLexGetAttribute(pParseState->lexHandle,
                                       &attrSize,
                                       &pszAttr);
                    dwError = RegShellParseStringType(
                                  pszAttr,
                                  &valueType,
                                  NULL);
                    if (valueType == REG_NONE)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                        BAIL_ON_REG_ERROR(dwError);
                    }
                    dwError = RegCStringDuplicate(&pszArgv[dwArgc++], pszAttr);
                    BAIL_ON_REG_ERROR(dwError);

                    state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE;
                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE:
                    /* Get current buffer before reading the next token
                     * to determine if EOF has been found. Do this before
                     * the next call to RegLexGetToken(), as it consumes
                     * the first hex pair if found.
                     */
                    if (valueType == REG_BINARY || valueType == REG_DWORD)
                    {
                        dwError = RegIOBufferGetData(
                                      pParseState->ioHandle,
                                      &pszBinaryData,
                                      NULL,
                                      &dwBinaryDataOffset);
                        BAIL_ON_REG_ERROR(dwError);
                    }
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (eof)
                    {
                        if ((cmdEnum==REGSHELL_CMD_SET_VALUE && dwArgc<=3) ||
                            (cmdEnum== REGSHELL_CMD_ADD_VALUE && dwArgc<=4))
                        {
                            dwError = LWREG_ERROR_INVALID_CONTEXT;
                        }
                        else
                        {
                            state = REGSHELL_CMDLINE_STATE_ADDVALUE_STOP;
                        }
                    }
                    else if (valueType == REG_BINARY || valueType == REG_DWORD)
                    {
                        dwError = RegCStringDuplicate(
                                      &pszArgv[dwArgc],
                                      &pszBinaryData[dwBinaryDataOffset]);
                        BAIL_ON_REG_ERROR(dwError);
                        dwLen = strlen(pszArgv[dwArgc]);
                        if (dwLen>1 && pszArgv[dwArgc][dwLen-1] == '\n')
                        {
                            pszArgv[dwArgc][dwLen-1] = '\0';
                        }
                        dwArgc++;
                        /*
                         * Force a stop now, as the parser will return
                         * subsequent hex pairs, which is not what we want.
                         * Call RegLexResetToken() to flush the remainder
                         * of the command line from the parser.
                         */
                        stop = TRUE;
                        RegLexResetToken(pParseState->lexHandle);
                    }
                    else if (token == REGLEX_REG_SZ ||
                             token == REGLEX_PLAIN_TEXT ||
                             token == REGLEX_DASH)
                    {
                        RegLexGetAttribute(pParseState->lexHandle,
                                           &attrSize,
                                           &pszAttr);
                        dwError = RegCStringDuplicate(&pszArgv[dwArgc++], pszAttr);
                        BAIL_ON_REG_ERROR(dwError);
                        if (dwArgc >= dwAllocSize)
                        {
                            dwAllocSize *= 2;
                            dwError = RegReallocMemory(
                                          pszArgv,
                                          (LW_PVOID) &pszArgvRealloc,
                                          dwAllocSize * sizeof(*pszArgv));
                            BAIL_ON_REG_ERROR(dwError);
                            pszArgv = pszArgvRealloc;
                            pszArgvRealloc = NULL;
                        }
                    }
                    else
                    {
                            dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_STOP:
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (!eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else
                    {
                        stop = TRUE;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_IMPORT:
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else
                    {
                        state = REGSHELL_CMDLINE_STATE_IMPORT_FILE;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_IMPORT_FILE:
                    RegLexGetAttribute(pParseState->lexHandle,
                                       &attrSize,
                                       &pszAttr);
                    dwError = RegCStringDuplicate((LW_PVOID) &pszArgv[dwArgc++],
                                                  pszAttr);
                    BAIL_ON_REG_ERROR(dwError);

                    state = REGSHELL_CMDLINE_STATE_IMPORT_STOP;
                    break;

                case REGSHELL_CMDLINE_STATE_IMPORT_STOP:
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (!eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else
                    {
                        stop = eof;
                    }
                    break;

            default:
                stop = TRUE;
                break;
        }
    }
    while (!stop && dwError == ERROR_SUCCESS);

    *pdwNewArgc = dwArgc;
    *pszNewArgv = pszArgv;
#ifdef _LW_DEBUG
    if (pszArgv)
    {
        int i;
        for (i=0; i<dwArgc; i++)
        {
            printf("RegShellCmdlineParseToArgv: argv[%d] = '%s'\n",
                   i, pszArgv[i]);
        }
    }
#endif

cleanup:
    RegShellCmdParseFree(pCmdItem);
    LWREG_SAFE_FREE_STRING(pszKeyName);
    if (dwError)
    {
        RegLexResetToken(pParseState->lexHandle);
    }
    return dwError;

error:
    RegShellCmdlineParseFree(dwArgc, pszArgv);
    goto cleanup;
}


DWORD
RegShellCmdlineParseFree(
    DWORD dwArgc,
    PSTR *pszArgv)
{
    DWORD dwError = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(pszArgv);

    for (i=0; i<dwArgc && pszArgv[i]; i++)
    {
        LWREG_SAFE_FREE_STRING(pszArgv[i]);
    }
    LWREG_SAFE_FREE_MEMORY(pszArgv);

cleanup:
    return dwError;

error:
    goto cleanup;
}


LW_VOID
RegShellUsage(
    PSTR progname)
{
    printf("usage: %s [--file | -f] command_file.txt\n"
        "       add_key [[KeyName]]\n"
        "       list_keys [[keyName]]\n"
        "       delete_key [KeyName]\n"
        "       delete_tree [KeyName]\n"
        "       cd [KeyName]\n"
        "       pwd\n"
        "       add_value [[KeyName]] \"ValueName\" Type \"Value\" [\"Value2\"] [...]\n"
        "       set_value [[KeyName]] \"ValueName\" \"Value\" [\"Value2\"] [...]\n"
        "       list_values [[keyName]]\n"
        "       delete_value [[KeyName]] \"ValueName\"\n"
        "       set_hive HIVE_NAME\n"
        "       import file.reg | -\n"
        "       export [--legacy | --values] [[keyName]] [file.reg | -]\n"
        "       upgrade file.reg | -\n"
        "       cleanup file.reg | -\n"
        "       exit | quit | ^D\n"
        "       history\n"
        "\n"
        "         Type: REG_SZ | REG_DWORD | REG_BINARY | REG_MULTI_SZ\n"
        "               REG_DWORD and REG_BINARY values are hexadecimal\n"
        "         Note: cd and pwd only function in interactive mode\n"
        "         Note: HKEY_THIS_MACHINE is the only supported hive\n"
        ,progname);
}
