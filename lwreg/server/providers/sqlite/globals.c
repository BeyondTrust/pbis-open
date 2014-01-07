/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 *        Global Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

REG_DB_HANDLE ghCacheConnection = {0};

const DWORD dwDefaultCacheSize = 1000;

REGPROV_PROVIDER_FUNCTION_TABLE gRegSqliteProviderAPITable =
{
        &SqliteCreateKeyEx,
        &SqliteCloseKey,
        &SqliteDeleteKey,
        &SqliteDeleteKeyValue,
        &SqliteDeleteValue,
        &SqliteDeleteTree,
        &SqliteEnumKeyEx,
        &SqliteEnumValue,
        &SqliteGetValue,
        &SqliteOpenKeyEx,
        &SqliteQueryInfoKey,
        &SqliteQueryMultipleValues,
        &SqliteSetValueEx,
        &SqliteSetKeySecurity,
        &SqliteGetKeySecurity,
        &SqliteSetValueAttributes,
        &SqliteGetValueAttributes,
        &SqliteDeleteValueAttributes
};

REG_SRV_SQLITE_KEYLOOKUP gActiveKeyList =
    {
            .mutex    = PTHREAD_MUTEX_INITIALIZER,
            .pKeyList = NULL
    };

REG_SRV_SQLITE_KEYLOOKUP gRegDbKeyList =
    {
            .mutex    = PTHREAD_MUTEX_INITIALIZER,
            .pKeyList = NULL
    };

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
