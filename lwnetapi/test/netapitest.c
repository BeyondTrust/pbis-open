/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#include "includes.h"


static
PTEST
FindTest(
    PTEST pFirstTest,
    PCSTR pszName
    );

static
DWORD
StartTest(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
VOID
DisplayUsage(
    VOID
    );


VOID
AddTest(
    PTEST   pFt,
    PCSTR   pszName,
    test_fn Function
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PTEST pNextTest = NULL;
    PTEST pLastTest = NULL;

    if (pFt == NULL) return;

    pLastTest = pFt;
    while (pLastTest && pLastTest->pNext)
    { 
        pLastTest = pLastTest->pNext;
    }

    if (pLastTest->pszName && pLastTest->Function)
    {
        /* allocate the new test */
        dwError = LwAllocateMemory(sizeof(TEST),
                                   OUT_PPVOID(&pNextTest));
        BAIL_ON_WIN_ERROR(dwError);

        /* append the test to the list */
        pLastTest->pNext = pNextTest;
    }
    else
    {
        /* this is the very first test so use already
           allocated node */
        pNextTest = pLastTest;
    }

    /* init the new test */
    pNextTest->pszName    = pszName;
    pNextTest->Function   = Function;
    pNextTest->pNext      = NULL;

error:
    return;
}


static
PTEST
FindTest(
    PTEST pFirstTest,
    PCSTR pszName
    )
{
    PTEST pTest = pFirstTest;

    /* search through the tests and try to find
       a matching name */
    while (pTest)
    {
        if (strcasecmp(pTest->pszName, pszName) == 0)
        {
            return pTest;
        }

        pTest = pTest->pNext;
    }

    return NULL;
}


static
DWORD
StartTest(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    DWORD dwRet = 0;

    if (pTest == NULL) return -1;

    dwRet = pTest->Function(pTest,
                            pwszHostname,
                            pwszBindingString,
                            pCreds,
                            pOptions,
                            dwOptcount);
    printf("%s\n", (dwRet) ? "SUCCEEDED" : "FAILED");
    return dwRet;
}


static
VOID
DisplayUsage(
    VOID
    )
{
    printf("Usage: netapitest [-v] [-k] -h hostname\n"
           "\t[-u username]\n"
           "\t[-p password]\n"
           "\t[-d domain]\n"
           "\t[-w workstation]\n"
           "\t[-r principal]\n"
           "\t[-c creds cache]\n"
           "\t[-b binding string]\n"
           "\t[-o options]\n"
           "\ttestname\n");
    printf("hostname       - host to connect when performing a test\n");
    printf("username       - user name (NTLM authentication)\n");
    printf("password       - password (NTLM authentication)\n");
    printf("domain         - domain name (NTLM authentication)\n");
    printf("workstation    - workstation name (NTLM authentication)\n");
    printf("principal      - user principal name (Kerberos authentication)\n");
    printf("creds cache    - kerberos credentials cache path (Kerberos authentication)\n");
    printf("binding string - binding string specifying rpc endpoint (rpc client functions)\n");
    printf("options        - \"key1=value1,key2=value2\" options passed to the test\n");
}


extern char *optarg;
int verbose_mode;


int main(int argc, char *argv[])
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int i = 0;
    int opt = 0;
    int ret = 0;
    PSTR pszTestname = NULL;
    PSTR pszHost = NULL;
    PSTR pszOptionalArgs = NULL;
    PSTR pszUser = NULL;
    PSTR pszPass = NULL;
    PSTR pszDomain = NULL;
    PSTR pszWorkstation = NULL;
    PSTR pszPrincipal = NULL;
    PSTR pszCredsCache = NULL;
    PSTR pszBindingString = NULL;
    int krb5_auth = 1;
    PTEST pTests  = NULL;
    PTEST pRunTest = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszBindingString = NULL;
    LW_PIO_CREDS pCreds = NULL;
    CREDENTIALS Credentials;
    PPARAMETER pParams = NULL;
    DWORD dwParamsLen = 0;

    memset(&Credentials, 0, sizeof(Credentials));

    verbose_mode = false;

    while ((opt = getopt(argc, argv, "h:o:vu:p:d:w:r:c:kb:")) != -1) {
        switch (opt) {
        case 'h':
            pszHost = optarg;
            break;

        case 'o':
            pszOptionalArgs = optarg;
            break;

        case 'v':
            verbose_mode = true;
            break;

        case 'd':
            pszDomain = optarg;
            break;

        case 'w':
            pszWorkstation = optarg;
            break;

        case 'u':
            pszUser = optarg;
            break;

        case 'p':
            pszPass = optarg;
            break;

        case 'r':
            pszPrincipal = optarg;
            break;

        case 'c':
            pszCredsCache = optarg;
            break;

        case 'k':
            krb5_auth = 1;
            break;

        case 'b':
            pszBindingString = optarg;
            break;

        default:
            DisplayUsage();
            return -1;
        }
    }

    dwError = LwAllocateMemory(sizeof(pTests[0]),
                               OUT_PPVOID(&pTests));
    if (dwError)
    {
        printf("Failed to allocate tests\n");
        BAIL_ON_WIN_ERROR(dwError);
    }

    pTests->pszName   = NULL;
    pTests->Function  = NULL;
    pTests->pNext     = NULL;

    if (pszHost)
    {
        dwError = LwMbsToWc16s(pszHost, &pwszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (pszBindingString)
    {
        dwError = LwMbsToWc16s(pszBindingString, &pwszBindingString);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (pszUser && pszPass)
    {
        ntStatus = LwIoCreatePlainCredsA(pszUser,
                                         pszDomain,
                                         pszPass,
                                         &pCreds);
        if (ntStatus)
        {
            printf("Failed to create NTLM credentials\n");
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwError = LwMbsToWc16s(pszUser,
                               &Credentials.Ntlm.pwszUsername);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwMbsToWc16s(pszPass,
                               &Credentials.Ntlm.pwszPassword);
        BAIL_ON_WIN_ERROR(dwError);

        if (pszDomain)
        {
            dwError = LwMbsToWc16s(pszDomain,
                                   &Credentials.Ntlm.pwszDomain);
            BAIL_ON_WIN_ERROR(dwError);
        }

        if (pszWorkstation)
        {
            dwError = LwMbsToWc16s(pszWorkstation,
                                   &Credentials.Ntlm.pwszWorkstation);
            BAIL_ON_WIN_ERROR(dwError);
        }
    }

    if (pszPrincipal && pszCredsCache)
    {
        ntStatus = LwIoCreateKrb5CredsA(pszPrincipal,
                                        pszCredsCache,
                                        &pCreds);
        if (ntStatus)
        {
            printf("Failed to create KRB5 credentials\n");
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwError = LwMbsToWc16s(pszPrincipal,
                               &Credentials.Krb5.pwszPrincipal);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwMbsToWc16s(pszCredsCache,
                               &Credentials.Krb5.pwszCredsCache);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (pCreds)
    {
        ntStatus = LwIoSetThreadCreds(pCreds);
        if (ntStatus)
        {
            printf("Failed to set thread credentials\n");
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    pParams = get_optional_params(pszOptionalArgs, &dwParamsLen);
    if ((pParams != NULL && dwParamsLen == 0) ||
        (pParams == NULL && dwParamsLen != 0))
    {
        printf("Error while parsing optional parameters [%s]\n", pszOptionalArgs);
        goto error;
    }

    SetupNetApiTests(pTests);
    SetupLsaTests(pTests);
    SetupSamrTests(pTests);
    SetupNetlogonTests(pTests);
    SetupDsrTests(pTests);
    SetupWkssvcTests(pTests);
    
    for (i = 1; i < argc; i++)
    {
        pszTestname = argv[i];
        pRunTest = FindTest(pTests, pszTestname);

        if (pRunTest)
        {
            ret = StartTest(pRunTest,
                            pwszHostname,
                            pwszBindingString,
                            &Credentials,
                            pParams,
                            dwParamsLen);
            goto done;
        }
    }

    printf("No test name specified. Available tests:\n");
    pRunTest = pTests;
    while (pRunTest)
    {
        printf("%s\n", pRunTest->pszName);
        pRunTest = pRunTest->pNext;
    }
    printf("\n");
    

done:
error:
    LW_SAFE_FREE_MEMORY(pwszHostname);
    LW_SAFE_FREE_MEMORY(pwszBindingString);

    while (pTests)
    {
        PTEST pTest = pTests->pNext;

        LW_SAFE_FREE_MEMORY(pTests);
        pTests = pTest;
    }

    for (i = 0; i < dwParamsLen; i++)
    {
        LW_SAFE_FREE_MEMORY(pParams[i].key);
        LW_SAFE_FREE_MEMORY(pParams[i].val);
    }
    LW_SAFE_FREE_MEMORY(pParams);

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    return ret;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
