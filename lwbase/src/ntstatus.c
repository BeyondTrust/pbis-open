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
 *        ntstatus.c
 *
 * Abstract:
 *
 *        NT status codes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

typedef struct _TABLE_ENTRY
{
    NTSTATUS code;
    int unixErrno;
    PCSTR pszSymbolicName;
    PCSTR pszDescription;
} const TABLE_ENTRY, *PTABLE_ENTRY;

static
PTABLE_ENTRY
LwNtLookupCode(
    IN NTSTATUS NtStatus
    );

#define NTSTATUS_CODE(code, err, desc) { code, err, #code, desc },
static
TABLE_ENTRY LwNtStatusCodeTable[] =
{
#include "ntstatus-table.h"
    {-1, -1, NULL, NULL}
};
#undef NTSTATUS_CODE

LW_PCSTR
LwNtStatusToName(
    LW_IN LW_NTSTATUS NtStatus
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(NtStatus);

    if (pEntry && pEntry->pszSymbolicName)
    {
        return pEntry->pszSymbolicName;
    }
    else
    {
        return "UNKNOWN";
    }
}

LW_PCSTR
LwNtStatusToDescription(
    IN NTSTATUS NtStatus
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(NtStatus);

    if (pEntry)
    {
        if (pEntry->pszDescription)
        {
            return pEntry->pszDescription;
        }
        else
        {
            return "No description available";
        }
    }
    else
    {
        return "Unknown error";
    }
}

int
LwNtStatusToErrno(
    IN NTSTATUS NtStatus
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(NtStatus);

    if (pEntry)
    {
        return pEntry->unixErrno;
    }
    else if (_LW_IS_ERRNO_NTSTATUS(NtStatus))
    {
        return LW_NTSTATUS_GET_CODE(NtStatus);
    }
    else
    {
        return -1;
    }
}

static
PTABLE_ENTRY
LwNtLookupCode(
    IN NTSTATUS NtStatus
    )
{
    ULONG index;

    for (index = 0; index < sizeof(LwNtStatusCodeTable) / sizeof(*LwNtStatusCodeTable); index++)
    {
        if (LwNtStatusCodeTable[index].code == NtStatus)
        {
            return &LwNtStatusCodeTable[index];
        }
    }

    return NULL;
}

