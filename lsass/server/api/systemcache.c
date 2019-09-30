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
 *        systemcache.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) 
 *                    
 *        A function to flush the operating system's user/group cache.
 *
 * Authors: 
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvFlushSystemCache(
    VOID
    )
{
    DWORD dwError = 0;

#if defined (__LWI_DARWIN__)
    int i;
    const char* cacheUtils[] = {
        "/usr/sbin/lookupd", /* Before Mac OS X 10.5 */
        "/usr/bin/dscacheutil" /* On Mac OS X 10.5 */
    };
    const char* cacheUtilCmd[] = {
        "/usr/sbin/lookupd -flushcache", /* Before Mac OS X 10.5 */
        "/usr/bin/dscacheutil -flushcache" /* On Mac OS X 10.5 */
    };
    BOOLEAN bCacheFlushed = FALSE;

    LSA_LOG_VERBOSE("Going to flush the Mac DirectoryService cache ...");

    for (i = 0;
         !bCacheFlushed && i < (sizeof(cacheUtils) / sizeof(cacheUtils[0]));
         i++)
    {
        const char* util = cacheUtils[i];
        const char* command = cacheUtilCmd[i];
        BOOLEAN exists;

        /* Sanity check */
        if (!util)
        {
            continue;
        }

        dwError = LsaCheckFileExists(util, &exists);
        BAIL_ON_LSA_ERROR(dwError);

        if (!exists)
        {
            continue;
        }

        system(command);

        /* Bail regardless */
        bCacheFlushed = TRUE;
    }

    if (!bCacheFlushed)
    {
        LSA_LOG_ERROR("Could not locate cache flush utility");
        dwError = LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LSA_LOG_VERBOSE("Finished flushing the Mac DirectoryService cache");
#endif
    return dwError;

#if defined (__LWI_DARWIN__)
error:

    goto cleanup;
#endif
}
