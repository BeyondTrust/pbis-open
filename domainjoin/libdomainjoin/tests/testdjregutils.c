/*
 * Copyright BeyondTrust Software
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
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU LESSER GENERAL
 * PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR
 * WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY
 * BEYONDTRUST, PLEASE CONTACT BEYONDTRUST AT beyondtrust.com/contact
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

