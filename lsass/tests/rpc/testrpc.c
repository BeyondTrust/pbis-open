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


void
AddTest(
    struct test *ft,
    const char *name,
    test_fn function
    )
{
    DWORD dwError = ERROR_SUCCESS;
    struct test *nt = NULL;
    struct test *lt = NULL;

    if (ft == NULL) return;

    lt = ft;
    while (lt && lt->next) lt = lt->next;

    if (lt->name && lt->function)
    {
        /* allocate the new test */
        dwError = LwAllocateMemory(sizeof(struct test),
                                   OUT_PPVOID(&nt));
        BAIL_ON_WIN_ERROR(dwError);

        /* append the test to the list */
        lt->next = nt;
    }
    else
    {
        /* this is the very first test so use already
           allocated node */
        nt = lt;
    }

    /* init the new test */
    nt->name     = name;
    nt->function = function;
    nt->next     = NULL;

error:
    return;
}


struct test*
FindTest(
    struct test *ft,
    const char *name
    )
{
    struct test *t = ft;

    /* search through the tests and try to find
       a matching name */
    while (t)
    {
        if (strcasecmp(t->name, name) == 0)
        {
            return t;
        }

        t = t->next;
    }

    return NULL;
}


int
StartTest(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *username,
    const wchar16_t *password,
    struct parameter *options,
    int optcount
    )
{
    int ret = 0;

    if (t == NULL) return -1;

    ret = t->function(t,
                      hostname, username, password,
                      options, optcount);
    printf("%s\n", (ret) ? "SUCCEEDED" : "FAILED");
    return ret;
}



void display_usage()
{
    printf("Usage: testrpc [-v] -h hostname [-k] [-u username] [-p password]\n"
           "               [-d domain] [-w workstation] [-r principal] [-c creds cache]\n"
           "               [-o options] testname\n");
    printf("\thostname - host to connect when performing a test\n");
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
    char *testname = NULL;
    char *host = NULL;
    char *optional_args = NULL;
    char *user = NULL;
    char *pass = NULL;
    char *dom = NULL;
    char *princ = NULL;
    char *cache = NULL;
    int krb5_auth = 1;
    struct test *tests  = NULL;
    struct test *runtest = NULL;
    wchar16_t *hostname = NULL;
    wchar16_t *username = NULL;
    wchar16_t *password = NULL;
    LW_PIO_CREDS pCreds = NULL;
    struct parameter *params = NULL;
    int params_len = 0;

    verbose_mode = false;

    while ((opt = getopt(argc, argv, "h:o:vu:p:d:r:c:k")) != -1) {
        switch (opt) {
        case 'h':
            host = optarg;
            break;

        case 'o':
            optional_args = optarg;
            break;

        case 'v':
            verbose_mode = true;
            break;

        case 'd':
            dom = optarg;
            break;

        case 'u':
            user = optarg;
            break;

        case 'p':
            pass = optarg;
            break;

        case 'r':
            princ = optarg;
            break;

        case 'c':
            cache = optarg;
            break;

        case 'k':
            krb5_auth = 1;
            break;

        default:
            display_usage();
            return -1;
        }
    }

    dwError = LwAllocateMemory(sizeof(struct test),
                               OUT_PPVOID(&tests));
    if (dwError)
    {
        printf("Failed to allocate tests\n");
        BAIL_ON_WIN_ERROR(dwError);
    }

    tests->name     = NULL;
    tests->function = NULL;
    tests->next     = NULL;

    if (host)
    {
        dwError = LwMbsToWc16s(host, &hostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (user && pass)
    {
        ntStatus = LwIoCreatePlainCredsA(user,
                                         dom,
                                         pass,
                                         &pCreds);
        if (ntStatus)
        {
            printf("Failed to create NTLM credentials\n");
            BAIL_ON_NT_STATUS(ntStatus);
        }

    }

    if (princ && cache)
    {
        ntStatus = LwIoCreateKrb5CredsA(princ,
                                        cache,
                                        &pCreds);
        if (ntStatus)
        {
            printf("Failed to create KRB5 credentials\n");
            BAIL_ON_NT_STATUS(ntStatus);
        }
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

    params = get_optional_params(optional_args, &params_len);
    if ((params != NULL && params_len == 0) ||
        (params == NULL && params_len != 0))
    {
        printf("Error while parsing optional parameters [%s]\n", optional_args);
        goto error;
    }

    SetupLsaTests(tests);
    SetupSamrTests(tests);
    SetupNetlogonTests(tests);
    SetupDsrTests(tests);
    SetupWkssvcTests(tests);
    
    for (i = 1; i < argc; i++)
    {
        testname = argv[i];
        runtest = FindTest(tests, testname);

        if (runtest)
        {
            ret = StartTest(runtest, hostname, username, password,
                            params, params_len);
            goto done;
        }
    }

    printf("No test name specified. Available tests:\n");
    runtest = tests;
    while (runtest)
    {
        printf("%s\n", runtest->name);
        runtest = runtest->next;
    }
    printf("\n");
    

done:
error:
    LW_SAFE_FREE_MEMORY(hostname);

    while (tests)
    {
        struct test *t = tests->next;

        LW_SAFE_FREE_MEMORY(tests);
        tests = t;
    }

    LW_SAFE_FREE_MEMORY(username);
    LW_SAFE_FREE_MEMORY(password);

    for (i = 0; i < params_len; i++)
    {
        LW_SAFE_FREE_MEMORY(params[i].key);
        LW_SAFE_FREE_MEMORY(params[i].val);
    }
    LW_SAFE_FREE_MEMORY(params);

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    return 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
