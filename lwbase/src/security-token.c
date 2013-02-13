/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        security-token.c
 *
 * Abstract:
 *
 *        Token/Access Functions in Security Module.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "security-includes.h"
#include <lw/atomic.h>
#include <lw/rtllog.h>
#include <lw/errno.h>
#include <pthread.h>
#include <stdlib.h>

#define EXCLUSIVE_LOCK_RWLOCK(pLock, bInLock) \
    do { \
        int _unixError = pthread_rwlock_wrlock(pLock); \
        if (_unixError) \
        { \
            LW_RTL_LOG_ERROR("ABORTING: Failed to set exclusive lock " \
                             "(error = %d).", _unixError); \
            abort(); \
        } \
        bInLock = TRUE; \
    } while (0)

#define SHARED_LOCK_RWLOCK(pLock, bInLock) \
    do { \
        int _unixError = pthread_rwlock_rdlock(pLock); \
        if (_unixError) \
        { \
            LW_RTL_LOG_ERROR("ABORTING: Failed to set shared lock " \
                             "(error = %d).", _unixError); \
            abort(); \
        } \
        bInLock = TRUE; \
    } while (0)

#define UNLOCK_RWLOCK(pLock, bInLock) \
    do { \
        if (bInLock) \
        { \
            int _unixError = pthread_rwlock_unlock(pLock); \
            if (_unixError) \
            { \
                LW_RTL_LOG_ERROR("ABORTING: Failed to unlock rwlock " \
                                 "(error = %d).", _unixError); \
                abort(); \
            } \
            bInLock = FALSE; \
        } \
    } while (0)

//
// ACCESS_MASK Functions
//

static
inline
VOID
RtlpMapGenericMaskSingleFlag(
    IN OUT PACCESS_MASK AccessMask,
    IN ACCESS_MASK GenericFlag,
    IN ACCESS_MASK SpecificFlags
    )
{
    if (IsSetFlag(*AccessMask, GenericFlag))
    {
        ClearFlag(*AccessMask, GenericFlag);
        SetFlag(*AccessMask, SpecificFlags);
    }
}

VOID
RtlMapGenericMask(
    IN OUT PACCESS_MASK AccessMask,
    IN PGENERIC_MAPPING GenericMapping
    )
{
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_READ, GenericMapping->GenericRead);
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_WRITE, GenericMapping->GenericWrite);
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_EXECUTE, GenericMapping->GenericExecute);
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_ALL, GenericMapping->GenericAll);
}

//
// Access Token Functions
//

inline
static
PVOID
RtlpAppendData(
    IN PVOID Location,
    IN PVOID Data,
    IN ULONG DataSize
    )
{
    RtlCopyMemory(Location, Data, DataSize);
    return LW_PTR_ADD(Location, DataSize);
}

