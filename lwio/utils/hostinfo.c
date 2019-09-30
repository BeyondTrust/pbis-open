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
 *        hostinfo.c
 *
 * Abstract:
 *
 *        BeyondTrust I/O Subsystem (LWIO)
 *
 *        Host Info API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LwioGetHostInfo(
    PSTR* ppszHostname
    )
{
    DWORD dwError = 0;
    CHAR szBuffer[256];
    PSTR pszLocal = NULL;
    PSTR pszDot = NULL;
    int len = 0;
    PSTR pszHostname = NULL;

    if ( gethostname(szBuffer, sizeof(szBuffer)) != 0 )
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    len = strlen(szBuffer);
    if ( len > strlen(".local") )
    {
        pszLocal = &szBuffer[len - strlen(".local")];
        if ( !strcasecmp( pszLocal, ".local" ) )
        {
            pszLocal[0] = '\0';
        }
    }
    
    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    pszDot = strchr(szBuffer, '.');
    if ( pszDot )
    {
        pszDot[0] = '\0';
    }

    dwError = SMBAllocateString(szBuffer, &pszHostname);
    BAIL_ON_LWIO_ERROR(dwError);

    if ( ppszHostname ) {
        *ppszHostname = pszHostname;
        pszHostname = NULL;
    }
        
cleanup:

    LWIO_SAFE_FREE_STRING(pszHostname);

    return dwError;

error:
    
    if (ppszHostname)
        *ppszHostname = NULL;
    
    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
