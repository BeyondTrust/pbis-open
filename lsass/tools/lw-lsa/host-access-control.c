/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Beyondtrust Software    2017
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

#include "config.h"

#include <lsasystem.h>

#include <lsa/lsa.h>
#include <lsaclient.h>
#include <lw/base.h>

#include <lwerror.h>
#include <lwdef.h>
#include <reg/lwreg.h>
#include <reg/regutil.h>

#define AD_PROVIDER_REGKEY "Services\\lsass\\Parameters\\Providers\\ActiveDirectory"
#define AD_PROVIDER_HOST_ACCESS_REGKEY AD_PROVIDER_REGKEY "\\HostAccess"
#define AD_PROVIDER_POLICY_REGKEY "Policy\\" AD_PROVIDER_REGKEY

typedef struct _POLICY_LIST
{
    PSTR* pszaPolicyList;
    DWORD dwPolicyCount;
    PSTR* pszaLocalList;
    DWORD dwLocalCount;
} POLICY_LIST, *P_POLICY_LIST;

static
VOID
ParseArgs(
    int    argc,
    char*  argv[]
    );

static
VOID
ShowUsage(const char *cmd);

static
DWORD
PolicyListGet(
        HANDLE hReg,
        PSTR pszPolicyKey,
        PSTR pszLocalKey,
        PSTR pszKey,
        P_POLICY_LIST pLists
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    REG_DATA_TYPE regType = REG_MULTI_SZ;
    
    dwError = RegUtilGetValue(
                hReg,
                HKEY_THIS_MACHINE,
                pszPolicyKey,
                NULL,
                pszKey,
                &regType,
                (PVOID) &pLists->pszaPolicyList,
                &pLists->dwPolicyCount);
    
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = RegUtilGetValue(
                    hReg,
                    HKEY_THIS_MACHINE,
                    pszLocalKey,
                    NULL,
                    pszKey,
                    &regType,
                    (PVOID) &pLists->pszaLocalList,
                    &pLists->dwLocalCount);
    }
    
    return dwError;
}

static
void
PolicyListFree(P_POLICY_LIST pList)
{
    if (pList)
    {
        LwFreeStringArray(pList->pszaPolicyList, pList->dwPolicyCount);
        LwFreeStringArray(pList->pszaLocalList, pList->dwLocalCount);

        pList->pszaPolicyList = pList->pszaLocalList = NULL;
        pList->dwPolicyCount = pList->dwLocalCount = 0;
    }
}

int
host_access_control_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    POLICY_LIST TemplateList = {0};
    POLICY_LIST HostGroupList = {0};
    POLICY_LIST AccessAllowedList = {0};
    DWORD i;
    HANDLE hReg = NULL;
    HANDLE hLsa = NULL;

    ParseArgs(argc, argv);
    
    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERROR(dwError);
    
    /* Ensure any pending Host Access Templates are resolved into Host Access Groups
     */
    dwError = LsaCheckUserInList(hLsa, "nobody", NULL);
    if (dwError == LW_ERROR_NO_SUCH_USER)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwRegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = PolicyListGet(
                hReg,
                AD_PROVIDER_HOST_ACCESS_REGKEY,
                AD_PROVIDER_REGKEY,
                "HostAccessTemplate",
                &TemplateList);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = PolicyListGet(
                hReg,
                AD_PROVIDER_HOST_ACCESS_REGKEY,
                AD_PROVIDER_REGKEY,
                "HostAccessGroup",
                &HostGroupList);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = PolicyListGet(
                hReg,
                AD_PROVIDER_POLICY_REGKEY,
                AD_PROVIDER_REGKEY,
                "RequireMembershipOf",
                &AccessAllowedList);
    BAIL_ON_LSA_ERROR(dwError);    

    if (TemplateList.pszaPolicyList)
    {
        if (TemplateList.dwPolicyCount == 0)
        {
            printf("Empty Cell Host Access Templates.\n");
        }
        else
        {
            printf("Cell Host Access Templates:\n");

            for (i=0; i<TemplateList.dwPolicyCount; i++)
            {
                printf("    %s\n", TemplateList.pszaPolicyList[i]);
            }
        }
    }
    else if (TemplateList.dwLocalCount)
    {
        printf("Local Host Access Templates:\n");

        for (i=0; i<TemplateList.dwLocalCount; i++)
        {
            printf("    %s\n", TemplateList.pszaLocalList[i]);
        }
    }
    else
    {
        printf("No Host Access Templates defined.\n");
    }

    if (HostGroupList.pszaPolicyList)
    {
        if (HostGroupList.dwPolicyCount == 0)
        {
            printf("\nNo Host Access Group Membership.\n");
        }
        else
        {
            printf("\nHost Access Group Membership:\n");

            for (i = 0; i < HostGroupList.dwPolicyCount; i++)
            {
                printf("    %s\n", HostGroupList.pszaPolicyList[i]);
            }
        }
    }
    else if (HostGroupList.dwLocalCount)
    {
        printf("\nLocal Host Access Groups:\n");

        for (i=0; i<HostGroupList.dwLocalCount; i++)
        {
            printf("    %s\n", HostGroupList.pszaLocalList[i]);
        }
    }

    if (AccessAllowedList.pszaPolicyList)
    {
        if (AccessAllowedList.dwPolicyCount == 0)
        {
            printf("\nEmpty Allow Logon Rights Policy.\n");
        }
        else
        {
            printf("\nAllow Logon Rights Policy:\n");

            for (i=0; i<AccessAllowedList.dwPolicyCount; i++)
            {
                printf("    %s\n", AccessAllowedList.pszaPolicyList[i]);
            }
        }
    }
    else if (AccessAllowedList.dwLocalCount)
    {
        printf("\nLocal Allow Logon Rights:\n");

        for (i=0; i<AccessAllowedList.dwLocalCount; i++)
        {
            printf("    %s\n", AccessAllowedList.pszaLocalList[i]);
        }
    }

cleanup:
    
    PolicyListFree(&TemplateList);
    PolicyListFree(&HostGroupList);
    PolicyListFree(&AccessAllowedList);

    if (hLsa != (HANDLE)NULL) {
        LsaCloseServer(hLsa);
    }

    if (hReg != (HANDLE)NULL) {
        RegCloseServer(hReg);
    }

    return (dwError);

error:

    fprintf(
        stderr,
        "Failed to query Host Access Control configuration.  Error code %u (%s).\n",
        dwError,
        LW_SAFE_LOG_STRING(LwWin32ExtErrorToDescription(dwError)));

    goto cleanup;
}

static
VOID
ParseArgs(
    int    argc,
    char*  argv[]
    )
{
    int iArg = 1;
    PSTR pszArg = NULL;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        if ((strcmp(pszArg, "--help") == 0) ||
            (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage(argv[0]);
            exit(0);
        }
        else
        {
            ShowUsage(argv[0]);
            exit(1);
        }
    } while (iArg < argc);
}

static
void
ShowUsage(const char *cmd)
{
    printf("Usage: %s\n", cmd);
}

