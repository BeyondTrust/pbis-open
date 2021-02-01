/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include <unity.h>
#include <string.h>
#include "pbps-int.h"
#include "MockDJGetComputerName.h"

const char * const HOSTNAME = "narwhal";

char *computerName = NULL;
PbpsApi_t *pApi =  NULL;
PbpsApiManagedAccount_t *pAccount = NULL;

struct account_info {
    const char * const systemName;
    const char * const domainName;
    const char * const accountName;
};

void setUp(void) {
    computerName = strdup(HOSTNAME);
    PbpsApiInitialize(&pApi);
    pAccount = NULL;

    DJGetComputerName_ExpectAnyArgsAndReturn(ERROR_SUCCESS);
    DJGetComputerName_ReturnThruPtr_ppszComputerName(&computerName);
}

void tearDown(void) {
    PbpsApiManagedAccountFree(pAccount);
    PbpsApiRelease(pApi);
}

DWORD GetManagedAccountJSON(const struct account_info values[], const int count, char **json) {
    // these are abbreviated, containing only the fields we explicity parse
    char const * const domainNameFormat   = "{\"PlatformID\":4,\"SystemId\":5,\"SystemName\":\"%s\",\"DomainName\":\"%s\",\"AccountId\":5,\"AccountName\":\"%s\"}";
    char const * const noDomainNameFormat = "{\"PlatformID\":4,\"SystemId\":5,\"SystemName\":\"%s\",\"DomainName\":null,\"AccountId\":5,\"AccountName\":\"%s\"}";

    DWORD dwError = ERROR_SUCCESS;
    PSTR pTmpJsonBuffer = NULL;
    PSTR pTmpJsonEntry = NULL;
    size_t bufsize = 0;
    size_t bufmultiple = 256;
    int i = 0;

    for(i = 0; i < count; i++)
    {
        if (values[i].domainName) {
            dwError = LwAllocateStringPrintf(&pTmpJsonEntry, domainNameFormat, values[i].systemName, values[i].domainName, values[i].accountName);
        } else {
            dwError = LwAllocateStringPrintf(&pTmpJsonEntry, noDomainNameFormat, values[i].systemName, values[i].accountName);
        }

        if (dwError) {
            return dwError;
        }

        if (pTmpJsonBuffer) {
            LwRtlCStringStrcatGrow(&pTmpJsonBuffer, &bufsize, bufmultiple, ",");
        }
        LwRtlCStringStrcatGrow(&pTmpJsonBuffer, &bufsize, bufmultiple, pTmpJsonEntry);

        LW_SAFE_FREE_STRING(pTmpJsonEntry);
    }

    *json = pTmpJsonBuffer;

    return dwError;
}

void testPbpsApiGetJoinAccountReturnsNoSuchUserWhenThereAreNoManagedAccounts() {
    DWORD dwError = ERROR_SUCCESS;

    dwError = PbpsApiGetJoinAccount(
           pApi,
           "user1",
           &pAccount);
    TEST_ASSERT_EQUAL(LW_ERROR_NO_SUCH_USER, dwError);
    TEST_ASSERT_NULL(pAccount);
}

