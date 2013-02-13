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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for exercising setpwent/getpwent/endpwent
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
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
    printf("Usage: test-getpwent\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[]
    )
{
    int iArg = 1;
    PSTR pszArg = NULL;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
    } while (iArg < argc);

    return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    struct passwd* pUser = NULL;
    BOOLEAN bEndPwent = FALSE;
    DWORD   iUser = 0;

    dwError = ParseArgs(argc, argv);
    BAIL_ON_LSA_ERROR(dwError);

    setpwent();
    bEndPwent = TRUE;

    while ((pUser = getpwent()) != NULL)
    {
          printf("User #%u", ++iUser);
          printf("==========\n");
          printf("Name:     %s\n",
                  LW_IS_NULL_OR_EMPTY_STR(pUser->pw_name) ? "<null>" : pUser->pw_name);
          printf("Uid:      %u\n", (unsigned int)pUser->pw_uid);
          printf("Gid:      %u\n", (unsigned int)pUser->pw_gid);
          printf("Gecos:    %s\n", 
                  LW_IS_NULL_OR_EMPTY_STR(pUser->pw_gecos) ? "<null>" : pUser->pw_gecos);
          printf("Shell:    %s\n",
                  LW_IS_NULL_OR_EMPTY_STR(pUser->pw_shell) ? "<null>" : pUser->pw_shell);
          printf("Home dir: %s\n",
                  LW_IS_NULL_OR_EMPTY_STR(pUser->pw_dir) ? "<null>" : pUser->pw_dir);
    }

cleanup:

    if (bEndPwent) {
       endpwent();
    }

    return (dwError);

error:

    goto cleanup;
}
