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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include <moonunit/moonunit.h>
#include <lw/base.h>

#define MU_ASSERT_STATUS_SUCCESS(status) \
    MU_ASSERT(STATUS_SUCCESS == (status))

MU_TEST(Security, 0000_SidInitialize)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    ULONG sidSize = 0;
    SID_IDENTIFIER_AUTHORITY ntAuthority = { SECURITY_NT_AUTHORITY };
    const UCHAR ntSubAuthorityCount = 4;

    sidSize = RtlLengthRequiredSid(ntSubAuthorityCount);
    MU_ASSERT(sidSize >= SID_MIN_SIZE);

    status = RTL_ALLOCATE(&sid, SID, sidSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlInitializeSid(sid, &ntAuthority, ntSubAuthorityCount);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_ASSERT(RtlValidSid(sid));
    MU_ASSERT(RtlLengthSid(sid) == sidSize);
    MU_ASSERT(RtlEqualMemory(&sid->IdentifierAuthority, &ntAuthority, sizeof(ntAuthority)));

    RTL_FREE(&sid);
}

MU_TEST(Security, 0001_SidFromCString)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR initialSidString = "S-1-5-21-100-200-300-500";
    PSID sid = NULL;
    PSTR parsedSidString = NULL;

    status = RtlAllocateSidFromCString(&sid, initialSidString);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateCStringFromSid(&parsedSidString, sid);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_ASSERT(RtlCStringIsEqual(initialSidString, parsedSidString, FALSE));

    RTL_FREE(&parsedSidString);
    RTL_FREE(&sid);
}

MU_TEST(Security, 0001_SidFromWC16String)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR initialSidStringA = "S-1-5-21-100-200-300-500";
    PWSTR initialSidString = NULL;
    PSID sid = NULL;
    PWSTR parsedSidString = NULL;

    status = RtlWC16StringAllocateFromCString(&initialSidString, initialSidStringA);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateSidFromWC16String(&sid, initialSidString);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateWC16StringFromSid(&parsedSidString, sid);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_ASSERT(RtlWC16StringIsEqual(initialSidString, parsedSidString, FALSE));

    RTL_FREE(&parsedSidString);
    RTL_FREE(&sid);
    RTL_FREE(&initialSidString);
}

MU_TEST(Security, 0002_SidChange)
{
    MU_SKIP("Not implemented");
    // RtlSidAllocate
    // RtlSidCopyAlloc
    // SidCopy
    // RtlSidAppendRid
}

static
VOID
DumpSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_CONTROL control = 0;
    UCHAR revision = 0;
    PSID sid = NULL;
    BOOLEAN isDefaulted = FALSE;
    PSTR sidString = NULL;
    BOOLEAN isPresent = FALSE;
    PACL acl = NULL;
    ULONG aceIndex = 0;

    status = RtlGetSecurityDescriptorControl(
                    SecurityDescriptor,
                    &control,
                    &revision);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_INFO("control = 0x%04x, revision = %u", control, revision);

    status = RtlGetOwnerSecurityDescriptor(
                    SecurityDescriptor,
                    &sid,
                    &isDefaulted);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateCStringFromSid(&sidString, sid);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_INFO("OwnerSid = %s (%c)", sidString, isDefaulted ? 'Y' : 'N');

    status = RtlGetGroupSecurityDescriptor(
                    SecurityDescriptor,
                    &sid,
                    &isDefaulted);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateCStringFromSid(&sidString, sid);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_INFO("GroupSid = %s (%c)", sidString, isDefaulted ? 'Y' : 'N');

    status = RtlGetDaclSecurityDescriptor(
                    SecurityDescriptor,
                    &isPresent,
                    &acl,
                    &isDefaulted);
    MU_ASSERT_STATUS_SUCCESS(status);

    // ISSUE-Hmm...need to make public accessor for ace count since
    // we made ACL extra private...

    for (;;)
    {
        PACCESS_ALLOWED_ACE ace = NULL;
        status = RtlGetAce(acl, aceIndex++, OUT_PPVOID(&ace));
        if (STATUS_INVALID_PARAMETER == status)
        {
            break;
        }
        MU_ASSERT_STATUS_SUCCESS(status);

        MU_INFO("Dacl ACE %u - (Type = %u, Flags = 0x%02x, Size = %u)",
                aceIndex,
                ace->Header.AceType,
                ace->Header.AceFlags,
                ace->Header.AceSize);

        switch (ace->Header.AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
            case SYSTEM_ALARM_ACE_TYPE:
                status = RtlAllocateCStringFromSid(&sidString, (PSID) &ace->SidStart);
                MU_ASSERT_STATUS_SUCCESS(status);

                MU_INFO("    0x%08x %s", ace->Mask, sidString);
                break;
        }
    }

    RTL_FREE(&sidString);
}

