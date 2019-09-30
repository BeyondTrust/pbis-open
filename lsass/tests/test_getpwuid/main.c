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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for exercising getpwuid
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
    printf("Usage: test-getpwuid <uid>\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[],
    uid_t* pUID
    )
{
    int iArg = 1;
    PSTR pszArg = NULL;
    uid_t uid = 0;
    BOOLEAN bUIDSpecified = FALSE;

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
        else
        {
            uid = (uid_t) atoi(pszArg);
            bUIDSpecified = TRUE;
            break;
        }
    } while (iArg < argc);

    if (!bUIDSpecified) {
       fprintf(stderr, "Please specify a user id to query for.\n");
       ShowUsage();
       exit(1);
    }

    *pUID = uid;
    
    return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    struct passwd user = {0};
    struct passwd* pUser = NULL;
    char   szBuf[1024];
    uid_t uid = 0;

    dwError = ParseArgs(argc, argv, &uid);
    BAIL_ON_LSA_ERROR(dwError);

    if (getpwuid_r(uid, &user, szBuf, sizeof(szBuf), &pUser) < 0) {
       fprintf(stderr, "Failed to lookup user [uid:%u]", (unsigned int)uid);
       dwError = LwMapErrnoToLwError(errno);
       BAIL_ON_LSA_ERROR(dwError);
    }

    if(pUser)
    {
        printf("User info:\n");
        printf("==========\n");
        printf("Name:     %s\n", LW_IS_NULL_OR_EMPTY_STR(pUser->pw_name) ? "<null>" : pUser->pw_name);
        printf("Uid:      %u\n", (unsigned int)pUser->pw_uid);
        printf("Gid:      %u\n", (unsigned int)pUser->pw_gid);
        printf("Gecos:    %s\n", LW_IS_NULL_OR_EMPTY_STR(pUser->pw_gecos) ? "<null>" : pUser->pw_gecos);
        printf("Shell:    %s\n", LW_IS_NULL_OR_EMPTY_STR(pUser->pw_shell) ? "<null>" : pUser->pw_shell);
        printf("Home dir: %s\n", LW_IS_NULL_OR_EMPTY_STR(pUser->pw_dir) ? "<null>" : pUser->pw_dir);
    }
    else
    {
        printf("pUser == NULL\n");
    }

cleanup:

    return (dwError);

error:

    goto cleanup;
}
