/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Module Name:
 *
 *        errno.c
 *
 * Abstract:
 *
 *        UNIX errno codes
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

typedef struct _TABLE_ENTRY
{
    int code;
    NTSTATUS ntStatus;
    PCSTR pszSymbolicName;
} const TABLE_ENTRY, *PTABLE_ENTRY;

static
PTABLE_ENTRY
LwUnixErrnoLookupCode(
    IN int Uerror
    );

#define ERRNO_CODE(code, nt) { code, nt, #code },
static
TABLE_ENTRY LwErrnoCodeTable[] =
{
#include "errno-table.h"
    {-1, -1, NULL}
};
#undef ERRNO_CODE

LW_PCSTR
LwErrnoToName(
    int Uerror
    )
{
    PTABLE_ENTRY pEntry = LwUnixErrnoLookupCode(Uerror);

    if (pEntry)
    {
        return pEntry->pszSymbolicName;
    }
    else
    {
        return "UNKNOWN";
    }
}

NTSTATUS
LwErrnoToNtStatus(
    IN int Uerror
    )
{
    PTABLE_ENTRY pEntry = LwUnixErrnoLookupCode(Uerror);

    if (pEntry && (pEntry->ntStatus != (NTSTATUS) -1))
    {
        return pEntry->ntStatus;
    }
    else
    {
        return _LW_MAKE_ERRNO_NTSTATUS(Uerror);
    }
}

static
PTABLE_ENTRY
LwUnixErrnoLookupCode(
    IN int Uerror
    )
{
    ULONG index;

    for (index = 0; index < sizeof(LwErrnoCodeTable) / sizeof(*LwErrnoCodeTable); index++)
    {
        if (LwErrnoCodeTable[index].code == Uerror)
        {
            return &LwErrnoCodeTable[index];
        }
    }

    return NULL;
}

