/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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

