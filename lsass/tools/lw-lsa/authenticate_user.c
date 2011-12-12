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
 *        authenticate_user.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Tool to test user authentications
 *
 * Authors: Kyle Stemen <kstemen@beyondtrust.com>
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsaclient.h"
#include "common.h"
#include <lsa/lsa.h>
#include <lwmem.h>
#include <lwerror.h>
#include <lw/rtllog.h>

#include <popt.h>
#include <termios.h>
#include <unistd.h>

DWORD
PromptForPassword(
    PSTR* ppPassword
    );

ssize_t
afgets(
    char **strp,
    FILE *stream
    );

ssize_t
afgets(
    char **strp,
    FILE *stream
    )
{
    ssize_t size = 0;
    ssize_t capacity = 0;
    ssize_t read_count = 0;
    char *buffer = NULL;
    char *newbuffer = NULL;
    do
    {
        capacity = size*2 + 10;
        newbuffer = (char *)realloc(buffer, capacity);
        if(newbuffer == NULL)
            goto error;
        buffer = newbuffer;
        read_count = read(fileno(stream), buffer + size, capacity - size - 1);
        if(read_count == -1)
            goto error;
        size += read_count;
        buffer[size] = '\0';
    }
    while(size == capacity - 1 && buffer[size-1] != '\n');

    *strp = buffer;
    return size;
error:
    if (buffer != NULL)
    {
        free(buffer);
    }
    return -1;
}

static
VOID
ShowUsage(
    PCSTR pszProgramName,
    BOOLEAN bFull
    )
{
    printf(
        "Usage: %s --user <name> --domain <name> [ --password <pass> ] [ --provider name ]\n",
        Basename(pszProgramName));
    
    if (bFull)
    {
        printf(
            "\n"
            "    --user                  User name to authenticate with\n"
            "    --domain                User's domain\n"
            "    --password              User's password (prompted if not passed on commandline)\n"
            "\n");
    }
}
    
