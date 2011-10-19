/*Copyright Likewise Software    2004-2008
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

#include "includes.h"

#define STATUS_CODE(status, werror, errno, desc)             \
    {status, werror, errno, #status, #werror, #errno, desc },
#define __ERROR_XMACRO__

struct table_entry
{
    NTSTATUS    ntStatus;
    WINERROR    werror;
    int         uerror;
    PCSTR       pszStatusName;
    PCSTR       pszWinerrName;
    PCSTR       pszUnixErrnoName;
    PCSTR       pszDescription;

} status_table[] =
{
#include "error-table.h"
    {-1, 0, 0}
};

#undef STATUS_CODE
#undef __ERROR_XMACRO__

typedef int (*predicate) (struct table_entry* e, void* data);

static int
match_status(
    struct table_entry* e,
    void *data
    )
{
    return e->ntStatus == *((LW_NTSTATUS*) data);
}


static int
match_werror(
    struct table_entry* e,
    void *data
    )
{
    return e->werror == *((LW_WINERROR*) data);
}


static int
match_uerror(
    struct table_entry* e,
    void *data
    )
{
    return e->uerror == *((int*) data);
}


static struct table_entry*
find(
    predicate pred,
    void* data
    )
{
    unsigned int i;

    for (i = 0; i < sizeof(status_table)/sizeof(status_table[0]); i++)
    {
        if (pred(&status_table[i], data))
            return &status_table[i];
    }

    return NULL;
}

/* Converting NT status errors */
LW_WINERROR
LwNtStatusToWin32Error(
    LW_NTSTATUS ntStatus
    )
{
    struct table_entry *e = find(match_status, &ntStatus);
    return e ? e->werror : (LW_WINERROR)-1;
}

/* Converting Unix errors */
LW_WINERROR
LwErrnoToWin32Error(
    int uerror
    )
{
    struct table_entry *e = find(match_uerror, &uerror);
    return e ? e->werror : (LW_WINERROR)-1;
}

LW_PCSTR
LwErrnoToDescription(
    int uerror
    )
{
    struct table_entry *e = find(match_werror, &uerror);
    return (e) ? e->pszDescription : NULL;
}


/*Converting WinErrors */
int
LwWin32ErrorToErrno(
    LW_WINERROR winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->uerror : 0;
}

NTSTATUS
LwWin32ErrorToNtStatus(
    LW_WINERROR winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->ntStatus : (NTSTATUS)-1;
}

PCSTR
LwWin32ErrorToName(
    LW_WINERROR winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->pszWinerrName : NULL;
}

PCSTR
LwWin32ErrorToDescription(
    LW_WINERROR winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->pszDescription : NULL;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

