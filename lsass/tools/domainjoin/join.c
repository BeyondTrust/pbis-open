/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2010
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

#include "includes.h"

typedef enum DJ_ACTIONS
{
    DJ_JOIN,
    DJ_LEAVE,
} DJ_ACTIONS;


typedef struct LwDjArgs
{
    DJ_ACTIONS eAction;
    PSTR pszDcDomain;
    PSTR pszDnsDomain;
    PSTR pszOU;
    PSTR pszAdministrator;
    PSTR pszPassword;

    /* Host specific information */
    PSTR pszHostname;
    PSTR pszOsIdentifier;
    PSTR pszRelease;
    PSTR pszVersion;
} LwDjArgs, *PLwDjArgs;


void printError(PSTR msg, DWORD dwError)
{
    CHAR cError[256] = {0};
    PSTR pszError = (PSTR) cError;
    LwGetErrorString(dwError, cError, sizeof(cError));
    if (!strncmp(cError, "Unknown", 7))
    {
        pszError = (PSTR) LwWin32ExtErrorToName(dwError);
    }
    printf("%s (%d) %s\n", msg ? msg : "", dwError, pszError);
}


DWORD
LwDjJoinDomain(
    IN PSTR pszDnsDomain,
    IN PSTR pszDcDomain,
    IN PSTR pszOU,
    IN PSTR pszAdministrator,
    IN PSTR pszPassword,
    IN PSTR pszHostname,
    IN PSTR pszOsIdentifier,
    IN PSTR pszRelease,
    IN PSTR pszVersion)
{
    DWORD dwError = 0;
    HANDLE hLsa = NULL;

    dwError = LsaOpenServer(&hLsa);
    if (dwError)
    {
        return dwError;
    }

    dwError = LsaAdJoinDomain(
                 hLsa,
                 pszHostname,
                 pszDnsDomain,
                 pszDcDomain,
                 pszOU,
                 pszAdministrator,
                 pszPassword,
                 pszOsIdentifier ? pszOsIdentifier : "Unknown OS",
                 pszRelease ? pszRelease : "Unknown Release",
                 pszVersion ? pszVersion : "1",
                 0);
    if (dwError)
    {
        return dwError;
    }
    LsaCloseServer(hLsa);

    return 0;
}


DWORD
LwDjLeaveDomain(
    PCSTR username,
    PCSTR password)
{
    HANDLE hLsa = NULL;
    DWORD dwError = 0;

    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_DJ_ERROR(dwError);

    dwError = LsaAdLeaveDomain(hLsa, username, password);
    BAIL_ON_DJ_ERROR(dwError);

cleanup:
    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }
    return dwError;

error:
    goto cleanup;
}


void
LwDjUsage(
    PSTR pszProgName,
    PSTR pszMessage)
{
    if (pszMessage)
    {
        fprintf(stderr, "       ERROR: %s\n", pszMessage);
    }
    fprintf(stderr, "Usage: %s join [--ou OU] DOMAIN User [passwd] |\n"
            "       %s leave User [passwd]\n",
            pszProgName, pszProgName);
    exit(1);
}


