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
 *        BeyondTrust Site Manager
 *
 *        Client Test Program
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"
#include "client/ipc_client_p.h"

static
void
ShowUsage()
{
    printf("Usage: netlogonclient\n");
}

static
int
ParseArgs(
    int argc,
    char* argv[])
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
    HANDLE hLWNetConnection = (HANDLE)NULL;

    lwnet_init_logging_to_file(LWNET_LOG_LEVEL_VERBOSE, TRUE, "");

    ParseArgs(argc, argv);

    dwError = LWNetOpenServer(&hLWNetConnection);
    BAIL_ON_LWNET_ERROR(dwError);

    LWNetCloseServer(hLWNetConnection);
    hLWNetConnection = (HANDLE)NULL;

    LWNET_LOG_INFO("Successfully communicated with the LWNET Agent.\n");

    return (dwError);

  error:
 
    if (hLWNetConnection != (HANDLE)NULL) {
       LWNetCloseServer(hLWNetConnection);
    }

    LWNET_LOG_ERROR("Failed communication with likewise-netlogond. Error code [%d]\n", dwError);

    return (dwError);
}