static
VOID
DumpTokenInfo(
    IN PACCESS_TOKEN Token,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE tokenInfo = NULL;
    ULONG size = 0;
    ULONG savedSize = 0;

    status = RtlQueryAccessTokenInformation(
                    Token,
                    TokenInformationClass,
                    tokenInfo,
                    size,
                    &size);
    MU_ASSERT(STATUS_BUFFER_TOO_SMALL == status);

    MU_INFO("size = %u", size);
    savedSize = size;

    status = RTL_ALLOCATE(&tokenInfo, BYTE, size);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlQueryAccessTokenInformation(
                    Token,
                    TokenInformationClass,
                    tokenInfo,
                    size,
                    &size);
    MU_INFO("status = 0x%08x", status);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_ASSERT(size == savedSize);

    switch (TokenInformationClass)
    {
        case TokenUser:
        {
            PTOKEN_USER tokenUser = (PTOKEN_USER) tokenInfo;
            PSTR sidString = NULL;

            status = RtlAllocateCStringFromSid(&sidString, tokenUser->User.Sid);
            MU_ASSERT_STATUS_SUCCESS(status);

            MU_INFO("User = %s", sidString);

            RTL_FREE(&sidString);

            break;
        }
        case TokenGroups:
        {
            PTOKEN_GROUPS tokenGroups = (PTOKEN_GROUPS) tokenInfo;
            ULONG i = 0;
            PSTR sidString = NULL;

            MU_INFO("GroupCount = %u", tokenGroups->GroupCount);

            for (i = 0; i < tokenGroups->GroupCount; i++)
            {
                RTL_FREE(&sidString);

                status = RtlAllocateCStringFromSid(&sidString, tokenGroups->Groups[i].Sid);
                MU_ASSERT_STATUS_SUCCESS(status);

                MU_INFO("Groups[%u] = (0x%08x, %s)", i, tokenGroups->Groups[i].Attributes, sidString);
            }

            RTL_FREE(&sidString);

            break;
        }
    }

    RTL_FREE(&tokenInfo);
}

static
VOID
DumpToken(
    IN PACCESS_TOKEN Token
    )
{
    DumpTokenInfo(Token, TokenUser);
    DumpTokenInfo(Token, TokenGroups);
}