void testPbpsApiGetJoinAccountReturnsNoSuchUserWhenThereIsNoMatch() {
    DWORD dwError = ERROR_SUCCESS;
    char *json = NULL;

    struct account_info const accounts[] = {
        {
            .systemName = "host1",
            .domainName = "domain1",
            .accountName = "user1"
        }
    };

    dwError = GetManagedAccountJSON(accounts, 1, &json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiManagedAccountsParse(json, &(pApi->session.pManagedAccountList));
    LW_SAFE_FREE_STRING(json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiGetJoinAccount(
           pApi,
           "user2",
           &pAccount);
    TEST_ASSERT_EQUAL(LW_ERROR_NO_SUCH_USER, dwError);
    TEST_ASSERT_NULL(pAccount);
}

void testPbpsApiGetJoinAccountReturnsAccountForThisHost() {
    DWORD dwError = ERROR_SUCCESS;
    char *json = NULL;

    struct account_info const accounts[] = {
        {
            .systemName = "host1",
            .domainName = "domain1",
            .accountName = "user1"
        },
        {
            .systemName = HOSTNAME,
            .domainName = "domain1",
            .accountName = "user1"
        }
    };

    dwError = GetManagedAccountJSON(accounts, 2, &json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiManagedAccountsParse(json, &(pApi->session.pManagedAccountList));
    LW_SAFE_FREE_STRING(json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiGetJoinAccount(
           pApi,
           "user1",
           &pAccount);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);
    TEST_ASSERT_EQUAL_STRING(HOSTNAME, pAccount->pszSystemName);
    TEST_ASSERT_EQUAL_STRING("user1", pAccount->pszAccountName);
}

void testPbpsApiGetJoinAccountReturnsAccountMatchingDomain() {
    DWORD dwError = ERROR_SUCCESS;
    char *json = NULL;

    struct account_info const accounts[] = {
        {
            .systemName = "host1",
            .domainName = "domain1",
            .accountName = "user1"
        },
        {
            .systemName = "host2",
            .domainName = "domain3",
            .accountName = "user1"
        }
    };

    dwError = GetManagedAccountJSON(accounts, 2, &json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiManagedAccountsParse(json, &(pApi->session.pManagedAccountList));
    LW_SAFE_FREE_STRING(json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiGetJoinAccount(
           pApi,
           "domain3\\\\user1",
           &pAccount);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);
    TEST_ASSERT_EQUAL_STRING("host2", pAccount->pszSystemName);
    TEST_ASSERT_EQUAL_STRING("domain3", pAccount->pszDomainName);
    TEST_ASSERT_EQUAL_STRING("user1", pAccount->pszAccountName);
}

void testPbpsApiGetJoinAccountIgnoresNullDomainName() {
    DWORD dwError = ERROR_SUCCESS;
    char *json = NULL;

    struct account_info const accounts[] = {
        {
            .systemName = "host2",
            .domainName = NULL,
            .accountName = "user1"
        },
        {
            .systemName = HOSTNAME,
            .domainName = NULL,
            .accountName = "user1"
        },
        {
            .systemName = "host2",
            .domainName = "domain3",
            .accountName = "user1"
        }
    };

    dwError = GetManagedAccountJSON(accounts, 3, &json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiManagedAccountsParse(json, &(pApi->session.pManagedAccountList));
    LW_SAFE_FREE_STRING(json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiGetJoinAccount(
           pApi,
           "user1",
           &pAccount);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);
    TEST_ASSERT_EQUAL_STRING(HOSTNAME, pAccount->pszSystemName);
    TEST_ASSERT_EQUAL_STRING("user1", pAccount->pszAccountName);
}

void testPbpsApiGetJoinAccountLookupsDomainNameAccounts() {
    DWORD dwError = ERROR_SUCCESS;
    char *json = NULL;

    struct account_info const accounts[] = {
        {
            .systemName = "host2",
            .domainName = NULL,
            .accountName = "user1"
        },
        {
            .systemName = HOSTNAME,
            .domainName = "null.com",
            .accountName = "user1"
        }
    };

    dwError = GetManagedAccountJSON(accounts, 2, &json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiManagedAccountsParse(json, &(pApi->session.pManagedAccountList));
    LW_SAFE_FREE_STRING(json);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);

    dwError = PbpsApiGetJoinAccount(
           pApi,
           "null.com\\\\user1",
           &pAccount);
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, dwError);
    TEST_ASSERT_EQUAL_STRING(HOSTNAME, pAccount->pszSystemName);
    TEST_ASSERT_EQUAL_STRING("user1", pAccount->pszAccountName);
    TEST_ASSERT_EQUAL_STRING("null.com", pAccount->pszDomainName);
}

