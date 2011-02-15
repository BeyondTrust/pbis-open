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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        GSSAPI Wrappers
 *
 *        Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

DWORD
LsaGssInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    pthread_mutex_init(&gLsaGSSGlobals.mutex, NULL);
    gLsaGSSGlobals.pMutex = &gLsaGSSGlobals.mutex;

    dwError = LwNtStatusToWin32Error(
                    LwMapSecurityInitialize()
                    );
    BAIL_ON_LSA_GSS_ERROR(dwError);

    gLsaGSSGlobals.bLwMapSecurityInitialized = TRUE;

    dwError = LwNtStatusToWin32Error(
                    LwMapSecurityCreateContext(
                            &gLsaGSSGlobals.pSecurityContext)
                    );
    BAIL_ON_LSA_GSS_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (gLsaGSSGlobals.bLwMapSecurityInitialized)
    {
        LwMapSecurityCleanup();

        gLsaGSSGlobals.bLwMapSecurityInitialized = FALSE;
    }

    goto cleanup;
}

VOID
LsaGssShutdown(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    if (gLsaGSSGlobals.pMutex)
    {
        LSA_GSS_LOCK_MUTEX(bInLock, &gLsaGSSGlobals.mutex);

        if (gLsaGSSGlobals.pSecurityContext)
        {
            LwMapSecurityFreeContext(&gLsaGSSGlobals.pSecurityContext);
        }

        if (gLsaGSSGlobals.bLwMapSecurityInitialized)
        {
            LwMapSecurityCleanup();

            gLsaGSSGlobals.bLwMapSecurityInitialized = FALSE;
        }

        pthread_mutex_destroy(&gLsaGSSGlobals.mutex);
        gLsaGSSGlobals.pMutex = NULL;
    }

    LSA_GSS_UNLOCK_MUTEX(bInLock, &gLsaGSSGlobals.mutex);
}

VOID
LsaGssFreeStringArray(
    PSTR* ppszStringArray,          /* IN OUT          */
    DWORD dwNumStrings              /* IN              */
    )
{
    if (ppszStringArray)
    {
        DWORD iString = 0;

        for (; iString < dwNumStrings; iString++)
        {
            if (ppszStringArray[iString])
            {
                LwFreeMemory(ppszStringArray[iString]);
            }
        }

        LwFreeMemory(ppszStringArray);
    }
}

VOID
LsaGssFreeMemory(
    PVOID pMemory                   /* IN OUT          */
    )
{
    if (pMemory)
    {
        LwFreeMemory(pMemory);
    }
}

