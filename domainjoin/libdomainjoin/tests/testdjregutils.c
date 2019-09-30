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

#include "lwerror.h"
#include "Mocklwreg.h"
#include "djregistry.h"

void testWhenRegServerOpenFailsDeleteValueFails()
{
    DWORD dwError = ERROR_SUCCESS;

    LwRegOpenServer_IgnoreAndReturn(ERROR_OUT_OF_PAPER);

    dwError = DeleteValueFromRegistry("path", "name");

    TEST_ASSERT_EQUAL_INT(ERROR_OUT_OF_PAPER, dwError);
}

void testWhenCalledWithNonExistentValueDeleteValueSucceeds()
{
    DWORD dwError = ERROR_SUCCESS;

    // we don't care about the reg open, root key ops for this test
    LwRegOpenServer_IgnoreAndReturn(ERROR_SUCCESS);
    LwRegOpenKeyExA_IgnoreAndReturn(ERROR_SUCCESS);

    LwRegDeleteKeyValueA_ExpectAndReturn(NULL, NULL, "path", "name", LWREG_ERROR_NO_SUCH_KEY_OR_VALUE);
    LwRegDeleteKeyValueA_IgnoreArg_hKey();

    LwRegCloseKey_IgnoreAndReturn(ERROR_SUCCESS);
    LwRegCloseServer_Ignore();

    dwError = DeleteValueFromRegistry("path", "name");

    TEST_ASSERT_EQUAL_INT(ERROR_SUCCESS, dwError);
}

void testWhenDeletingDefaultValueSucceeds()
{
    DWORD dwError = ERROR_SUCCESS;

    // we don't care about the reg open, root key ops for this test
    LwRegOpenServer_IgnoreAndReturn(ERROR_SUCCESS);
    LwRegOpenKeyExA_IgnoreAndReturn(ERROR_SUCCESS);

    LwRegDeleteKeyValueA_ExpectAndReturn(NULL, NULL, "path", "name", LWREG_ERROR_DELETE_DEFAULT_VALUE_NOT_ALLOWED);
    LwRegDeleteKeyValueA_IgnoreArg_hKey();

    LwRegCloseKey_IgnoreAndReturn(ERROR_SUCCESS);
    LwRegCloseServer_Ignore();

    dwError = DeleteValueFromRegistry("path", "name");

    TEST_ASSERT_EQUAL_INT(ERROR_SUCCESS, dwError);
}

void testWhenEncounteringAnErrorDeleteValueFails()
{
    DWORD dwError = LW_ERROR_SUCCESS;

    // we don't care about the reg open, root key ops for this test
    LwRegOpenServer_IgnoreAndReturn(LW_ERROR_SUCCESS);
    LwRegOpenKeyExA_IgnoreAndReturn(LW_ERROR_SUCCESS);

    LwRegDeleteKeyValueA_ExpectAndReturn(NULL, NULL, "path", "name", LWREG_ERROR_FAILED_DELETE_HAS_SUBKEY);
    LwRegDeleteKeyValueA_IgnoreArg_hKey();

    LwRegCloseKey_IgnoreAndReturn(LW_ERROR_SUCCESS);
    LwRegCloseServer_Ignore();

    dwError = DeleteValueFromRegistry("path", "name");

    TEST_ASSERT_EQUAL_INT(LWREG_ERROR_FAILED_DELETE_HAS_SUBKEY, dwError);
}

