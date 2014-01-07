/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    
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
