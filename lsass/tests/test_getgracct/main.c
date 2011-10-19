/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"

static
void
ShowUsage()
{
    printf("Usage: test-getgracct idtogroup <gid>\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[],
    gid_t* gid
    )
{
    PSTR  pszCommand = NULL;
    PSTR  pszGid     = NULL;
    DWORD ret        = 0;

    PSTR pszArg = NULL;

    if( argc != 3 ) {
        ShowUsage();
        exit(0);
    }

    pszCommand = argv[1];
    if( strcmp(pszCommand, "idtogroup")!=0 ) {
        ShowUsage();
        exit(0);
    }

    pszGid = argv[2];
    *gid   = (gid_t) atoi(pszGid);
    

cleanup:

    return ret;

error:

    ret = 1;

    goto cleanup;
}

int
main(
    int argc,
    char* argv[]
    )
{
    gid_t gid;
    DWORD dwError;
    int   ret          = 0;
    PSTR  pszId        = NULL;
    PSTR  pszGroupName = NULL;
    
    dwError = ParseArgs(argc, argv, &gid);

    pszGroupName = IDtogroup ( gid );

    printf("IDtoGroup:\n");
    printf("==========\n");
    printf("Gid:        %u\n", (unsigned int)gid);
    printf("Group Name: %s\n" , LW_IS_NULL_OR_EMPTY_STR(pszGroupName) ? "<null>" : pszGroupName);

cleanup:

    LW_SAFE_FREE_STRING(pszGroupName);

    return ret;

error:

    ret = 1;

    goto cleanup;
}