NTSTATUS
RtlCreateAccessToken(
    OUT PACCESS_TOKEN* AccessToken,
    IN PTOKEN_USER User,
    IN PTOKEN_GROUPS Groups,
    IN PTOKEN_PRIVILEGES Privileges,
    IN PTOKEN_OWNER Owner,
    IN PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN PTOKEN_DEFAULT_DACL DefaultDacl,
    IN OPTIONAL PTOKEN_UNIX Unix
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int unixError = 0;
    ULONG requiredSize = 0;
    PACCESS_TOKEN token = NULL;
    ULONG i = 0;
    ULONG size = 0;
    PVOID location = NULL;

    if (!User || !User->User.Sid ||
        !Groups ||
        !Owner ||
        !PrimaryGroup ||
        !DefaultDacl)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlValidSid(User->User.Sid) ||
        (Owner->Owner && !RtlValidSid(Owner->Owner)) ||
        (PrimaryGroup->PrimaryGroup && !RtlValidSid(PrimaryGroup->PrimaryGroup)))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    // No user attributes currently exist.
    if (User->User.Attributes != 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    for (i = 0; i < Groups->GroupCount; i++)
    {
        // TODO-Perhaps validate Group attributes
        if (!Groups->Groups[i].Sid)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
        if (!RtlValidSid(Groups->Groups[i].Sid))
        {
            status = STATUS_INVALID_SID;
            GOTO_CLEANUP();
        }
    }

    if (DefaultDacl->DefaultDacl &&
        !RtlValidAcl(DefaultDacl->DefaultDacl, NULL))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    // Compute size required

    requiredSize = sizeof(*token);

    size = RtlLengthSid(User->User.Sid);
    status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
    GOTO_CLEANUP_ON_STATUS(status);

    if (Owner->Owner)
    {
        size = RtlLengthSid(Owner->Owner);
        status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (PrimaryGroup->PrimaryGroup)
    {
        size = RtlLengthSid(PrimaryGroup->PrimaryGroup);
        status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (DefaultDacl->DefaultDacl)
    {
        status = RtlSafeAddULONG(&requiredSize, requiredSize,
                                 DefaultDacl->DefaultDacl->AclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSafeMultiplyULONG(&size, sizeof(Groups->Groups[0]), Groups->GroupCount);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
    GOTO_CLEANUP_ON_STATUS(status);

    for (i = 0; i < Groups->GroupCount; i++)
    {
        size = RtlLengthSid(Groups->Groups[i].Sid);

        status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSafeMultiplyULONG(&size,
                                  sizeof(Privileges->Privileges[0]),
                                  Privileges->PrivilegeCount);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&token, ACCESS_TOKEN, requiredSize);
    GOTO_CLEANUP_ON_STATUS(status);

    location = LW_PTR_ADD(token, sizeof(*token));

    // Initialize

    token->ReferenceCount = 1;
    token->Flags = 0;
    unixError = pthread_rwlock_init(&token->RwLock, NULL);
    if (unixError)
    {
        LW_RTL_LOG_ERROR("Failed to init rwlock in access token "
                         "(error = %d).", unixError);
        status = LwErrnoToNtStatus(unixError);
        GOTO_CLEANUP();
    }
    token->pRwLock = &token->RwLock;

    token->User.Attributes = User->User.Attributes;
    token->User.Sid = (PSID) location;
    location = RtlpAppendData(location,
                              User->User.Sid,
                              RtlLengthSid(User->User.Sid));

    token->GroupCount = Groups->GroupCount;
    token->Groups = (PSID_AND_ATTRIBUTES) location;
    location = LwRtlOffsetToPointer(location, sizeof(Groups->Groups[0]) * Groups->GroupCount);
    for (i = 0; i < Groups->GroupCount; i++)
    {
        token->Groups[i].Attributes = Groups->Groups[i].Attributes;
        token->Groups[i].Sid = (PSID) location;
        location = RtlpAppendData(location,
                                  Groups->Groups[i].Sid,
                                  RtlLengthSid(Groups->Groups[i].Sid));
    }

    token->PrivilegeCount = Privileges->PrivilegeCount;
    token->Privileges = (PLUID_AND_ATTRIBUTES) location;
    location = LwRtlOffsetToPointer(
                location,
                sizeof(Privileges->Privileges[0]) * Privileges->PrivilegeCount);
    memcpy(token->Privileges,
           Privileges->Privileges,
           sizeof(token->Privileges[0]) * token->PrivilegeCount);

    if (Owner->Owner)
    {
        token->Owner = (PSID) location;
        location = RtlpAppendData(location,
                                  Owner->Owner,
                                  RtlLengthSid(Owner->Owner));
    }

    if (PrimaryGroup->PrimaryGroup)
    {
        token->PrimaryGroup = (PSID) location;
        location = RtlpAppendData(location,
                                  PrimaryGroup->PrimaryGroup,
                                  RtlLengthSid(PrimaryGroup->PrimaryGroup));
    }

    if (DefaultDacl->DefaultDacl)
    {
        token->DefaultDacl = (PACL) location;
        location = RtlpAppendData(location,
                                  DefaultDacl->DefaultDacl,
                                  DefaultDacl->DefaultDacl->AclSize);
    }

    if (Unix)
    {
        SetFlag(token->Flags, ACCESS_TOKEN_FLAG_UNIX_PRESENT);
        token->Uid = Unix->Uid;
        token->Gid = Unix->Gid;
        token->Umask = Unix->Umask;
    }

    if (location != LW_PTR_ADD(token, requiredSize))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RtlReleaseAccessToken(&token);
    }

    *AccessToken = token;

    return status;
}

VOID
RtlReferenceAccessToken(
    IN PACCESS_TOKEN AccessToken
    )
{
    LwInterlockedIncrement(&AccessToken->ReferenceCount);
}

VOID
RtlReleaseAccessToken(
    IN OUT PACCESS_TOKEN* AccessToken
    )
{
    PACCESS_TOKEN accessToken = *AccessToken;

    if (accessToken)
    {
        LONG count = LwInterlockedDecrement(&accessToken->ReferenceCount);
        assert(count >= 0);
        if (0 == count)
        {
            if (accessToken->pRwLock)
            {
                pthread_rwlock_destroy(accessToken->pRwLock);
            }
            RTL_FREE(AccessToken);
        }
    }
}

NTSTATUS
RtlQueryAccessTokenInformation(
    IN PACCESS_TOKEN AccessToken,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT OPTIONAL PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnedLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isLocked = FALSE;
    ULONG requiredSize = 0;
    ULONG i = 0;
    PVOID location = NULL;

    if (!AccessToken || !ReturnedLength)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!TokenInformation && (TokenInformationLength != 0))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    SHARED_LOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    // Check required size
    switch (TokenInformationClass)
    {
        case TokenUser:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_USER),
                            RtlLengthSid(AccessToken->User.Sid));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenGroups:
            status = RtlSafeMultiplyULONG(
                        &requiredSize,
                        sizeof(AccessToken->Groups[0]),
                        AccessToken->GroupCount);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSafeAddULONG(&requiredSize, requiredSize, sizeof(TOKEN_GROUPS));
            GOTO_CLEANUP_ON_STATUS(status);

            for (i = 0; i < AccessToken->GroupCount; i++)
            {
                status = RtlSafeAddULONG(
                                &requiredSize,
                                requiredSize,
                                RtlLengthSid(AccessToken->Groups[i].Sid));
                GOTO_CLEANUP_ON_STATUS(status);
            }
            break;
        case TokenPrivileges:
            status = RtlSafeMultiplyULONG(
                            &requiredSize,
                            sizeof(AccessToken->Privileges[0]),
                            AccessToken->PrivilegeCount);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSafeAddULONG(&requiredSize, requiredSize, sizeof(TOKEN_PRIVILEGES));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenOwner:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_OWNER),
                            RtlLengthSid(AccessToken->Owner));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenPrimaryGroup:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_PRIMARY_GROUP),
                            RtlLengthSid(AccessToken->PrimaryGroup));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenDefaultDacl:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_DEFAULT_DACL),
                            (AccessToken->DefaultDacl ?
                             AccessToken->DefaultDacl->AclSize :
                             0));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
    }

    if (requiredSize > TokenInformationLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    if (!TokenInformation)
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    // Copy data
    switch (TokenInformationClass)
    {
        case TokenUser:
        {
            PTOKEN_USER tokenInfo = (PTOKEN_USER) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_USER));
            tokenInfo->User.Attributes = AccessToken->User.Attributes;
            tokenInfo->User.Sid = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->User.Sid,
                                      RtlLengthSid(AccessToken->User.Sid));
            break;
        }
        case TokenGroups:
        {
            PTOKEN_GROUPS tokenInfo = (PTOKEN_GROUPS) TokenInformation;
            location = LW_PTR_ADD(TokenInformation,
                                  (sizeof(TOKEN_GROUPS) + 
                                   (sizeof(AccessToken->Groups[0]) * AccessToken->GroupCount)));
            tokenInfo->GroupCount = AccessToken->GroupCount;
            for (i = 0; i < AccessToken->GroupCount; i++)
            {
                tokenInfo->Groups[i].Attributes = AccessToken->Groups[i].Attributes;
                tokenInfo->Groups[i].Sid = (PSID) location;
                location = RtlpAppendData(location,
                                          AccessToken->Groups[i].Sid,
                                          RtlLengthSid(AccessToken->Groups[i].Sid));
            }
            break;
        }
        case TokenPrivileges:
        {
            PTOKEN_PRIVILEGES tokenInfo = (PTOKEN_PRIVILEGES) TokenInformation;
            location = LW_PTR_ADD(TokenInformation,
                                  (sizeof(TOKEN_PRIVILEGES) +
                                   (sizeof(AccessToken->Privileges[0]) * AccessToken->PrivilegeCount)));
            tokenInfo->PrivilegeCount = AccessToken->PrivilegeCount;
            if (AccessToken->PrivilegeCount)
            {
                memcpy(tokenInfo->Privileges,
                       AccessToken->Privileges,
                       sizeof(tokenInfo->Privileges[0]) * tokenInfo->PrivilegeCount);
            }
            break;
        }
        case TokenOwner:
        {
            PTOKEN_OWNER tokenInfo = (PTOKEN_OWNER) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_OWNER));
            tokenInfo->Owner = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->Owner,
                                      RtlLengthSid(AccessToken->Owner));
            break;
        }
        case TokenPrimaryGroup:
        {
            PTOKEN_PRIMARY_GROUP tokenInfo = (PTOKEN_PRIMARY_GROUP) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_PRIMARY_GROUP));
            tokenInfo->PrimaryGroup = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->PrimaryGroup,
                                      RtlLengthSid(AccessToken->PrimaryGroup));
            break;
        }
        case TokenDefaultDacl:
        {
            PTOKEN_DEFAULT_DACL tokenInfo = (PTOKEN_DEFAULT_DACL) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_DEFAULT_DACL));
            if (AccessToken->DefaultDacl)
            {
                tokenInfo->DefaultDacl = (PACL) location;
                location = RtlpAppendData(location,
                                          AccessToken->DefaultDacl,
                                          AccessToken->DefaultDacl->AclSize);
            }
            break;
        }
        default:
            // We should have already checked.
            status = STATUS_ASSERTION_FAILURE;
            GOTO_CLEANUP();
    }

    if (location != LW_PTR_ADD(TokenInformation, requiredSize))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    UNLOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (ReturnedLength)
    {
        *ReturnedLength = requiredSize;
    }

    return status;
}