DWORD
PromptForPassword(
    PSTR* ppPassword
    )
{
    DWORD dwError = 0;
    PSTR pPassword = NULL;
    struct termios old = {0}, new = {0};
    FILE *tty = NULL;
    BOOLEAN echoCleared = FALSE;
    ssize_t len = 0;
    
    tty = fopen("/dev/tty", "r+");
    if (tty == NULL)
    {
        dwError = LwMapErrnoToLwError(errno);
        goto error;
    }

    // Turn off echo
    tcgetattr(fileno(tty), &old);
    memcpy(&new, &old, sizeof(old));
    new.c_lflag &= ~(ECHO);
    tcsetattr(fileno(tty), TCSANOW, &new);
    echoCleared = TRUE;

    fprintf(tty, "%s", "Password: ");
    fflush(tty);

    len = afgets(&pPassword, tty);
    if (len < 0)
    {
        dwError = ERROR_BROKEN_PIPE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (len > 0 && pPassword[len - 1] == '\n')
    {
        pPassword[len - 1] = 0;
    }

    fprintf(tty, "\n");
    *ppPassword = pPassword;

cleanup:
    if (echoCleared)
    {
        tcsetattr(fileno(tty), TCSANOW, &old);
    }
    if (tty)
    {
        fclose(tty);
    }
    return dwError;

error:
    LW_SAFE_FREE_STRING(pPassword);
    *ppPassword = NULL;
    goto cleanup;
}

static 
void
PrintDataBlob(
    PLW_LSA_DATA_BLOB pBlob
    )
{
    DWORD index = 0;

    for (index = 0; index < pBlob->dwLen; index++)
    {
        if (index % 16 == 0)
        {
            if (index > 0)
            {
                printf("\n");
            }
            printf("\t\t");
        }
        printf("%3x", pBlob->pData[index]);
    }
    if (index)
    {
        printf("\n");
    }
}

static
void
PrintAuthUserInfo(
    PLSA_AUTH_USER_INFO pInfo
    )
{
    DWORD index = 0;

    printf("\tUserFlags: 0x%x\n", pInfo->dwUserFlags);
    printf("\tAccount: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszAccount));
    printf("\tUserPrincipalName: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszUserPrincipalName));
    printf("\tFullName: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszFullName));
    printf("\tDomain: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszDomain));
    printf("\tDnsDomain: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszDnsDomain));
    printf("\tAcctFlags: 0x%x\n", pInfo->dwAcctFlags);
    printf("\tSession Key:\n");
    PrintDataBlob(pInfo->pSessionKey);
    printf("\tLanman Session Key:\n");
    PrintDataBlob(pInfo->pLmSessionKey);
    printf("\tLogonCount: %d\n", pInfo->LogonCount);
    printf("\tBadPasswordCount: %d\n", pInfo->BadPasswordCount);
    printf("\tLogonTime: %lld\n", (long long int) pInfo->LogonTime);
    printf("\tLogoffTime: %lld\n", (long long int) pInfo->LogoffTime);
    printf("\tKickoffTime: %lld\n", (long long int) pInfo->KickoffTime);
    printf("\tLastPasswordChange: %lld\n", (long long int) pInfo->LastPasswordChange);
    printf("\tCanChangePassword: %lld\n", (long long int) pInfo->CanChangePassword);
    printf("\tMustChangePassword: %lld\n", (long long int) pInfo->MustChangePassword);
    printf("\tLogonServer: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszLogonServer));
    printf("\tLogonScript: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszLogonScript));
    printf("\tProfilePath: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszProfilePath));
    printf("\tHomeDirectory: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszHomeDirectory));
    printf("\tHomeDrive: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszHomeDrive));
    printf("\tDomainSid: %s\n", LW_RTL_LOG_SAFE_STRING(pInfo->pszDomainSid));
    printf("\tUserRid: %d\n", pInfo->dwUserRid);
    printf("\tPrimaryGroupRid: %d\n", pInfo->dwPrimaryGroupRid);

    printf("\tNumRids: %d\n", pInfo->dwNumRids);
    for (index = 0; index < pInfo->dwNumRids; index++)
    {
        printf("\tRid %d: %d\n", index, pInfo->pRidAttribList[index].Rid);
        printf("\tRid %d attribute: 0x%x\n", index, pInfo->pRidAttribList[index].dwAttrib);
    }

    printf("\tNumSids: %d\n", pInfo->dwNumSids);
    for (index = 0; index < pInfo->dwNumSids; index++)
    {
        printf("\tSid %d: %s\n", index, LW_RTL_LOG_SAFE_STRING(pInfo->pSidAttribList[index].pszSid));
        printf("\tSid %d attribute: 0x%x\n", index, pInfo->pSidAttribList[index].dwAttrib);
    }
}

int
AuthenticateUserMain(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;
    PSTR pPasswordAllocated = NULL;
    PSTR pProvider = NULL;
    int result = 0;
    LSA_AUTH_USER_PARAMS lsaParams = {0};
    poptContext context = NULL;
    BOOLEAN showUsage = FALSE;
    HANDLE hLsa = (HANDLE)NULL;
    PLSA_AUTH_USER_INFO pUserInfo = NULL;

    struct poptOption options[] =
    {
        { "user", '\0', POPT_ARG_STRING, &lsaParams.pszAccountName, 0, NULL, NULL },
        { "domain", '\0', POPT_ARG_STRING, &lsaParams.pszDomain, 0, NULL, NULL },
        { "password", '\0', POPT_ARG_STRING, &lsaParams.pass.clear.pszPassword, 0, NULL, NULL },
        { "provider", '\0', POPT_ARG_STRING, &pProvider, 0, NULL, NULL },
        POPT_TABLEEND
    };

    lsaParams.AuthType = LSA_AUTH_PLAINTEXT;

    context = poptGetContext(
                NULL,
                argc,
                (const char **)ppszArgv,
                options,
                0);

    if (context == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }

    while ((result = poptGetNextOpt(context)) >= 0);

    if (result != -1)
    {
        showUsage = TRUE;
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (lsaParams.pszAccountName == NULL)
    {
        fprintf(stderr, "The user name must be set on the command line\n");

        showUsage = TRUE;
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (lsaParams.pass.clear.pszPassword == NULL)
    {
        dwError = PromptForPassword(&pPasswordAllocated);
        BAIL_ON_LSA_ERROR(dwError);

        lsaParams.pass.clear.pszPassword = pPasswordAllocated;
    }

    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAuthenticateUserEx(
                hLsa,
                pProvider,
                &lsaParams,
                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    printf("Success\n");

    if (pUserInfo)
    {
        printf("\n");
        printf("Auth User Info:\n");
        PrintAuthUserInfo(pUserInfo);
    }

error:
    LW_SAFE_FREE_STRING(pPasswordAllocated);
    if (hLsa != NULL)
    {
        LsaCloseServer(hLsa);
    }
    if (pUserInfo != NULL)
    {
        LsaFreeAuthUserInfo(&pUserInfo);
    }
    if (context != NULL)
    {
        poptFreeContext(context);
    }
    if (dwError)
    {
        printf("Error: %s (0x%x)\n", LwWin32ExtErrorToName(dwError), dwError);

        if (showUsage)
        {
            ShowUsage(ppszArgv[0], TRUE);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}
