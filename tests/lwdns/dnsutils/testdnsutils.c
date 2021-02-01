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


#include <stdlib.h>
#include <unity.h>

#include <lw/types.h>
#include <lw/winerror.h>

#ifndef HAVE_HPUX_OS

/*
 * these aren't in a published header, so are declared here
 */
DWORD
DNSInet6GetPtrAddress(
    PSTR pszInet6InputAddr,
    PSTR *ppszInet6OutputAddr
    );

DWORD
DNSInet6GetPtrZoneAddress(
    PSTR pszInet6InputAddr,
    PSTR *ppszInet6OutputAddr
    );

void testDNSInet6GetPtrAddressReturnsValidPtrRecord()
{
    DWORD dwError = 0;
    PSTR pszIP6Address = "0:0:0:0:0:0:0:1";
    PCSTR pszIP6PtrRecord = "1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa";
    PSTR pSzInet6OutputAddr = NULL;

    dwError = DNSInet6GetPtrAddress(pszIP6Address, &pSzInet6OutputAddr);

    TEST_ASSERT_EQUAL(0, dwError);
    TEST_ASSERT_EQUAL_STRING(pszIP6PtrRecord, pSzInet6OutputAddr);

    free(pSzInet6OutputAddr);
}

void testDNSInet6GetPtrAddressReturnsInvalidIpAddress()
{
    DWORD dwError = DNSInet6GetPtrAddress("0:0:0:0:", NULL);
    TEST_ASSERT_EQUAL(DNS_ERROR_INVALID_IP_ADDRESS, dwError);
}

void testDNSInet6GetPtrZoneAddress()
{
    DWORD dwError = 0;
    PSTR pszIP6Address = "0:0:0:0:0:0:0:1";
    PCSTR pszIP6PtrRecord = "0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa";
    PSTR pSzInet6OutputAddr = NULL;

    dwError = DNSInet6GetPtrZoneAddress(pszIP6Address, &pSzInet6OutputAddr);

    TEST_ASSERT_EQUAL(0, dwError);
    TEST_ASSERT_EQUAL_STRING(pszIP6PtrRecord, pSzInet6OutputAddr);

    free(pSzInet6OutputAddr);
}

void testDNSInet6GetPtrZoneAddressReturnsInvalidIpAddress()
{
    DWORD dwError = DNSInet6GetPtrZoneAddress("0:0:0:0:", NULL);
    TEST_ASSERT_EQUAL(DNS_ERROR_INVALID_IP_ADDRESS, dwError);
}
#endif
