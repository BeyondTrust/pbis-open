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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        sddlsidstring.c
 *
 * Abstract:
 *
 *        Sid-string and Rid mapping
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#include "includes.h"
#include "security-sddl-internal.h"

typedef struct _TABLE_ENTRY
{
    ULONG rid;
    PCSTR pszAliasSidString;
    PCSTR pszSidString;
} const TABLE_ENTRY, *PTABLE_ENTRY;

static
PTABLE_ENTRY
RtlpSddlLookupRid(
    IN ULONG rid
    );

static
PTABLE_ENTRY
RtlpSddlLookupAliasSidString(
    IN PCSTR pszAliasSidString
    );

#define SIDRID_MAPPING(rid, aliasstring, sidstring) { rid, aliasstring, sidstring },
static
TABLE_ENTRY LwSddlSidStringTable[] =
{
#include "sddlsid-table.h"
    {-1,  "", ""}
};
#undef SIDRID_MAPPING

LW_PCSTR
RtlpRidToAliasSidString(
    LW_IN ULONG rid
    )
{
    PTABLE_ENTRY pEntry = RtlpSddlLookupRid(rid);

    if (pEntry && pEntry->pszAliasSidString)
    {
        return pEntry->pszAliasSidString;
    }
    else
    {
        return NULL;
    }
}

ULONG
RtlpAliasSidStringToRid(
    LW_IN PCSTR pszAliasSidString
    )
{
    PTABLE_ENTRY pEntry = RtlpSddlLookupAliasSidString(pszAliasSidString);

    if (pEntry)
    {
        return pEntry->rid;
    }
    else
    {
        return -1;
    }
}

LW_PCSTR
RtlpAliasSidStringToSidString(
    LW_IN PCSTR pszAliasSidString
    )
{
    PTABLE_ENTRY pEntry = RtlpSddlLookupAliasSidString(pszAliasSidString);

    if (pEntry)
    {
        return pEntry->pszSidString;
    }
    else
    {
        return NULL;
    }
}

static
PTABLE_ENTRY
RtlpSddlLookupRid(
    IN ULONG rid
    )
{
    ULONG index;

    for (index = 0; index < sizeof(LwSddlSidStringTable) / sizeof(*LwSddlSidStringTable); index++)
    {
        if (LwSddlSidStringTable[index].rid == rid)
        {
            return &LwSddlSidStringTable[index];
        }
    }

    return NULL;
}

static
PTABLE_ENTRY
RtlpSddlLookupAliasSidString(
    IN PCSTR pszAliasSidString
    )
{
    ULONG index;

    for (index = 0; index < sizeof(LwSddlSidStringTable) / sizeof(*LwSddlSidStringTable); index++)
    {
        if (!strcasecmp(LwSddlSidStringTable[index].pszAliasSidString, pszAliasSidString))
        {
            return &LwSddlSidStringTable[index];
        }
    }

    return NULL;
}
