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