MU_TEST(Security, 0003_AccessCheck)
{
    NTSTATUS status = STATUS_SUCCESS;
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
    PSID sid = NULL;
    TOKEN_USER tokenUser = { { 0 } };
    union {
        TOKEN_GROUPS tokenGroups;
        struct {
            ULONG GroupCount;
            SID_AND_ATTRIBUTES Groups[10];
        };
    } tokenGroupsUnion = { .tokenGroups = { 0 } };
    TOKEN_PRIVILEGES tokenPrivileges = { 0 };
    TOKEN_OWNER tokenOwner = { 0 };
    TOKEN_PRIMARY_GROUP tokenPrimaryGroup = { 0 };
    TOKEN_DEFAULT_DACL tokenDefaultDacl = { 0 };
    PACCESS_TOKEN token = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE relativeSd = (PSECURITY_DESCRIPTOR_RELATIVE) (PBYTE) buffer;
    ULONG relativeSdLength = sizeof(buffer);
    PSECURITY_DESCRIPTOR_ABSOLUTE sd = NULL;
    PACL dacl = NULL;
    PACL sacl = NULL;
    PSID owner = NULL;
    PSID primaryGroup = NULL;
    ULONG sdSize = 0;
    ULONG daclSize = 0;
    ULONG saclSize = 0;
    ULONG ownerSize = 0;
    ULONG primaryGroupSize = 0;
    GENERIC_MAPPING mapping = {
        .GenericRead = 0,
        .GenericWrite = 0,
        .GenericExecute = 0,
        .GenericAll = 0,
    };
    ACCESS_MASK granted = 0;
    BOOLEAN isGranted = FALSE;

    // SID present in an ACE
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-1805");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenUser.User.Sid = sid;

    // Bogus SID
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-12345678");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Sid = sid;
    SetFlag(tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Attributes, SE_GROUP_ENABLED);
    tokenGroupsUnion.GroupCount++;

    // Bogus SID
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-87654321");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Sid = sid;
    tokenGroupsUnion.GroupCount++;

    // SID present in an ACE
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-1773");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Sid = sid;
    SetFlag(tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Attributes, SE_GROUP_ENABLED);
    tokenGroupsUnion.GroupCount++;

    status = RtlCreateAccessToken(
                    &token,
                    &tokenUser,
                    &tokenGroupsUnion.tokenGroups,
                    &tokenPrivileges,
                    &tokenOwner,
                    &tokenPrimaryGroup,
                    &tokenDefaultDacl,
                    NULL);
    MU_ASSERT_STATUS_SUCCESS(status);

    DumpToken(token);

    if (!RtlValidRelativeSecurityDescriptor(
                    relativeSd,
                    relativeSdLength,
                    0))
    {
        status = STATUS_INVALID_ACL;
        MU_ASSERT_STATUS_SUCCESS(status);
    }

    status = RtlSelfRelativeToAbsoluteSD(
                    relativeSd,
                    NULL,
                    &sdSize,
                    NULL,
                    &daclSize,
                    NULL,
                    &saclSize,
                    NULL,
                    &ownerSize,
                    NULL,
                    &primaryGroupSize);
    MU_ASSERT(STATUS_BUFFER_TOO_SMALL == status);

    daclSize = LW_MAX(daclSize, 1);
    saclSize = LW_MAX(saclSize, 1);
    ownerSize = LW_MAX(ownerSize, 1);
    primaryGroupSize = LW_MAX(primaryGroupSize, 1);

    status = RTL_ALLOCATE(&sd, VOID, sdSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    if (daclSize)
    {
        status = RTL_ALLOCATE(&dacl, VOID, daclSize);
        MU_ASSERT_STATUS_SUCCESS(status);
    }

    if (saclSize)
    {
        status = RTL_ALLOCATE(&sacl, VOID, saclSize);
        MU_ASSERT_STATUS_SUCCESS(status);
    }

    if (ownerSize)
    {
        status = RTL_ALLOCATE(&owner, VOID, ownerSize);
        MU_ASSERT_STATUS_SUCCESS(status);
    }

    if (primaryGroupSize)
    {
        status = RTL_ALLOCATE(&primaryGroup, VOID, primaryGroupSize);
        MU_ASSERT_STATUS_SUCCESS(status);
    }

    status = RtlSelfRelativeToAbsoluteSD(
                    relativeSd,
                    sd,
                    &sdSize,
                    dacl,
                    &daclSize,
                    sacl,
                    &saclSize,
                    owner,
                    &ownerSize,
                    primaryGroup,
                    &primaryGroupSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    DumpSecurityDescriptor(sd);

    isGranted = RtlAccessCheck(
                    sd,
                    token,
                    MAXIMUM_ALLOWED,
                    0,
                    &mapping,
                    &granted,
                    &status);
    MU_ASSERT(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED);

    MU_INFO("status = 0x%08x, granted = 0x%08x (%c)", status, granted, isGranted ? 'Y' : 'N');
}

MU_TEST(Security, 0004_AccessMarshal)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    TOKEN_USER tokenUser = { { 0 } };
    union {
        TOKEN_GROUPS tokenGroups;
        struct {
            ULONG GroupCount;
            SID_AND_ATTRIBUTES Groups[10];
        };
    } tokenGroupsUnion = { .tokenGroups = { 0 } };
    TOKEN_PRIVILEGES tokenPrivileges = { 0 };
    TOKEN_OWNER tokenOwner = { 0 };
    TOKEN_PRIMARY_GROUP tokenPrimaryGroup = { 0 };
    TOKEN_DEFAULT_DACL tokenDefaultDacl = { 0 };
    PACCESS_TOKEN token = NULL;
    PACCESS_TOKEN token2 = NULL;
    PACCESS_TOKEN_SELF_RELATIVE relative = NULL;
    ULONG ulRelativeSize = 0;
    //PSID primaryGroup = NULL;

    // SID present in an ACE
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-1805");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenUser.User.Sid = sid;

    // Bogus SID
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-12345678");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Sid = sid;
    SetFlag(tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Attributes, SE_GROUP_ENABLED);
    tokenGroupsUnion.GroupCount++;

    // Bogus SID
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-87654321");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Sid = sid;
    tokenGroupsUnion.GroupCount++;

    // SID present in an ACE
    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-1773");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Sid = sid;
    SetFlag(tokenGroupsUnion.Groups[tokenGroupsUnion.GroupCount].Attributes, SE_GROUP_ENABLED);
    tokenGroupsUnion.GroupCount++;

    status = RtlCreateAccessToken(
                    &token,
                    &tokenUser,
                    &tokenGroupsUnion.tokenGroups,
                    &tokenPrivileges,
                    &tokenOwner,
                    &tokenPrimaryGroup,
                    &tokenDefaultDacl,
                    NULL);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_INFO("Original token:");

    DumpToken(token);

    status = RtlAccessTokenToSelfRelativeAccessToken(
        token,
        NULL,
        &ulRelativeSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RTL_ALLOCATE(&relative, struct _ACCESS_TOKEN_SELF_RELATIVE, ulRelativeSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAccessTokenToSelfRelativeAccessToken(
        token,
        relative,
        &ulRelativeSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlSelfRelativeAccessTokenToAccessToken(
        relative,
        ulRelativeSize,
        &token2);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_INFO("Reconstructed token:");

    DumpToken(token2);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
