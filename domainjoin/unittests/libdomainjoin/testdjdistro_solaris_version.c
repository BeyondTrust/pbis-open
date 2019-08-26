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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 */
#include <unity.h>
#include <djdistroinfo.h>

/* forward declaration */
void baseTestCase(char * extended, const int expectedMajor, const int expectedMinor);


void testDJGetSolarisVersionReturnsFalseWhenNotSolaris() {
    const LwDistroInfo distroInfo = {
        .os = OS_AIX
    };

    TEST_ASSERT_FALSE(DJGetSolarisVersion(&distroInfo, NULL, NULL));
}

void testDJGetSolarisVersionReturnsFalseWhenNULLExtendedInformation() {
    const LwDistroInfo distroInfo = {
        .os = OS_SUNOS
    };

    TEST_ASSERT_FALSE(DJGetSolarisVersion(&distroInfo, NULL, NULL));
}

void testDJGetSolarisVersionReturnsFalseWhenNoExtendedInformation() {
    const LwDistroInfo distroInfo = {
        .os = OS_SUNOS,
        .extended = ""
    };

    TEST_ASSERT_FALSE(DJGetSolarisVersion(&distroInfo, NULL, NULL));
}

void testDJGetSolarisVersionWhenCantParseExtendedVersionInfo() {
    const LwDistroInfo distroInfo = {
        .os = OS_SUNOS,
        .extended = "garbage.garbage"
    };
    int major = 0;
    int minor = 0;

    TEST_ASSERT_FALSE(DJGetSolarisVersion(&distroInfo, &major, &minor));
}

void testDJGetSolarisVersionWhenExtendedVersionInfoPresent() {
    baseTestCase("11", 11, 0);
    baseTestCase("11.4", 11, 4);
    baseTestCase("11.4.5.0", 11, 4);
}

void baseTestCase(char * extended, const int expectedMajor, const int expectedMinor) {
    const LwDistroInfo distroInfo = {
        .os = OS_SUNOS,
        .extended = extended
    };
    int major = -1;
    int minor = -1;

    TEST_ASSERT_TRUE_MESSAGE(DJGetSolarisVersion(&distroInfo, &major, &minor), "Return code");
    TEST_ASSERT_EQUAL_INT_MESSAGE(expectedMajor, major, "Major version");
    TEST_ASSERT_EQUAL_INT_MESSAGE(expectedMinor, minor, "Minor version");
}

