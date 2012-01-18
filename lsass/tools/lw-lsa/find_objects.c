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
 *        find_objects.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Tool to find objects
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

#define SAFE_STRING(x) ((x) == NULL ? "<null>" : (x))

static struct
{
    PCSTR pszTargetProvider;
    LSA_FIND_FLAGS FindFlags;
    LSA_OBJECT_TYPE ObjectType;
    LSA_QUERY_TYPE QueryType;
    DWORD dwCount;
    LSA_QUERY_LIST QueryList;
    BOOLEAN bShowUsage;
} gState = 
{
    .pszTargetProvider = NULL,
    .FindFlags = 0,
    .ObjectType = LSA_OBJECT_TYPE_UNDEFINED,
    .QueryType = LSA_QUERY_TYPE_UNDEFINED,
    .dwCount = 0,
    .bShowUsage = FALSE
};

static
DWORD
ParseQueryItem(
    PCSTR pszArg
    )
{
    DWORD dwError = 0;
    unsigned long id = 0;
    LSA_QUERY_LIST NewList;

    switch (gState.QueryType)
    {
    case LSA_QUERY_TYPE_BY_UNIX_ID:
        id = strtoul(pszArg, NULL, 10);
        dwError = LwReallocMemory(
            gState.QueryList.pdwIds,
            OUT_PPVOID(&NewList.pdwIds),
            sizeof(*NewList.pdwIds) * (gState.dwCount + 1));
        BAIL_ON_LSA_ERROR(dwError);
        NewList.pdwIds[gState.dwCount] = (DWORD) id;
        gState.QueryList.pdwIds = NewList.pdwIds;
        gState.dwCount++;
        break;
    default:
        dwError = LwReallocMemory(
            gState.QueryList.ppszStrings,
            OUT_PPVOID(&NewList.ppszStrings),
            sizeof(*NewList.ppszStrings) * (gState.dwCount + 1));
        BAIL_ON_LSA_ERROR(dwError);
        NewList.ppszStrings[gState.dwCount] = pszArg;
        gState.QueryList.ppszStrings = NewList.ppszStrings;
        gState.dwCount++;
        break;
    }

error:

    return dwError;
}

static
DWORD
SetQueryType(
    LSA_QUERY_TYPE type
    )
{
    DWORD dwError = 0;

    if (gState.QueryType != LSA_QUERY_TYPE_UNDEFINED)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        gState.QueryType = type;
    }

error:

    return dwError;
}

static
VOID
ShowUsage(
    PCSTR pszProgramName,
    BOOLEAN bFull
    )
{
    printf(
        "Usage: %s [ --<object type> ] [ --by-<key> ] [ <flags> ] [ --provider name ] keys ...\n",
        Basename(pszProgramName));
    
    if (bFull)
    {
        printf(
            "\n"
            "Object type options:\n"
            "    --user                  Return only user objects\n"
            "    --group                 Return only group objects\n"
            "\n"
            "Key type options:\n"
            "    --by-dn                 Query by distinguished name\n"
            "    --by-sid                Query by SID\n"
            "    --by-nt4                Query by NT4-style domain-qualified name\n"
            "    --by-alias              Query by alias (must specify object type)\n"
            "    --by-upn                Query by user principal name\n"
            "    --by-unix-id            Query by UID or GID (must specify object type)\n"
            "    --by-name               Query by generic name (NT4, alias, or UPN accepted)\n"
            "\n"
            "Query flags:\n"
            "     --nss                  Omit data not necessary for NSS layer\n"
            "\n"
            "Other options:\n"
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
        else if (!strcmp(ppszArgv[i], "--by-dn"))
        {
            dwError = SetQueryType(LSA_QUERY_TYPE_BY_DN);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[i], "--by-sid"))
        {
            dwError = SetQueryType(LSA_QUERY_TYPE_BY_SID);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[i], "--by-nt4"))
        {
            dwError = SetQueryType(LSA_QUERY_TYPE_BY_NT4);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[i], "--by-alias"))
        {
            dwError = SetQueryType(LSA_QUERY_TYPE_BY_ALIAS);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[i], "--by-upn"))
        {
            dwError = SetQueryType(LSA_QUERY_TYPE_BY_UPN);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[i], "--by-unix-id"))
        {
            dwError = SetQueryType(LSA_QUERY_TYPE_BY_UNIX_ID);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[i], "--by-name"))
        {
            dwError = SetQueryType(LSA_QUERY_TYPE_BY_NAME);
            BAIL_ON_LSA_ERROR(dwError);
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
        else if (!strcmp(ppszArgv[i], "--help") ||
                 !strcmp(ppszArgv[i], "-h"))
        {
            gState.bShowUsage = TRUE;
        }
        else
        {
            dwError = ParseQueryItem(ppszArgv[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

error:

    return dwError;
}

static
DWORD
FindObjects(
    VOID
    )
{
    DWORD dwError = 0;
    HANDLE hLsa = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwIndex = 0;

    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindObjects(
        hLsa,
        gState.pszTargetProvider,
        gState.FindFlags,
        gState.ObjectType,
        gState.QueryType,
        gState.dwCount,
        gState.QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    for (dwIndex = 0; dwIndex < gState.dwCount; dwIndex++)
    {
        if (ppObjects[dwIndex])
        {
            PrintSecurityObject(ppObjects[dwIndex], dwIndex, gState.dwCount);
        }
        else
        {
            switch (gState.QueryType)
            {
            case LSA_QUERY_TYPE_BY_UNIX_ID:
                printf("Not found [%u of %u]: %lu\n",
                       dwIndex + 1, gState.dwCount,
                       (unsigned long) gState.QueryList.pdwIds[dwIndex]);
                break;
            default:
                printf("Not found [%u of %u]: %s\n", 
                       dwIndex + 1, gState.dwCount,
                       gState.QueryList.ppszStrings[dwIndex]);
                break;
            }
        }
        printf("\n");
    }

error:

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(gState.dwCount, ppObjects);
    }

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    return dwError;
}

int
FindObjectsMain(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;

    dwError = ParseArguments(argc, ppszArgv);
    BAIL_ON_LSA_ERROR(dwError);

    if (gState.QueryType == LSA_QUERY_TYPE_UNDEFINED)
    {
        /* Default to querying by name */
        gState.QueryType = LSA_QUERY_TYPE_BY_NAME;
    }

    if (!gState.bShowUsage)
    {
        if (gState.dwCount == 0)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = FindObjects();
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
