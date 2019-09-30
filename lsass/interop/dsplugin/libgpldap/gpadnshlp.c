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

#include "../includes.h"
#include "dns.h"

/* used by inet_addr, not defined on Solaris anywhere!? */
#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) -1)
#endif

long
GetDnsHostName(
    char ** hostname
    )
{
    long macError = eDSNoErr;
    char buffer[256];
    char * szLocal = NULL;
    char * szDot = NULL;
    uint len = 0;
    PSTR pszHostName = NULL;

    if ( gethostname(buffer, sizeof(buffer)) != 0 )
    {
        macError = ePlugInInitError;
        if (macError)
        {
            goto cleanup;
        }
    }

    len = strlen(buffer);
    if ( len > strlen(".local") )
    {
        szLocal = &buffer[len - strlen(".local")];
        if ( !strcasecmp( szLocal, ".local" ) )
        {
            szLocal[0] = '\0';
        }
    }

    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    szDot = strchr(buffer, '.');
    if ( szDot )
    {
        szDot[0] = '\0';
    }

    macError = LwAllocateString(buffer, &pszHostName);
    if (macError)
    {
        goto cleanup;
    }

    if ( hostname ) {
        *hostname = pszHostName;
        pszHostName = NULL;
    }

    // return below so that we free the unrequested out parameters.

cleanup:

    LW_SAFE_FREE_STRING(pszHostName);

    return macError;
}
