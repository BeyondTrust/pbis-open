/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
