/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
#include "ctbase.h"

#include <errno.h>

#define ERRNO_HELP \
    "An unexpected system error has occurred.  " \
    "Please contact Likewise technical support for assistance."
    
#define CENTERROR_CATALOG(token) { token, #token, NULL, NULL },
#define CENTERROR_CATALOG_DESC(token, desc, help) { token, #token, desc, help },
#define CENTERROR_CATALOG_ERRNO(token) { CENTERROR_ERRNO(token), #token, NULL, ERRNO_HELP },

static struct catalog_entry
{
    CENTERROR code;
    const char* code_name;
    const char* description;
    const char* help;
} catalog[] =
{
#include "cterr_catalog.h"
    {0, NULL, NULL}
};

static struct catalog_entry*
LookupError(CENTERROR error)
{
    unsigned int i;

    for (i = 0; catalog[i].code_name; i++)
    {
	if (catalog[i].code == error)
	    return &catalog[i];
    }

    return NULL;
}

static struct catalog_entry*
LookupName(const char *name)
{
    unsigned int i;

    for (i = 0; catalog[i].code_name; i++)
    {
	if (!strcmp(catalog[i].code_name, name))
	    return &catalog[i];
    }

    return NULL;
}

const char*
CTErrorName(CENTERROR error)
{
    struct catalog_entry* entry = LookupError(error);
    
    if (!entry)
	return NULL;
    else
	return entry->code_name;
}

CENTERROR
CTErrorFromName(const char* name)
{
    if (!name)
	return CENTERROR_SUCCESS;
    else
    {
	struct catalog_entry* entry = LookupName(name);
	
	if (entry)
	    return entry->code;
	else
	    return CENTERROR_SUCCESS;
    }
}

const char*
CTErrorDescription(CENTERROR error)
{
    if (CENTERROR_IS_ERRNO(error))
    {
	return strerror(CENTERROR_ERRNO_CODE(error));
    }
    else
    {
	struct catalog_entry* entry = LookupError(error);
	
	if (!entry)
	    return NULL;
	else
	    return entry->description;
    }
}

const char*
CTErrorHelp(CENTERROR error)
{
    struct catalog_entry* entry = LookupError(error);
    
    if (!entry)
	return NULL;
    else
	return entry->help;
}

CENTERROR
CTMapSystemError(
    int dwError
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    switch(dwError)
    {
    case 0:
        ceError = CENTERROR_SUCCESS;
        break;

    case EPERM:
        ceError = CENTERROR_INVALID_OPERATION;
        break;

    case EACCES:
        ceError = CENTERROR_ACCESS_DENIED;
        break;

    case ENOMEM:
        ceError = CENTERROR_OUT_OF_MEMORY;
        break;

    case EINVAL:
        ceError = CENTERROR_INVALID_PARAMETER;
        break;

    case ECONNREFUSED:
        ceError = CENTERROR_CONNECTION_REFUSED;
        break;

    default:
        ceError = CENTERROR_ERRNO(dwError);
        break;
    }

    return ceError;
}

