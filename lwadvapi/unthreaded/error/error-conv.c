/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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

#include "config.h"
#include <stdlib.h>
#include <errno.h>

#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <lw/errno.h>
#include <lwerror.h>

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

} status_table_exterror[] =
{
#include <lwerror-table.h>
    {-1, 0, 0}
};

#undef STATUS_CODE
#undef __ERROR_XMACRO__

typedef int (*predicate) (struct table_entry* e, void* data);

static
PCSTR
LwErrorToName(
    LW_WINERROR winerr
    );

static
PCSTR
LwErrorToDescription(
    LW_WINERROR winerr
    );

static int
match_werror(
    struct table_entry* e,
    void *data
    )
{
    return e->werror == *((LW_WINERROR*) data);
}

static struct table_entry*
find(
    predicate pred,
    void* data
    )
{
    unsigned int i;

    for (i = 0; i < sizeof(status_table_exterror)/sizeof(status_table_exterror[0]); i++)
    {
        if (pred(&status_table_exterror[i], data))
            return &status_table_exterror[i];
    }

    return NULL;
}

PCSTR
LwWin32ExtErrorToName(
    LW_WINERROR winerr
    )
{
	PCSTR pszError = LwWin32ErrorToName(winerr);

	if (!pszError)
	{
		pszError = LwErrorToName(winerr);
	}

	return pszError;
}

PCSTR
LwWin32ExtErrorToDescription(
    LW_WINERROR winerr
    )
{
	PCSTR pszError = LwWin32ErrorToDescription(winerr);

	if (!pszError)
	{
		pszError = LwErrorToDescription(winerr);
	}

	return pszError;
}


static
PCSTR
LwErrorToName(
    LW_WINERROR winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->pszWinerrName : NULL;
}

static
PCSTR
LwErrorToDescription(
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
