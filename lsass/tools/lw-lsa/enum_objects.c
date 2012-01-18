/*
 * Copyright Likewise Software
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
 * Module Name:
 *
 *        enum_objects.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Tool to enum objects
 *
 * Authors: Brian Koropoff(bkoropoff@likewise.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsaclient.h"
#include "common.h"

#include <lsa/lsa.h>
#include <lwmem.h>
#include <lwerror.h>

static struct
{
    PCSTR pszTargetProvider;
    LSA_FIND_FLAGS FindFlags;
    LSA_OBJECT_TYPE ObjectType;
    PCSTR pszDomainName;
    BOOLEAN bShowUsage;
} gState = 
{
    .pszTargetProvider = NULL,
    .FindFlags = 0,
    .ObjectType = LSA_OBJECT_TYPE_UNDEFINED,
    .pszDomainName = NULL,
    .bShowUsage = FALSE
};

static
VOID
ShowUsage(
    PCSTR pszProgramName,
    BOOLEAN bFull
    )
{
    printf(
        "Usage: %s [ --<object type> ] [ <flags> ] [ --provider name ]\n",
        Basename(pszProgramName));
    
    if (bFull)
    {
        printf(
            "\n"
            "Object type options:\n"
            "    --user                  Return only user objects\n"
            "    --group                 Return only group objects\n"
            "\n"
            "Query flags:\n"
            "     --nss                  Omit data not necessary for NSS layer\n"
            "\n"
            "Other options:\n"
            "     --domain name          Restrict enumeration to the specified NetBIOS domain\n"
            "     --provider name        Direct request to provider with the specified name\n"
            "\n");
    }
}

static
DWORD
ParseArguments(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;
    int i = 0;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(ppszArgv[i], "--user"))
        {
            gState.ObjectType = LSA_OBJECT_TYPE_USER;
        }
        else if (!strcmp(ppszArgv[i], "--group"))
        {
            gState.ObjectType = LSA_OBJECT_TYPE_GROUP;
        }
        else if (!strcmp(ppszArgv[i], "--nss"))
        {
            gState.FindFlags |= LSA_FIND_FLAGS_NSS;
        }
        else if (!strcmp(ppszArgv[i], "--provider"))
        {
            i++;
                
            if (i >= argc)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }

            gState.pszTargetProvider = ppszArgv[i];
        }
        else if (!strcmp(ppszArgv[i], "--domain"))
        {
            i++;
                
            if (i >= argc)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }

            gState.pszDomainName = ppszArgv[i];
        }
        else if (!strcmp(ppszArgv[i], "--help") ||
                 !strcmp(ppszArgv[i], "-h"))
        {
            gState.bShowUsage = TRUE;
        }
        else
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

error:

    return dwError;
}

static
DWORD
EnumObjects(
    VOID
    )
{
    DWORD dwError = 0;
    HANDLE hLsa = NULL;
    HANDLE hEnum = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    const DWORD dwMaxCount = 512;
    DWORD dwCount = 0;
    DWORD dwIndex = 0;
    DWORD dwTotalIndex = 0;

    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenEnumObjects(
        hLsa,
        gState.pszTargetProvider,
        &hEnum,
        gState.FindFlags,
        gState.ObjectType,
        gState.pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    for (dwTotalIndex = 0;;)
    {
        dwError = LsaEnumObjects(
            hLsa,
            hEnum,
            dwMaxCount,
            &dwCount,
            &ppObjects);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = 0;
            break;
        }
        BAIL_ON_LSA_ERROR(dwError);

        for (dwIndex = 0; dwIndex < dwCount; dwIndex++, dwTotalIndex++)
        {
            if (ppObjects[dwIndex])
            {
                PrintSecurityObject(ppObjects[dwIndex], dwTotalIndex, 0);
                printf("\n");
            }
        }

        if (ppObjects)
        {
            LsaFreeSecurityObjectList(dwCount, ppObjects);
        }
    }

cleanup:

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(dwCount, ppObjects);
    }
    
    if (hEnum)
    {
        LsaCloseEnum(hLsa, hEnum);
    }

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    return dwError;

error:

    goto cleanup;
}

int
EnumObjectsMain(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;

    dwError = ParseArguments(argc, ppszArgv);
    BAIL_ON_LSA_ERROR(dwError);

   if (!gState.bShowUsage)
   {
       dwError = EnumObjects();
       BAIL_ON_LSA_ERROR(dwError);
   }

error:

    if (gState.bShowUsage)
    {
        ShowUsage(ppszArgv[0], TRUE);
    }
    else if (dwError == LW_ERROR_INVALID_PARAMETER)
    {
        ShowUsage(ppszArgv[0], FALSE);
    }

    if (dwError)
    {
        printf("Error: %s (%x)\n", LwWin32ExtErrorToName(dwError), dwError);
        return 1;
    }
    else
    {
        return 0;
    }
}