NTSTATUS
RtlQueryAccessTokenUnixInformation(
    IN PACCESS_TOKEN AccessToken,
    OUT PTOKEN_UNIX TokenInformation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isLocked = FALSE;
    TOKEN_UNIX tokenInfo = { 0 };

    if (!AccessToken)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    SHARED_LOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (!IsSetFlag(AccessToken->Flags, ACCESS_TOKEN_FLAG_UNIX_PRESENT))
    {
        status = STATUS_NOT_FOUND;
        GOTO_CLEANUP();
    }

    tokenInfo.Uid = AccessToken->Uid;
    tokenInfo.Gid = AccessToken->Gid;
    tokenInfo.Umask = AccessToken->Umask;

    status = STATUS_SUCCESS;

cleanup:
    UNLOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (!NT_SUCCESS(status))
    {
        RtlZeroMemory(&tokenInfo, sizeof(tokenInfo));
    }

    *TokenInformation = tokenInfo;
    return status;
}

BOOLEAN
RtlIsSidMemberOfToken(
    IN PACCESS_TOKEN AccessToken,
    IN PSID Sid
    )
{
    BOOLEAN isMember = FALSE;
    BOOLEAN isLocked = FALSE;
    ULONG i = 0;

    SHARED_LOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (RtlEqualSid(Sid, AccessToken->User.Sid))
    {
        isMember = TRUE;
        GOTO_CLEANUP();
    }

    for (i = 0; i < AccessToken->GroupCount; i++)
    {
        PSID_AND_ATTRIBUTES sidInfo = &AccessToken->Groups[i];
        if (IsSetFlag(sidInfo->Attributes, SE_GROUP_ENABLED) &&
            RtlEqualSid(Sid, sidInfo->Sid))
        {
            isMember = TRUE;
            GOTO_CLEANUP();
        }
    }

    isMember = FALSE;

cleanup:
    UNLOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    return isMember;
}

////////////////////////////////////////////////////////////////////////

BOOLEAN
RtlAccessCheck(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PACCESS_TOKEN AccessToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )
{
    return RtlAccessCheckEx(
               SecurityDescriptor,
               AccessToken,
               DesiredAccess,
               PreviouslyGrantedAccess,
               GenericMapping,
               NULL,
               GrantedAccess,
               AccessStatus);
}


////////////////////////////////////////////////////////////////////////