DWORD
LwDjParseArgs(int argc, char *argv[], BOOLEAN bIsRoot, PLwDjArgs pDjArgs)
{
    DWORD dwError = 0;
    DWORD dwArgIndx = 1;
    PSTR pszArgv0 = NULL;
    PSTR pszPwdPrompt = NULL;
    int i = 0;
    struct utsname utsbuf;

    pszArgv0 = strrchr(argv[0], '/');
    if (pszArgv0)
    {
        pszArgv0 = strdup(pszArgv0 + 1);
    }
    else
    {
        pszArgv0 = strdup(argv[0]);
    }

    if (argc < 2)
    {
        LwDjUsage(pszArgv0, NULL);
    }

    /*
     * Parse action
     */
    if (dwArgIndx<argc && !strcasecmp(argv[dwArgIndx], "join"))
    {
        pDjArgs->eAction = DJ_JOIN;
    }
    else if (dwArgIndx<argc && !strcasecmp(argv[dwArgIndx], "leave"))
    {
        pDjArgs->eAction = DJ_LEAVE;
    }
    else if (dwArgIndx<argc && 
             (!strcasecmp(argv[dwArgIndx], "--help") ||
              !strcasecmp(argv[dwArgIndx], "-h")))
    {
        LwDjUsage(pszArgv0, NULL);
    }
    else
    {
        LwDjUsage(pszArgv0, "Action must be 'join' or 'leave'");
    }
    dwArgIndx++;
                      
    if (pDjArgs->eAction == DJ_JOIN)
    {
        if (dwArgIndx<argc && !strcmp("--ou", argv[dwArgIndx]))
        {
            dwArgIndx++;
            if (dwArgIndx<argc)
            {
                pDjArgs->pszOU = strdup((char *) argv[dwArgIndx++]);
            }
            else
            {
                LwDjUsage(pszArgv0, "--ou requires an argument");
            }
        }

        /*
         * Parse Windows Domain to join
         */
        if (dwArgIndx<argc)
        {
            dwError = LwRtlCStringAllocatePrintf(
                          &pDjArgs->pszDcDomain,
                          "%s",
                          argv[dwArgIndx]);
            BAIL_ON_DJ_ERROR(dwError);
            dwError = LwRtlCStringAllocatePrintf(
                          &pDjArgs->pszDnsDomain,
                          "%s",
                          argv[dwArgIndx]);
            BAIL_ON_DJ_ERROR(dwError);
            dwArgIndx++;
    
            for (i=0; pDjArgs->pszDcDomain[i]; i++)
            {
                pDjArgs->pszDcDomain[i] = toupper(pDjArgs->pszDcDomain[i]);
            }
        }
        else
        {
            LwDjUsage(pszArgv0,
                      "Name of Windows Domain to join must be specified");
        }
    }
    else
    {
        dwError = LWNetGetCurrentDomain(&pDjArgs->pszDcDomain);
        BAIL_ON_DJ_ERROR(dwError);
    }

    /*
     * Parse user account used for authentication
     */
    if (dwArgIndx<argc)
    {
        dwError = LwRtlCStringAllocatePrintf(
                      &pDjArgs->pszAdministrator,
                      "%s@%s",
                      argv[dwArgIndx++],
                      pDjArgs->pszDcDomain);
        BAIL_ON_DJ_ERROR(dwError);
    }
    else
    {
        LwDjUsage(pszArgv0,
                  "Domain administrator username must be specified");
    }

    /*
     * Parse password if present, otherwise prompt for one
     */
    if (dwArgIndx<argc)
    {
        dwError = LwRtlCStringAllocatePrintf(
                      &pDjArgs->pszPassword,
                      "%s",
                      argv[dwArgIndx++]);
        BAIL_ON_DJ_ERROR(dwError);
    }
    else if (bIsRoot)
    {
        dwError = LwRtlCStringAllocatePrintf(
                      &pszPwdPrompt,
                      "%s's password: ",
                      pDjArgs->pszAdministrator);
        BAIL_ON_DJ_ERROR(dwError);

        dwError = LwRtlCStringAllocatePrintf(
                      &pDjArgs->pszPassword,
                      "%s",
                      getpass(pszPwdPrompt));
        BAIL_ON_DJ_ERROR(dwError);
    }


    memset(&utsbuf, 0, sizeof(utsbuf));
    if (uname(&utsbuf) == -1)
    {
        dwError = LW_STATUS_INVALID_HANDLE;
        BAIL_ON_DJ_ERROR(dwError);
    }
    pDjArgs->pszHostname = strdup((char *) utsbuf.nodename);
    pDjArgs->pszOsIdentifier = strdup((char *) utsbuf.sysname);
    pDjArgs->pszRelease = strdup((char *) utsbuf.release);
    pDjArgs->pszVersion = strdup((char *) utsbuf.version);

cleanup:
    LWDJ_SAFE_FREE_STRING(pszPwdPrompt);
    

    return dwError;

error:
    goto cleanup;
}


int main(int argc, char *argv[])
{
    DWORD dwError = 0;
    PSTR pszCurrentDomain = NULL;
    LwDjArgs djArgs = {0};
    int rsts = 0;
    BOOLEAN bIsRoot = FALSE;

    if (getuid() == 0)
    {
        bIsRoot = TRUE;
    }
    dwError = LwDjParseArgs(argc, argv, bIsRoot, &djArgs);
    BAIL_ON_DJ_ERROR(dwError);

    if (!bIsRoot)
    {
        fprintf(stderr, "%s: Must be run as root\n", argv[0]);
        return 1;
    }

    switch (djArgs.eAction)
    {
        case DJ_JOIN:
            printf("Joining to AD Domain: %s\n"
                   "With Computer DNS Name: %s.%s\n\n",
                   djArgs.pszDcDomain,
                   djArgs.pszHostname,
                   djArgs.pszDnsDomain);

            dwError = LwDjJoinDomain(
                          djArgs.pszDnsDomain,
                          djArgs.pszDcDomain,
                          djArgs.pszOU,
                          djArgs.pszAdministrator,
                          djArgs.pszPassword,
                          djArgs.pszHostname,
                          djArgs.pszOsIdentifier,
                          djArgs.pszRelease,
                          djArgs.pszVersion);
            BAIL_ON_DJ_ERROR(dwError);

            dwError = LWNetGetCurrentDomain(&pszCurrentDomain);
            BAIL_ON_DJ_ERROR(dwError);

            printf("SUCCESS!\nYour computer is now joined to '%s'\n",
                   pszCurrentDomain);
                   
            break;

        case DJ_LEAVE:
            dwError = LWNetGetCurrentDomain(&pszCurrentDomain);
            BAIL_ON_DJ_ERROR(dwError);

            printf("Leaving AD Domain: %s\n", pszCurrentDomain);

            dwError = LwDjLeaveDomain(
                    djArgs.pszAdministrator,
                    djArgs.pszPassword);
            BAIL_ON_DJ_ERROR(dwError);
            printf("SUCCESS\n");
            break;
    }

cleanup:
    return rsts;

error:
    printError("LwDjJoinDomain", dwError);
    rsts = 1;
    goto cleanup;
}
