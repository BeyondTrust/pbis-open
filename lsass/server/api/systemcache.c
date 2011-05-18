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
 *        systemcache.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
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
