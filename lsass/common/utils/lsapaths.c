/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lsapaths.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        API to get special filesystem paths
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
LsaGetPrefixDirPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR  pszPath = NULL;
    
    dwError = LwAllocateString(
                    PREFIXDIR,
                    &pszPath);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppszPath = pszPath;
    
cleanup:

    return dwError;
    
error:

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
LsaGetLibDirPath(
    PSTR* ppszLibPath
    )
{
    DWORD   dwError = 0;
    BOOLEAN bExists = FALSE;
    PSTR    pszLibPath = NULL;
        
    dwError = LsaCheckDirectoryExists(
                        PREFIXDIR "/lib64",
                        &bExists);
    BAIL_ON_LSA_ERROR(dwError);
        
    if (bExists) {
       dwError = LwStrndup(
                     PREFIXDIR "/lib64",
                     strlen(PREFIXDIR "/lib64"),
                     &pszLibPath);
       BAIL_ON_LSA_ERROR(dwError);
    } else {
       dwError = LwStrndup(
                      PREFIXDIR "/lib",
                      strlen(PREFIXDIR "/lib"),
                      &pszLibPath);
       BAIL_ON_LSA_ERROR(dwError);
    }
        
    *ppszLibPath = pszLibPath;
        
cleanup:

    return dwError;
        
error:

    *ppszLibPath = NULL;
        
    LW_SAFE_FREE_STRING(pszLibPath);

    goto cleanup;
}

