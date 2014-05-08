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

static PREG_DB_CONNECTION gMemRegRoot;

inline
PREG_DB_CONNECTION 
MemRegRoot(
    VOID)
{
    return gMemRegRoot;
}

inline VOID
MemRegRootInit(
    PREG_DB_CONNECTION hMemRegRoot
    )
{
    gMemRegRoot = hMemRegRoot;
}


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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
