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
 *        regshell.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry Shell header file
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#ifndef _REGSHELL_H
#define _REGSHELL_H

#include <config.h>
#include <regsystem.h>

#include <reg/lwreg.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw/base.h>
#include <lw/swab.h>
#include <lw/ntstatus.h>
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>

#include <regutils.h>
#include <regdef.h>
#include <regclient.h>
#include <regparse_r.h>
#include "../utils/includes.h"

#include "rsutils.h"
#include "../parse/regio.h"
#include "../parse/reglex.h"
#include "../parse/regiconv.h"
#include "../parse/regparse.h"

#ifdef _LW_DEBUG
#define d_printf(x) printf(x)
#else
#define d_printf(x)
#endif


typedef enum _REGSHELL_EXPORT_TYPE_E
{
   REGSHELL_EXPORT_DEFAULT = 0,
   REGSHELL_EXPORT_LEGACY,
   REGSHELL_EXPORT_VALUES
} REGSHELL_EXPORT_TYPE_E, *PREGSHELL_EXPORT_TYPE_E;

typedef enum _REGSHELL_CMD_MODE_E
{
    REGSHELL_CMD_MODE_CMDLINE = 1,
    REGSHELL_CMD_MODE_INTERACTIVE,
    REGSHELL_CMD_MODE_CMDFILE,
} REGSHELL_CMD_MODE_TYPE_E, *PREGSHELL_CMD_MODE_TYPE_E;

typedef enum _REGSHELL_CMD_E
{
    REGSHELL_CMD_NONE = 0,
    REGSHELL_CMD_LIST_KEYS,
    REGSHELL_CMD_DIRECTORY,
    REGSHELL_CMD_LIST,
    REGSHELL_CMD_LIST_VALUES,
    REGSHELL_CMD_ADD_KEY,
    REGSHELL_CMD_ADD_VALUE,
    REGSHELL_CMD_CHDIR,
    REGSHELL_CMD_DELETE_KEY,
    REGSHELL_CMD_DELETE_VALUE,
    REGSHELL_CMD_DELETE_TREE,
    REGSHELL_CMD_SET_VALUE,
    REGSHELL_CMD_SET_HIVE,
    REGSHELL_CMD_PWD,
    REGSHELL_CMD_HELP,
    REGSHELL_CMD_QUIT,
    REGSHELL_CMD_IMPORT,
    REGSHELL_CMD_EXPORT,
    REGSHELL_CMD_UPGRADE,
    REGSHELL_CMD_CLEANUP,
} REGSHELL_CMD_E, *PREGSHELL_CMD_E;


typedef struct _REGSHELL_CMD_ID
{
    PCHAR pszCommand;
    REGSHELL_CMD_E eCommand;
} REGSHELL_CMD_ID, *PREGSHELL_CMD_ID;


typedef struct _REGSHELL_CMD_ITEM
{
    REGSHELL_CMD_E command;
    PSTR keyName;
    PSTR valueName;
    REG_DATA_TYPE type;
    DWORD backendType;
    PCHAR *args;
    DWORD argsCount;
    PUCHAR binaryValue;
    DWORD binaryValueLen;
} REGSHELL_CMD_ITEM, *PREGSHELL_CMD_ITEM;


typedef enum _REGSHELL_TAB_COMPLETION_E
{
    REGSHELL_TAB_BRACKET_PREFIX = 0,
    REGSHELL_TAB_CMD,
    REGSHELL_TAB_ROOT_KEY,
    REGSHELL_TAB_SUBKEY,
    REGSHELL_TAB_DONE,
    REGSHELL_TAB_ERROR,
} REGSHELL_TAB_COMPLETION_E;


typedef struct _REGSHELL_TAB_COMPLETION_STATE
{
    REGSHELL_TAB_COMPLETION_E eTabState;
    PSTR pszCommand;
    PSTR pszRootKey;
    PSTR pszSubKey;
    PSTR pszInputLine;
    PSTR pszPrevLine;
    PSTR pszCurLine;
} REGSHELL_TAB_COMPLETION_STATE;


typedef struct _REGSHELL_PARSE_STATE
{
    HANDLE ioHandle;
    PREGLEX_ITEM lexHandle;
    HANDLE hReg;
    PSTR pszDefaultRootKeyName;
    PSTR pszDefaultKey;

    // Same as pszDefaultKey, but manipulated during tab completion
    PSTR pszDefaultKeyCompletion;

    // Overrides pszDefaultKey when root key path is specified
    PSTR pszFullRootKeyName;
    PSTR pszFullKeyPath; 
    REGSHELL_TAB_COMPLETION_STATE tabState;
    BOOLEAN bBracketPrefix;
    DWORD dwTabPressCount;
    REGSHELL_CMD_MODE_TYPE_E eShellMode;
} REGSHELL_PARSE_STATE, *PREGSHELL_PARSE_STATE;


typedef struct _REGSHELL_REG_TYPE_ENTRY
{
    PCHAR pszType;
    REG_DATA_TYPE type;
    DWORD backendType;
} REGSHELL_REG_TYPE_ENTRY, *PREGSHELL_REG_TYPE_ENTRY;

typedef struct __REG_EXPORT_STATE
{
    FILE* fp;
    PBYTE pRootKeySecDescRel[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE];
    ULONG ulRootKeySecDescLen;
    DWORD dwExportFormat; 
}REG_EXPORT_STATE, *PREG_EXPORT_STATE;


PSTR
RegShellGetRootKey(
    PREGSHELL_PARSE_STATE pParseState);

PSTR
RegShellGetDefaultKey(
    PREGSHELL_PARSE_STATE pParseState);

DWORD
RegShellCmdEnumToString(
    REGSHELL_CMD_E cmdEnum,
    PCHAR cmdString,
    DWORD dwCmdStringLen);


DWORD
RegShellCmdStringToEnum(
    PCHAR cmdString,
    PREGSHELL_CMD_E pCmdEnum);

DWORD
RegShellCmdParse(
    PREGSHELL_PARSE_STATE pParseState,
    DWORD argc,
    PCHAR *argv,
    PREGSHELL_CMD_ITEM *parsedCmd);

DWORD
RegShellCmdlineParseToArgv(
    PREGSHELL_PARSE_STATE pParseState,
    PDWORD pdwNewArgc,
    PSTR **pszNewArgv);

DWORD
RegShellCmdlineParseFree(
    DWORD dwArgc,
    PSTR *pszArgv);


DWORD
RegShellDumpCmdItem(
    PREGSHELL_CMD_ITEM rsItem);

LW_VOID
RegShellUsage(
    PSTR progname);


DWORD
RegShellAllocKey(
    PREGSHELL_PARSE_STATE pParseState,
    PSTR pszKeyName,
    PSTR *pszNewKey);

DWORD
RegShellParseStringType(
    PCHAR pszType,
    PREG_DATA_TYPE pType,
    PDWORD pBackendType);

DWORD
RegShellOpenHandle(
    HANDLE hReg,
    PSTR pszDefaultKey,
    PSTR pszKeyName,
    PHKEY pNewHandle);

DWORD
RegShellCmdParseFree(
    PREGSHELL_CMD_ITEM pCmdItem);

DWORD
RegShellUtilExport(
    HANDLE hReg,
    PREG_EXPORT_STATE pExportState,
    HKEY hKey,
    PCSTR pszKeyName,
    DWORD dwNumSubKeys,
    DWORD dwMaxSubKeyLen
    );

DWORD
RegShellUtilImportCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE hUserCtx
    );

DWORD
RegShellUtilImportDebugCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext
    );

#endif
