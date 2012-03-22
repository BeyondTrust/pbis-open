/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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

/*
 * Authors: Wei Fu (wfu@likewise.com)
 */

#include <moonunit/moonunit.h>
#include <lw/base.h>

#define MU_ASSERT_STATUS_SUCCESS(status) \
    MU_ASSERT(STATUS_SUCCESS == (status))

static BYTE buffer[] = {
        0x01, 0x00, 0x04, 0x84, 0x14, 0x00, 0x00, 0x00,
        0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x4c, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
        0x06, 0x6a, 0xeb, 0x18, 0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x0d, 0x07, 0x00, 0x00,
        0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x06, 0x6a, 0xeb, 0x18,
        0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x01, 0x02, 0x00, 0x00, 0x02, 0x00, 0xcc, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x00, 0x03, 0x24, 0x00, 0xff, 0x01, 0x1f, 0x00, 0x01, 0x05, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x06, 0x6a, 0xeb, 0x18, 0x5a, 0xbb, 0xfe, 0x46,
        0xa8, 0x7f, 0x47, 0x83, 0xed, 0x06, 0x00, 0x00, 0x00, 0x10, 0x24, 0x00, 0xff, 0x01, 0x1f, 0x00,
        0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x06, 0x6a, 0xeb, 0x18,
        0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x0d, 0x07, 0x00, 0x00, 0x00, 0x1b, 0x24, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
        0x06, 0x6a, 0xeb, 0x18, 0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x0d, 0x07, 0x00, 0x00,
        0x00, 0x10, 0x14, 0x00, 0xff, 0x01, 0x1f, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x12, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x14, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x05, 0x12, 0x00, 0x00, 0x00, 0x00, 0x10, 0x18, 0x00, 0xff, 0x01, 0x1f, 0x00,
        0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00,
        0x00, 0x1b, 0x18, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00
};

static const char sddl[] = "O:SYG:S-1-5-32-544D:(A;;RCKRKXNW;;;WD)(A;;RCSDWDWOKAKRKWKXNRNWNX;;;SY)";
static const char sddl_converted[] = "O:SYG:BAD:(A;;RCKRKXNW;;;WD)(A;;RCSDWDWOKAKRKWKXNRNWNX;;;SY)";


MU_TEST(Sddl, 0000_SddlConversion)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE relativeSd = (PSECURITY_DESCRIPTOR_RELATIVE) (PBYTE) buffer;
    ULONG relativeSdLength = 0;
    PSTR pszStringSecurityDescriptor = NULL;
    PSTR pszStringSecurityDescriptor1 = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor = NULL;
    ULONG pSdLength = 0;
    SECURITY_INFORMATION secInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);

    status = RtlAllocateSddlCStringFromSecurityDescriptor(
                &pszStringSecurityDescriptor,
                relativeSd,
                SDDL_REVISION_1,
                secInfoAll);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_INFO("The converted sddl string is %s ", pszStringSecurityDescriptor);

    status = RtlAllocateSecurityDescriptorFromSddlCString(
                 &pSecurityDescriptor,
                 &pSdLength,
                 pszStringSecurityDescriptor,
                 SDDL_REVISION_1
                 );
    MU_ASSERT_STATUS_SUCCESS(status);

    // Compare origianl sd and the sd obtained from the sddl string
    relativeSdLength = sizeof(buffer);
    MU_ASSERT(relativeSdLength == pSdLength);
    MU_ASSERT(LwRtlEqualMemory(relativeSd, pSecurityDescriptor, pSdLength));

    MU_INFO("Security Descriptor Buffer is equal.");

    status = RtlAllocateSddlCStringFromSecurityDescriptor(
                &pszStringSecurityDescriptor1,
                pSecurityDescriptor,
                SDDL_REVISION_1,
                secInfoAll);
    MU_ASSERT_STATUS_SUCCESS(status);

    // Compare originaly converted sddl string and the one that is converted from
    // the SD we convert to
    MU_ASSERT(!strcmp(pszStringSecurityDescriptor1, pszStringSecurityDescriptor));

    MU_INFO("Sddl string is equal.");

    RTL_FREE(&pszStringSecurityDescriptor);
    RTL_FREE(&pszStringSecurityDescriptor1);
    RTL_FREE(&pSecurityDescriptor);
}


MU_TEST(Sddl, 0001_SddlConversion)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszStringSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor1 = NULL;
    ULONG pSdLength = 0;
    ULONG pSdLength1 = 0;
    SECURITY_INFORMATION secInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);

    status = RtlAllocateSecurityDescriptorFromSddlCString(
                 &pSecurityDescriptor,
                 &pSdLength,
                 sddl,
                 SDDL_REVISION_1
                 );
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateSddlCStringFromSecurityDescriptor(
                &pszStringSecurityDescriptor,
                pSecurityDescriptor,
                SDDL_REVISION_1,
                secInfoAll);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_INFO("The converted sddl string is %s ", pszStringSecurityDescriptor);
    // Compare originaly converted sddl string and the one that is converted from
    // the SD we convert to
    MU_ASSERT(!strcmp(pszStringSecurityDescriptor, sddl_converted));

    MU_INFO("Sddl string is equal.");

    status = RtlAllocateSecurityDescriptorFromSddlCString(
                 &pSecurityDescriptor1,
                 &pSdLength1,
                 pszStringSecurityDescriptor,
                 SDDL_REVISION_1
                 );
    MU_ASSERT_STATUS_SUCCESS(status);

    // Compare sd buffer
    MU_ASSERT(pSdLength == pSdLength1);
    MU_ASSERT(LwRtlEqualMemory(pSecurityDescriptor, pSecurityDescriptor1, pSdLength));

    RTL_FREE(&pszStringSecurityDescriptor);
    RTL_FREE(&pSecurityDescriptor);
    RTL_FREE(&pSecurityDescriptor1);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