BOOLEAN
RtlAccessCheckEx(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PACCESS_TOKEN AccessToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACCESS_MASK RemainingDesiredAccess,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )
{
    NTSTATUS status = STATUS_ACCESS_DENIED;
    BOOLEAN isLocked = FALSE;
    ACCESS_MASK grantedAccess = PreviouslyGrantedAccess;
    ACCESS_MASK deniedAccess = 0;
    ACCESS_MASK desiredAccess = DesiredAccess;
    BOOLEAN wantMaxAllowed = FALSE;
    USHORT aclSizeUsed = 0;
    USHORT aceOffset = 0;
    PACE_HEADER aceHeader = NULL;
    union {
        SID Sid;
        BYTE Buffer[SID_MAX_SIZE];
    } sidBuffer;
    ULONG ulSidSize = sizeof(sidBuffer);

    if (!SecurityDescriptor || !AccessToken || !GenericMapping)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!LW_IS_VALID_FLAGS(DesiredAccess, VALID_DESIRED_ACCESS_MASK))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!LW_IS_VALID_FLAGS(PreviouslyGrantedAccess, VALID_GRANTED_ACCESS_MASK))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if ((SecurityDescriptor->Owner == NULL) || 
        (SecurityDescriptor->Group == NULL))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    wantMaxAllowed = IsSetFlag(desiredAccess, MAXIMUM_ALLOWED);
    ClearFlag(desiredAccess, MAXIMUM_ALLOWED);

    RtlMapGenericMask(&desiredAccess, GenericMapping);

    //
    // NT AUTHORITY\SYSTEM is always allowed an access
    //
    status = RtlCreateWellKnownSid(WinLocalSystemSid,
                                   NULL,
                                   &sidBuffer.Sid,
                                   &ulSidSize);
    GOTO_CLEANUP_ON_STATUS(status);

    SHARED_LOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (RtlIsSidMemberOfToken(AccessToken, &sidBuffer.Sid))
    {
        if (wantMaxAllowed)
        {
            SetFlag(desiredAccess, STANDARD_RIGHTS_ALL);
            SetFlag(desiredAccess, GENERIC_ALL);
            RtlMapGenericMask(&desiredAccess, GenericMapping);
        }
        SetFlag(grantedAccess, desiredAccess);
        desiredAccess = 0;

        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    if (wantMaxAllowed || IsSetFlag(desiredAccess, ACCESS_SYSTEM_SECURITY))
    {
        // TODO-Handle ACCESS_SYSTEM_SECURITY by checking SE_SECURITY_NAME
        // privilege.  For now, requesting ACCESS_SYSTEM_SECURITY is not
        // allowed.

        ulSidSize = sizeof(sidBuffer);
        status = RtlCreateWellKnownSid(
                     WinBuiltinAdministratorsSid,
                     NULL,
                     &sidBuffer.Sid,
                     &ulSidSize);
        GOTO_CLEANUP_ON_STATUS(status);

        if (RtlIsSidMemberOfToken(AccessToken, &sidBuffer.Sid))
        {
            SetFlag(grantedAccess, ACCESS_SYSTEM_SECURITY);
            ClearFlag(desiredAccess, ACCESS_SYSTEM_SECURITY);
        }
        else if (IsSetFlag(desiredAccess, ACCESS_SYSTEM_SECURITY))
        {
            status = STATUS_PRIVILEGE_NOT_HELD;
            GOTO_CLEANUP();
        }
    }

    if (wantMaxAllowed || IsSetFlag(desiredAccess, WRITE_OWNER))
    {
        // TODO-Allow WRITE_OWNER if have SE_TAKE_OWNERSHIP_NAME regardless
        // of DACL.

        //
        // BUILTIN\Administrators are always allowed WRITE_OWNER
        //

        ulSidSize = sizeof(sidBuffer);
        status = RtlCreateWellKnownSid(
                     WinBuiltinAdministratorsSid,
                     NULL,
                     &sidBuffer.Sid,
                     &ulSidSize);
        GOTO_CLEANUP_ON_STATUS(status);

        if (RtlIsSidMemberOfToken(AccessToken, &sidBuffer.Sid))
        {
            SetFlag(grantedAccess, WRITE_OWNER);
            ClearFlag(desiredAccess, WRITE_OWNER);
        }
    }

    //
    // Owner can always read the SD and write the DACL.
    //

    if (wantMaxAllowed || IsSetFlag(desiredAccess, READ_CONTROL | WRITE_DAC))
    {
        if (RtlIsSidMemberOfToken(AccessToken, SecurityDescriptor->Owner))
        {
            if (wantMaxAllowed)
            {
                desiredAccess |= (READ_CONTROL | WRITE_DAC);
            }
                
            SetFlag(grantedAccess, (READ_CONTROL | WRITE_DAC) & desiredAccess);
            ClearFlag(desiredAccess, grantedAccess);
        }
    }

    // TODO-MAXIMUM_ALLOWED wrt privileges and WRITE_OWNER and
    // ACCESS_SYSTEM_SECURITY above.

    if (!SecurityDescriptor->Dacl)
    {
        // TODO-Interplay with special bits above?
        if (wantMaxAllowed)
        {
            SetFlag(desiredAccess, STANDARD_RIGHTS_ALL);
            SetFlag(desiredAccess, GENERIC_ALL);
            RtlMapGenericMask(&desiredAccess, GenericMapping);
        }
        SetFlag(grantedAccess, desiredAccess);
        desiredAccess = 0;

        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    if (!RtlValidAcl(SecurityDescriptor->Dacl, &aclSizeUsed))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    while (wantMaxAllowed || desiredAccess)
    {
        status = RtlIterateAce(SecurityDescriptor->Dacl,
                               aclSizeUsed,
                               &aceOffset,
                               &aceHeader);
        if (STATUS_NO_MORE_ENTRIES == status)
        {
            break;
        }
        GOTO_CLEANUP_ON_STATUS(status);

        // Check ACE
        switch (aceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            {
                PACCESS_ALLOWED_ACE ace = (PACCESS_ALLOWED_ACE) aceHeader;
                ACCESS_MASK mask = ace->Mask;

                RtlMapGenericMask(&mask, GenericMapping);

                if (wantMaxAllowed || IsSetFlag(desiredAccess, mask))
                {
                    // SID in token => add bits to granted bits
                    PSID sid = RtlpGetSidAccessAllowedAce(ace);

                    if (RtlIsSidMemberOfToken(AccessToken, sid))
                    {
                        if (wantMaxAllowed)
                        {
                            SetFlag(grantedAccess, mask & ~deniedAccess);
                        }
                        else
                        {
                            SetFlag(grantedAccess, mask & desiredAccess);
                        }

                        ClearFlag(desiredAccess, grantedAccess);
                    }
                }
                break;
            }
            case ACCESS_DENIED_ACE_TYPE:
            {
                // Allowed and deny ACEs are isomorphic.
                PACCESS_ALLOWED_ACE ace = (PACCESS_ALLOWED_ACE) aceHeader;
                ACCESS_MASK mask = ace->Mask;

                RtlMapGenericMask(&mask, GenericMapping);

                if (wantMaxAllowed || IsSetFlag(desiredAccess, mask))
                {
                    // SID in token => exit with STATUS_ACCESS_DENIED
                    PSID sid = RtlpGetSidAccessAllowedAce(ace);

                    if (RtlIsSidMemberOfToken(AccessToken, sid))
                    {
                        if (wantMaxAllowed)
                        {
                            SetFlag(deniedAccess, mask);
                        }
                        else
                        {
                            status = STATUS_ACCESS_DENIED;
                            GOTO_CLEANUP();
                        }

                        ClearFlag(desiredAccess, deniedAccess);
                    }
                }
                break;
            }
            default:
                // ignore
                break;
        }
    }

    status = desiredAccess ? STATUS_ACCESS_DENIED : STATUS_SUCCESS;

cleanup:
    UNLOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (NT_SUCCESS(status) &&
        !LW_IS_VALID_FLAGS(grantedAccess, VALID_GRANTED_ACCESS_MASK))
    {
        status = STATUS_ASSERTION_FAILURE;
    }
    if (!NT_SUCCESS(status))
    {
        grantedAccess = PreviouslyGrantedAccess;
    }

    if (RemainingDesiredAccess)
    {
        *RemainingDesiredAccess = desiredAccess;
    }

    *GrantedAccess = grantedAccess;
    *AccessStatus = status;

    return NT_SUCCESS(status) ? TRUE : FALSE;
}

static
inline
VOID
Align32(
    PULONG ulValue
    )
{
    ULONG ulRem = *ulValue % 32;

    if (ulRem) *ulValue += (32 - ulRem);
}

static
inline
NTSTATUS
CheckOffset(
    ULONG ulOffset,
    ULONG ulSize,
    ULONG ulMaxSize
    )
{
    if (ulMaxSize - ulOffset < ulSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

static
inline
NTSTATUS
AdvanceTo(
    PULONG ulValue,
    ULONG ulPosition,
    ULONG ulSize
    )
{
    if (ulPosition >= ulSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        *ulValue = ulPosition;
        return STATUS_SUCCESS;
    }
}

static
ULONG
RtlAccessTokenRelativeSize(
    PACCESS_TOKEN pToken
    )
{
    BOOLEAN isLocked = FALSE;
    ULONG ulRelativeSize = 0;
    ULONG i = 0;

    ulRelativeSize += sizeof(ACCESS_TOKEN_SELF_RELATIVE);
    Align32(&ulRelativeSize);

    SHARED_LOCK_RWLOCK(&pToken->RwLock, isLocked);

    ulRelativeSize += RtlLengthSid(pToken->User.Sid);
    Align32(&ulRelativeSize);

    if (pToken->Groups)
    {
        ulRelativeSize += sizeof(SID_AND_ATTRIBUTES_SELF_RELATIVE) * pToken->GroupCount;
        Align32(&ulRelativeSize);

        for (i = 0; i < pToken->GroupCount; i++)
        {
            ulRelativeSize += RtlLengthSid(pToken->Groups[i].Sid);
            Align32(&ulRelativeSize);
        }
    }

    if (pToken->Privileges)
    {
        ulRelativeSize += sizeof(pToken->Privileges[0]) * pToken->PrivilegeCount;
        Align32(&ulRelativeSize);
    }

    if (pToken->Owner)
    {
        ulRelativeSize += RtlLengthSid(pToken->Owner);
        Align32(&ulRelativeSize);
    }

    if (pToken->PrimaryGroup)
    {
        ulRelativeSize += RtlLengthSid(pToken->PrimaryGroup);
        Align32(&ulRelativeSize);
    }

    if (pToken->DefaultDacl)
    {
        ulRelativeSize += RtlGetAclSize(pToken->DefaultDacl);
        Align32(&ulRelativeSize);
    }

    UNLOCK_RWLOCK(&pToken->RwLock, isLocked);

    return ulRelativeSize;
}

NTSTATUS
RtlAccessTokenToSelfRelativeAccessToken(
    IN PACCESS_TOKEN pToken,
    OUT OPTIONAL PACCESS_TOKEN_SELF_RELATIVE pRelative,
    IN OUT PULONG pulSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isLocked = FALSE;
    ULONG ulRelativeSize = RtlAccessTokenRelativeSize(pToken);
    PSID_AND_ATTRIBUTES_SELF_RELATIVE pGroups = NULL;
    PBYTE pBuffer = NULL;
    ULONG ulOffset = 0;
    ULONG i = 0;

    if (pRelative)
    {
        if (*pulSize < ulRelativeSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        pBuffer = (PBYTE) pRelative;
        
        SHARED_LOCK_RWLOCK(&pToken->RwLock, isLocked);

        pRelative->Flags = pToken->Flags;
        pRelative->User.Attributes = pToken->User.Attributes;
        pRelative->GroupCount = pToken->GroupCount;
        pRelative->PrivilegeCount = pToken->PrivilegeCount;
        pRelative->Uid = pToken->Uid;
        pRelative->Gid = pToken->Gid;
        pRelative->Umask = pToken->Umask;
        
        ulOffset += sizeof(*pRelative);
        Align32(&ulOffset);
        
        pRelative->User.SidOffset = ulOffset;
        memcpy(pBuffer + ulOffset, pToken->User.Sid, RtlLengthSid(pToken->User.Sid));
        ulOffset += RtlLengthSid(pToken->User.Sid);
        Align32(&ulOffset);
        
        if (pToken->Groups)
        {
            pRelative->GroupsOffset = ulOffset;
            pGroups = (PSID_AND_ATTRIBUTES_SELF_RELATIVE) (pBuffer + ulOffset);
            ulOffset += sizeof(SID_AND_ATTRIBUTES_SELF_RELATIVE) * pToken->GroupCount;
            Align32(&ulOffset);
            
            for (i = 0; i < pToken->GroupCount; i++)
            {
                pGroups[i].Attributes = pToken->Groups[i].Attributes;
                pGroups[i].SidOffset = ulOffset;
                memcpy(pBuffer + ulOffset, pToken->Groups[i].Sid, RtlLengthSid(pToken->Groups[i].Sid));
                ulOffset += RtlLengthSid(pToken->Groups[i].Sid);
                Align32(&ulOffset);
            }
        }
        else
        {
            pRelative->GroupsOffset = 0;
        }
        
        if (pToken->Privileges)
        {
            pRelative->PrivilegesOffset = ulOffset;
            memcpy(pBuffer + ulOffset,
                   pToken->Privileges,
                   sizeof(pToken->Privileges[0]) * pToken->PrivilegeCount);
            ulOffset += sizeof(pToken->Privileges[0]) * pToken->PrivilegeCount;
            Align32(&ulOffset);
        }
        else
        {
            pRelative->PrivilegesOffset = 0;
        }

        if (pToken->Owner)
        {
            pRelative->OwnerOffset = ulOffset;
            memcpy(pBuffer + ulOffset, pToken->Owner, RtlLengthSid(pToken->Owner));
            ulOffset += RtlLengthSid(pToken->Owner);
            Align32(&ulOffset);
        }
        else
        {
            pRelative->OwnerOffset = 0;
        }
        
        if (pToken->PrimaryGroup)
        {
            pRelative->PrimaryGroupOffset = ulOffset;
            memcpy(pBuffer + ulOffset, pToken->PrimaryGroup, RtlLengthSid(pToken->PrimaryGroup));
            ulOffset += RtlLengthSid(pToken->PrimaryGroup);
            Align32(&ulOffset);
        }
        else
        {
            pRelative->PrimaryGroupOffset = 0;
        }
        
        if (pToken->DefaultDacl)
        {
            pRelative->DefaultDaclOffset = ulOffset;
            memcpy(pBuffer + ulOffset, pToken->DefaultDacl, RtlGetAclSize(pToken->DefaultDacl));
            ulOffset += RtlGetAclSize(pToken->DefaultDacl);
            Align32(&ulOffset);
        }
        else
        {
            pRelative->DefaultDaclOffset = 0;
        }
        

        assert(ulOffset == ulRelativeSize);
    }

cleanup:
    UNLOCK_RWLOCK(&pToken->RwLock, isLocked);

    *pulSize = ulRelativeSize;

    return status;
}

static
NTSTATUS
RtlValidateSelfRelativeSid(
    PSID pSid,
    ULONG ulOffset,
    ULONG ulRelativeSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = CheckOffset(ulOffset, SID_MIN_SIZE, ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CheckOffset(ulOffset, RtlLengthSid(pSid), ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    return status;
}

NTSTATUS
RtlSelfRelativeAccessTokenToAccessToken(
    IN PACCESS_TOKEN_SELF_RELATIVE pRelative,
    IN ULONG ulRelativeSize,
    OUT PACCESS_TOKEN* ppToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulOffset = 0;
    PBYTE pBuffer = (PBYTE) pRelative;
    PSID pSid = NULL;
    PSID_AND_ATTRIBUTES_SELF_RELATIVE pGroups = NULL;
    ULONG ulSize = 0;
    ULONG ulRealSize = 0;
    ULONG i = 0;
    TOKEN_USER User = {{0}};
    TOKEN_OWNER Owner = {0};
    TOKEN_PRIMARY_GROUP PrimaryGroup = {0};
    TOKEN_UNIX Unix = {0};
    PTOKEN_GROUPS pTokenGroups = NULL;
    PTOKEN_PRIVILEGES pTokenPrivileges = NULL;
    TOKEN_DEFAULT_DACL DefaultDacl = {0};

    status = CheckOffset(0, sizeof(*pRelative), ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pRelative->Flags & ACCESS_TOKEN_FLAG_UNIX_PRESENT)
    {
        Unix.Uid = pRelative->Uid;
        Unix.Gid = pRelative->Gid;
        Unix.Umask = pRelative->Umask;
    }

    User.User.Attributes = pRelative->User.Attributes;
    ulOffset = pRelative->User.SidOffset;
    pSid = (PSID) (pBuffer + ulOffset);
    status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);
    
    User.User.Sid = pSid;

    ulOffset = pRelative->GroupsOffset;
    if (ulOffset)
    {
        status = LwRtlSafeMultiplyULONG(
            &ulSize,
            sizeof(SID_AND_ATTRIBUTES_SELF_RELATIVE),
            pRelative->GroupCount);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwRtlSafeMultiplyULONG(
            &ulRealSize,
            sizeof(SID_AND_ATTRIBUTES),
            pRelative->GroupCount);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwRtlSafeAddULONG(
            &ulRealSize,
            ulRealSize,
            sizeof(TOKEN_GROUPS));
        GOTO_CLEANUP_ON_STATUS(status);

        status = CheckOffset(ulOffset, ulSize, ulRelativeSize);
        GOTO_CLEANUP_ON_STATUS(status);

        pGroups = (PSID_AND_ATTRIBUTES_SELF_RELATIVE) (pBuffer + ulOffset);
        
        status = RTL_ALLOCATE(&pTokenGroups, TOKEN_GROUPS, ulRealSize);
        GOTO_CLEANUP_ON_STATUS(status);
        
        pTokenGroups->GroupCount = pRelative->GroupCount;

        for (i = 0; i < pRelative->GroupCount; i++)
        {
            pTokenGroups->Groups[i].Attributes = pGroups[i].Attributes;
            
            ulOffset = pGroups[i].SidOffset;
            pSid = (PSID) (pBuffer + ulOffset);
            status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
            GOTO_CLEANUP_ON_STATUS(status);
            
            pTokenGroups->Groups[i].Sid = pSid;
        }
    }

    ulOffset = pRelative->PrivilegesOffset;
    if (ulOffset)
    {
        status = LwRtlSafeMultiplyULONG(
            &ulSize,
            sizeof(LUID_AND_ATTRIBUTES),
            pRelative->PrivilegeCount);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwRtlSafeMultiplyULONG(
            &ulRealSize,
            sizeof(LUID_AND_ATTRIBUTES),
            pRelative->PrivilegeCount);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwRtlSafeAddULONG(
            &ulRealSize,
            ulRealSize,
            sizeof(TOKEN_PRIVILEGES));
        GOTO_CLEANUP_ON_STATUS(status);

        status = CheckOffset(ulOffset, ulSize, ulRelativeSize);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RTL_ALLOCATE(&pTokenPrivileges, TOKEN_PRIVILEGES, ulRealSize);
        GOTO_CLEANUP_ON_STATUS(status);

        pTokenPrivileges->PrivilegeCount = pRelative->PrivilegeCount;
        memcpy(pTokenPrivileges->Privileges,
               pBuffer + ulOffset,
               sizeof(LUID_AND_ATTRIBUTES) * pTokenPrivileges->PrivilegeCount);
        
    }

    ulOffset = pRelative->OwnerOffset;
    if (ulOffset)
    {
        pSid = (PSID) (pBuffer + ulOffset);
        status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
        GOTO_CLEANUP_ON_STATUS(status);
    
        Owner.Owner = pSid;
    }

    ulOffset = pRelative->PrimaryGroupOffset;
    if (ulOffset)
    {
        pSid = (PSID) (pBuffer + ulOffset);
        status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
        GOTO_CLEANUP_ON_STATUS(status);
        
        PrimaryGroup.PrimaryGroup = pSid;
    }

    status = RtlCreateAccessToken(
        ppToken,
        &User,
        pTokenGroups,
        pTokenPrivileges,
        &Owner,
        &PrimaryGroup,
        &DefaultDacl,
        pRelative->Flags & ACCESS_TOKEN_FLAG_UNIX_PRESENT ? &Unix : NULL);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    RTL_FREE(&pTokenGroups);

    if (!NT_SUCCESS(status))
    {
        *ppToken = NULL;
    }

    return status;
}



BOOLEAN
RtlPrivilegeCheck(
   IN OUT PPRIVILEGE_SET RequiredPrivileges,
   IN PACCESS_TOKEN AccessToken
   )
{
    PLUID_AND_ATTRIBUTES pPrivileges = AccessToken->Privileges;
    BOOLEAN isLocked = FALSE;
    ULONG privilegesNeeded = 0;
    ULONG iAssigned = 0;
    ULONG iRequired = 0;

    privilegesNeeded = RequiredPrivileges->PrivilegeCount;
    
    SHARED_LOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    for (iRequired = 0;
         iRequired < RequiredPrivileges->PrivilegeCount;
         iRequired++)
    {
        for (iAssigned = 0;
             iAssigned < AccessToken->PrivilegeCount;
             iAssigned++)
        {
            if (RtlEqualLuid(&RequiredPrivileges->Privilege[iRequired].Luid,
                             &pPrivileges[iAssigned].Luid))
            {
                if (pPrivileges[iAssigned].Attributes & SE_PRIVILEGE_ENABLED)
                {
                    if (privilegesNeeded > 0)
                    {
                        privilegesNeeded--;
                    }

                    if (!(RequiredPrivileges->Control &
                          PRIVILEGE_SET_ALL_NECESSARY))
                    {
                        // Turn off counting the privileges needed to grant
                        // access. It's no longer necessary as at least one
                        // of them has to be enabled and it's just been
                        // found.
                        privilegesNeeded = 0;
                    }

                    RequiredPrivileges->Privilege[iRequired].Attributes
                        |= SE_PRIVILEGE_USED_FOR_ACCESS;
                }
                else if (RequiredPrivileges->Control &
                         PRIVILEGE_SET_ALL_NECESSARY)
                {
                    // Further privilege check is pointless since all of
                    // the privileges are required and one of them turns
                    // out disabled.
                    GOTO_CLEANUP();
                }
            }
        }
    }

cleanup:
    UNLOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    return (privilegesNeeded == 0);
}


static
NTSTATUS
RtlpAppendTokenPrivileges(
    OUT OPTIONAL PVOID Buffer,
    IN OPTIONAL ULONG BufferLength,
    IN PLUID_AND_ATTRIBUTES AppendPrivilege,
    IN ULONG Index,
    IN PULONG pBufferUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PTOKEN_PRIVILEGES pPrivileges = (PTOKEN_PRIVILEGES)Buffer;
    ULONG bufferUsed = 0;

    bufferUsed += sizeof(*pPrivileges);
    bufferUsed += sizeof(pPrivileges->Privileges[0]) * (Index + 1);

    if (bufferUsed <= BufferLength)
    {
        pPrivileges->Privileges[Index].Luid = AppendPrivilege->Luid;
        pPrivileges->Privileges[Index].Attributes = AppendPrivilege->Attributes;

        pPrivileges->PrivilegeCount = Index + 1;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    *pBufferUsed = bufferUsed;

    return status;
}


NTSTATUS
RtlAdjustTokenPrivileges(
    IN PACCESS_TOKEN AccessToken,
    IN BOOLEAN DisableAll,
    IN OPTIONAL PTOKEN_PRIVILEGES NewState,
    IN ULONG BufferLength,
    OUT OPTIONAL PTOKEN_PRIVILEGES PreviousState,
    OUT OPTIONAL PULONG pReturnedLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isLocked = FALSE;
    PLUID_AND_ATTRIBUTES pPrivileges = AccessToken->Privileges;
    ULONG assignedIndex = 0;
    ULONG modIndex = 0;
    ULONG returnedIndex = 0;
    ULONG BufferUsed = 0;
    ULONG adjustedCount = 0;

    if (!DisableAll && !NewState)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!NewState && BufferLength > 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    EXCLUSIVE_LOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (AccessToken->PrivilegeCount == 0)
    {
        status = STATUS_NOT_ALL_ASSIGNED;
        GOTO_CLEANUP();
    }

    if (DisableAll)
    {
        for (assignedIndex = 0;
             assignedIndex < AccessToken->PrivilegeCount;
             assignedIndex++)
        {
            status = RtlpAppendTokenPrivileges(
                                PreviousState,
                                BufferLength,
                                &pPrivileges[assignedIndex],
                                returnedIndex++,
                                &BufferUsed);
            if (status == STATUS_SUCCESS)
            {
                ClearFlag(pPrivileges[assignedIndex].Attributes,
                          SE_PRIVILEGE_ENABLED);
            }
        }

        GOTO_CLEANUP();
    }

    for (modIndex = 0;
         modIndex < NewState->PrivilegeCount;
         modIndex++)
    {
        for (assignedIndex = 0;
             assignedIndex < AccessToken->PrivilegeCount;
             assignedIndex++)
        {
            if (RtlEqualLuid(&NewState->Privileges[modIndex].Luid,
                             &pPrivileges[assignedIndex].Luid))
            {
                if (NewState->Privileges[modIndex].Attributes == 0)
                {
                    status = RtlpAppendTokenPrivileges(
                                        PreviousState,
                                        BufferLength,
                                        &pPrivileges[assignedIndex],
                                        returnedIndex++,
                                        &BufferUsed);
                    if (status == STATUS_SUCCESS)
                    {
                        ClearFlag(pPrivileges[assignedIndex].Attributes,
                                  SE_PRIVILEGE_ENABLED);
                        adjustedCount++;
                    }
                }

                if (IsSetFlag(NewState->Privileges[modIndex].Attributes,
                              SE_PRIVILEGE_ENABLED))
                {
                    status = RtlpAppendTokenPrivileges(
                                        PreviousState,
                                        BufferLength,
                                        &pPrivileges[assignedIndex],
                                        returnedIndex++,
                                        &BufferUsed);
                    if (status == STATUS_SUCCESS)
                    {
                        SetFlag(pPrivileges[assignedIndex].Attributes,
                                SE_PRIVILEGE_ENABLED);
                        adjustedCount++;
                    }
                }

                if (IsSetFlag(NewState->Privileges[modIndex].Attributes,
                              SE_PRIVILEGE_REMOVED))
                {
                    if (assignedIndex + 1 < AccessToken->PrivilegeCount)
                    {
                        RtlMoveMemory(
                              &pPrivileges[assignedIndex],
                              &pPrivileges[assignedIndex + 1],
                              sizeof(pPrivileges[0]) * 
                              AccessToken->PrivilegeCount - assignedIndex - 1);
                    }

                    RtlZeroMemory(
                          &pPrivileges[AccessToken->PrivilegeCount - 1],
                          sizeof(pPrivileges[0]));

                    AccessToken->PrivilegeCount--;
                    adjustedCount++;
                }

                break;
            }
        }
    }

cleanup:
    if (PreviousState && (BufferUsed > BufferLength))
    {
        // There was not enough space in PreviousState buffer so roll 
        // the changes back and return STATUS_BUFFER_TOO_SMALL
        for (returnedIndex = 0;
             returnedIndex < PreviousState->PrivilegeCount;
             returnedIndex++)
        {
            for (assignedIndex = 0;
                 assignedIndex < AccessToken->PrivilegeCount;
                 assignedIndex++)
            {
                if (RtlEqualLuid(&PreviousState->Privileges[returnedIndex].Luid,
                                 &pPrivileges[assignedIndex].Luid))
                {
                    pPrivileges[assignedIndex].Attributes
                        = PreviousState->Privileges[returnedIndex].Attributes;
                }
            }
        }

        status = STATUS_BUFFER_TOO_SMALL;
    }

    UNLOCK_RWLOCK(&AccessToken->RwLock, isLocked);

    if (status == STATUS_SUCCESS &&
        !DisableAll &&
        adjustedCount < NewState->PrivilegeCount)
    {
        status = STATUS_NOT_ALL_ASSIGNED;
    }

    if (pReturnedLength &&
        (status == STATUS_SUCCESS ||
         status == STATUS_BUFFER_TOO_SMALL ||
         status == STATUS_NOT_ALL_ASSIGNED))
    {
        *pReturnedLength = BufferUsed;
    }
    else
    {
        *pReturnedLength = 0;
    }

    return status;
}
