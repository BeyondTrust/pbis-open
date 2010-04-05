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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 * Eventlog server security wrapper
 *
 */

#include "includes.h"

static void *                gpLsaAccessLibHandle = (void*)NULL;

static PFNLSAACCESSGETDATA   gpfnLsaAccessGetData = NULL;
static PFNLSAACCESSCHECKDATA gpfnLsaAccessCheckData = NULL;
static PFNLSAACCESSFREEDATA  gpfnLsaAccessFreeData = NULL;

#define LSAACCESS_LIBPATH    LIBDIR "/" "liblsaaccess" MOD_EXT

DWORD
EVTLoadLsaLibrary(
    )
{
    DWORD dwError = 0;
    PCSTR pszError = NULL;

    if ( gpLsaAccessLibHandle )
    {
        goto cleanup;
    }

    dlerror();

    gpLsaAccessLibHandle = dlopen(LSAACCESS_LIBPATH,
                                  RTLD_NOW | RTLD_GLOBAL);
    if ( gpLsaAccessLibHandle == NULL )
    {
        pszError = dlerror();
        dwError = EVT_ERROR_LOAD_LIBRARY_FAILED;
        EVT_LOG_ERROR(
            "Failed to load library [%s]. Error [%s]",
            LSAACCESS_LIBPATH,
            (IsNullOrEmptyString(pszError) ? "" : pszError));
        goto error;
    }

    gpfnLsaAccessGetData = (PFNLSAACCESSGETDATA)dlsym(
                               gpLsaAccessLibHandle,
                               LSA_SYMBOL_NAME_ACCESS_GET_DATA);
    if ( gpfnLsaAccessGetData == NULL )
    {
        EVT_LOG_ERROR(
            "Unable to find LSA Access API - %s",
            LSA_SYMBOL_NAME_ACCESS_GET_DATA);
        dwError = EVT_ERROR_LOOKUP_SYMBOL_FAILED;
        goto error;
    }

    gpfnLsaAccessCheckData = (PFNLSAACCESSCHECKDATA)dlsym(
                                 gpLsaAccessLibHandle,
                                 LSA_SYMBOL_NAME_ACCESS_CHECK_DATA);
    if ( gpfnLsaAccessCheckData == NULL )
    {
        EVT_LOG_ERROR(
            "Unable to find LSA Access API - %s",
            LSA_SYMBOL_NAME_ACCESS_CHECK_DATA);
        dwError = EVT_ERROR_LOOKUP_SYMBOL_FAILED;
        goto error;
    }

    gpfnLsaAccessFreeData = (PFNLSAACCESSFREEDATA)dlsym(
                                gpLsaAccessLibHandle,
                                LSA_SYMBOL_NAME_ACCESS_FREE_DATA);
    if ( gpfnLsaAccessGetData == NULL )
    {
        EVT_LOG_ERROR(
            "Unable to find LSA Access API - %s",
            LSA_SYMBOL_NAME_ACCESS_FREE_DATA);
        dwError = EVT_ERROR_LOOKUP_SYMBOL_FAILED;
        goto error;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
EVTAccessGetData(
    PCSTR   pczConfigData,
    PVOID * ppAccessData
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    DWORD  dwIndex = 0;
    PCSTR  cp = NULL;
    PCSTR  cp2 = NULL;
    PSTR   cp3 = NULL;
    PSTR * ppczStrArray = NULL;

    for ( cp = pczConfigData ; *cp !=  0 ; cp++ )
    {
        if ( *cp == ',' ) dwCount++;
    }
    dwCount++;
    dwError = EVTAllocateMemory(
                  (dwCount+1)*sizeof(PCSTR),
                  (PVOID *)&ppczStrArray);

    cp = pczConfigData;
    for ( ;; )
    {
         cp2 = strchr(cp, ',');
         if ( cp2 ) {
             dwError = EVTStrndup( cp, cp2 - cp, &cp3 );
             BAIL_ON_EVT_ERROR(dwError);
         }
         else
         {
             dwError = EVTStrndup( cp, strlen(cp), &cp3 );
             BAIL_ON_EVT_ERROR(dwError);
         }
         EVTStripWhitespace(cp3, TRUE, TRUE);
         if ( strlen(cp3) > 0 )
         {
             ppczStrArray[dwIndex++] = cp3;
         }
         else
         {
             EVTFreeMemory(cp3);
         }

         if ( !cp2 ) break;
         cp = ++cp2;
    }
    if ( dwIndex == 0 )
    {
        *ppAccessData = NULL;
        goto cleanup;
    }

    if ( gpfnLsaAccessGetData )
    {
        dwError = gpfnLsaAccessGetData(
                      (PCSTR *)ppczStrArray,
                      ppAccessData);
    }
    else
    {
        dwError = EVT_ERROR_LOOKUP_SYMBOL_FAILED;
    }

cleanup:
    if ( ppczStrArray ) {
        for ( dwIndex = 0 ; ppczStrArray[dwIndex] != NULL ; dwIndex++ )
        {
            EVTFreeString(ppczStrArray[dwIndex]);
        }
        EVTFreeMemory(ppczStrArray);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
EVTAccessCheckData(
    PCSTR pczUserName,
    PVOID pAccessData
    )
{
    DWORD dwError = 0;

    if ( !gpfnLsaAccessCheckData )
    {
        dwError = EVT_ERROR_LOOKUP_SYMBOL_FAILED;
    }
    BAIL_ON_EVT_ERROR(dwError);

    dwError = gpfnLsaAccessCheckData(
                  pczUserName,
                  pAccessData);
    if ( dwError )
    {
        dwError = EVT_ERROR_ACCESS_DENIED;
    }
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
EVTAccessFreeData(
    PVOID pAccessData
    )
{
    DWORD dwError = 0;

    if ( gpfnLsaAccessFreeData )
    {
        dwError = gpfnLsaAccessFreeData(pAccessData);
    }
    else
    {
        dwError = EVT_ERROR_LOOKUP_SYMBOL_FAILED;
    }

    return dwError;
}

DWORD
EVTUnloadLsaLibrary(
    )
{
    DWORD dwError = 0;

    if ( gpLsaAccessLibHandle )
    {
        dlclose(gpLsaAccessLibHandle);
    }

    return dwError;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
