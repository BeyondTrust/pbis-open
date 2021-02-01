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

