/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
