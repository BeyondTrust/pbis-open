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
 *        globals.c
 *
 * Abstract:
 *        Global Variables for registry memory provider backend
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */
#include "includes.h"
#include "memstore_p.h"

REG_DB_HANDLE ghCacheConnection = {0};

PREGMEM_NODE ghMemRegRoot;

const DWORD dwDefaultCacheSize = 1000;

REGPROV_PROVIDER_FUNCTION_TABLE gRegMemProviderAPITable =
{
        &MemCreateKeyEx,
        &MemCloseKey,
        &MemDeleteKey,
        &MemDeleteKeyValue,
        &MemDeleteValue,
        &MemDeleteTree,
        &MemEnumKeyEx,
        &MemEnumValue,
        &MemGetValue,
        &MemOpenKeyEx,
        &MemQueryInfoKey,
        &MemQueryMultipleValues,
        &MemSetValueEx,
        &MemSetKeySecurity,
        &MemGetKeySecurity,
        &MemSetValueAttributes,
        &MemGetValueAttributes,
        &MemDeleteValueAttributes
};

pthread_mutex_t gMemRegDbMutex = PTHREAD_MUTEX_INITIALIZER;
BOOLEAN gbInLockDbMutex;

pthread_mutex_t gExportMutex = PTHREAD_MUTEX_INITIALIZER;
BOOLEAN gbInLockExportMutex;
pthread_cond_t gExportCond = PTHREAD_COND_INITIALIZER;

BOOLEAN gbValueChanged;
PMEMDB_FILE_EXPORT_CTX gExportCtx;

pthread_mutex_t gExportMutexStop = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gExportCondStop = PTHREAD_COND_INITIALIZER;


#if 0
REG_SRV_MEMORY_KEYLOOKUP gActiveKeyList =
    {
            .mutex    = PTHREAD_MUTEX_INITIALIZER,
            .pKeyList = NULL
    };

REG_SRV_MEMORY_KEYLOOKUP gRegDbKeyList =
    {
            .mutex    = PTHREAD_MUTEX_INITIALIZER,
            .pKeyList = NULL
    };
#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
