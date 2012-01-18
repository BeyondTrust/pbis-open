/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software. All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwmapsecurity-lsass.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Access Token Create Information
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lw/base.h>
#include <lw/mapsecurity-plugin.h>
#include <lw/rtlgoto.h>
#include <lsa/lsa.h>
#include <uuid/uuid.h>
#include <lwio/lwio.h>
#include <lw/rpc/samr.h>
#include <lw/rpc/netlogon.h>
#include <lw/rpc/krb5pac.h>
#include <lsaprivilege-internal.h>
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_ext.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"
#include <assert.h>

#define LSA_MAP_SECURITY_MAP_TO_GUEST_RID   DOMAIN_USER_RID_GUEST

#ifndef SAFE_LOG_STRING
#define SAFE_LOG_STRING(String) \
    ( (String) ? (String) : "<null>" )
#endif

typedef struct _LW_MAP_SECURITY_PLUGIN_CONTEXT {
    // TODO-Add connection caching using TLS, a connection pool,
    // or somesuch.  It may be useful to change the calls to LSASS to
    // go through a LsaMapSecurityCallLsass() that is like
    // LsaDmConnectDomain() so we can automatically retry on
    // connection-type errors.
    HANDLE hUnusedConnection;
} LW_MAP_SECURITY_PLUGIN_CONTEXT;

typedef UCHAR LSA_MAP_SECURITY_OBJECT_INFO_FLAGS, *PLSA_MAP_SECURITY_OBJECT_INFO_FLAGS;

#define LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER          0x01
#define LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID        0x02
#define LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID        0x04
#define LSA_MAP_SECURITY_OBJECT_INFO_FLAG_ACCOUNT_DISABLED 0x08

typedef struct _LSA_MAP_SECURITY_OBJECT_INFO {
    LSA_MAP_SECURITY_OBJECT_INFO_FLAGS Flags;
    ULONG Uid;
    ULONG Gid;
    PSID Sid;
    PSID PrimaryGroupSid;
} LSA_MAP_SECURITY_OBJECT_INFO, *PLSA_MAP_SECURITY_OBJECT_INFO;

#define IS_NOT_FOUND_ERROR(lsaError) ( \
    (LW_ERROR_NO_SUCH_USER == (lsaError)) || \
    (LW_ERROR_NO_SUCH_GROUP == (lsaError)) || \
    (LW_ERROR_NO_SUCH_OBJECT == (lsaError)) || \
    0 )

static
NTSTATUS
LsaMapSecurityCreateTokenDefaultDacl(
    PACL *ppDacl,
    PSID pOwnerSid
    );

static
NTSTATUS
LsapMapSecurityGetLocalGuestAccountSid(
    IN HANDLE hLsaConnection,
    OUT PSID* ppGuestSid
    );


static
NTSTATUS
LsaMapSecurityOpenConnection(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PHANDLE phConnection
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hConnection = NULL;

    dwError = LsaOpenServerThreaded(&hConnection);
    status = LsaLsaErrorToNtStatus(dwError);

    *phConnection = hConnection;

    return status;
}

static
VOID
LsaMapSecurityCloseConnection(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PHANDLE phConnection
    )
{
    HANDLE hConnection = *phConnection;

    if (hConnection)
    {
        NTSTATUS status = STATUS_SUCCESS;
        DWORD dwError = LW_ERROR_SUCCESS;

        dwError = LsaCloseServer(hConnection);
        status = LsaLsaErrorToNtStatus(dwError);

        *phConnection = NULL;
    }
}

static
VOID
LsaMapSecurityFreeObjectInfo(
    IN OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    RTL_FREE(&pObjectInfo->Sid);
    RTL_FREE(&pObjectInfo->PrimaryGroupSid);
    RtlZeroMemory(pObjectInfo, sizeof(*pObjectInfo));
}

//
// Use a single "resolve" function so that we can centralize any code
// to (re)connect as well as special-case certain errors.
//

static
NTSTATUS
LsaMapSecurityResolveObjectInfo(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN BOOLEAN IsUser,
    IN OPTIONAL PCSTR pszName,
    IN OPTIONAL PULONG Id,
    OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    ULONG id = Id ? *Id : (ULONG) -1;
    HANDLE hConnection = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_OBJECT_TYPE ObjectType = LSA_OBJECT_TYPE_UNDEFINED;

    if (IS_BOTH_OR_NEITHER(pszName, Id))
    {
        assert(FALSE);
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (IsUser)
    {
        ObjectType = LSA_OBJECT_TYPE_USER;
    }
    else
    {
        ObjectType = LSA_OBJECT_TYPE_GROUP;
    }

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    if (NT_SUCCESS(status))
    {
        if (pszName)
        {
            QueryList.ppszStrings = (PCSTR*) &pszName;

            dwError = LsaFindObjects(
                hConnection,
                NULL,
                0,
                ObjectType,
                LSA_QUERY_TYPE_BY_NAME,
                1,
                QueryList,
                &ppObjects);
        }
        else
        {
            QueryList.pdwIds = &id;

            dwError = LsaFindObjects(
                hConnection,
                NULL,
                0,
                ObjectType,
                LSA_QUERY_TYPE_BY_UNIX_ID,
                1,
                QueryList,
                &ppObjects);
        }

        if (dwError == LW_ERROR_SUCCESS && (ppObjects[0] == NULL || !ppObjects[0]->enabled))
        {
            dwError = LW_ERROR_NO_SUCH_OBJECT;
        }

        LsaMapSecurityCloseConnection(Context, &hConnection);
    }
    else
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }

    if (IS_NOT_FOUND_ERROR(dwError))
    {
        union {
            SID Sid;
            BYTE Buffer[SID_MAX_SIZE];
        } sidBuffer;

        if (pszName)
        {
            status = STATUS_NOT_FOUND;
            GOTO_CLEANUP();
        }

        status = LwMapSecurityInitializeSidFromUnmappedId(
                        sizeof(sidBuffer),
                        &sidBuffer.Sid,
                        IsUser,
                        id);
        GOTO_CLEANUP_ON_STATUS(status);

        if (IsUser)
        {
            SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);
            SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID);
            objectInfo.Uid = id;
        }
        else
        {
            SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);
            objectInfo.Gid = id;
        }

        status = RtlDuplicateSid(&objectInfo.Sid, &sidBuffer.Sid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsUser)
    {
        assert(pszName || ppObjects[0]->userInfo.uid);

        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID);
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);

        if (ppObjects[0]->userInfo.bAccountDisabled)
        {
            SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_ACCOUNT_DISABLED);
        }

        objectInfo.Uid = ppObjects[0]->userInfo.uid;
        objectInfo.Gid = ppObjects[0]->userInfo.gid;
        
        status = RtlAllocateSidFromCString(&objectInfo.Sid, ppObjects[0]->pszObjectSid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlAllocateSidFromCString(
            &objectInfo.PrimaryGroupSid,
            ppObjects[0]->userInfo.pszPrimaryGroupSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        assert(pszName || ppObjects[0]->groupInfo.gid);

        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);

        objectInfo.Gid = ppObjects[0]->groupInfo.gid;

        status = RtlAllocateSidFromCString(&objectInfo.Sid, ppObjects[0]->pszObjectSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeObjectInfo(&objectInfo);
    }

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    *pObjectInfo = objectInfo;

    return status;
}

static
NTSTATUS
LsaMapSecurityResolveObjectInfoBySid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN PSID Sid,
    OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSTR pszSid = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    HANDLE hConnection = NULL;

    status = RtlAllocateCStringFromSid(&pszSid, Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    QueryList.ppszStrings = (PCSTR*) &pszSid;

    dwError = LsaFindObjects(
                    hConnection,
                    NULL,
                    0,
                    LSA_OBJECT_TYPE_UNDEFINED,
                    LSA_QUERY_TYPE_BY_SID,
                    1,
                    QueryList,
                    &ppObjects);

    if (dwError == LW_ERROR_SUCCESS && (ppObjects[0] == NULL || !ppObjects[0]->enabled))
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }

    status = LsaLsaErrorToNtStatus(dwError);
    assert(STATUS_NOT_FOUND != status);
    GOTO_CLEANUP_ON_STATUS(status);

    if (ppObjects[0]->type == LSA_OBJECT_TYPE_USER)
    {
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID);
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);

        objectInfo.Uid = ppObjects[0]->userInfo.uid;
        objectInfo.Gid = ppObjects[0]->userInfo.gid;

        status = RtlAllocateSidFromCString(&objectInfo.Sid, ppObjects[0]->pszObjectSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        SetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);

        objectInfo.Gid = ppObjects[0]->groupInfo.gid;

        status = RtlAllocateSidFromCString(&objectInfo.Sid, ppObjects[0]->pszObjectSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:

    LsaMapSecurityCloseConnection(Context, &hConnection);

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeObjectInfo(&objectInfo);
    }

    LW_SAFE_FREE_STRING(pszSid);

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    *pObjectInfo = objectInfo;

    return status;
}

static
NTSTATUS
LsaMapSecurityConstructSid(
    IN PSID pDomainSid,
    IN ULONG rid,
    OUT PSID* ppSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;
    ULONG ulNewSidLength = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);

    status = RTL_ALLOCATE(&pSid, SID, ulNewSidLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCopySid(ulNewSidLength, pSid, pDomainSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAppendRidSid(ulNewSidLength, pSid, rid);
    GOTO_CLEANUP_ON_STATUS(status);

    *ppSid = pSid;

cleanup:

    if (!NT_SUCCESS(status))
    {
        *ppSid = NULL;
        RTL_FREE(&pSid);
    }
    
    return status;
}

static
NTSTATUS
LsaMapSecurityCompleteObjectInfoFromSid(
    IN HANDLE hLsaConnection,
    IN OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszSid = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;

    status = RtlAllocateCStringFromSid(&pszSid, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    QueryList.ppszStrings = (PCSTR*) &pszSid;

    dwError = LsaFindObjects(
                    hLsaConnection,
                    NULL,
                    0,
                    LSA_OBJECT_TYPE_UNDEFINED,
                    LSA_QUERY_TYPE_BY_SID,
                    1,
                    QueryList,
                    &ppObjects);
    status = LwWin32ErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    SetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);

    if (ppObjects[0] && ppObjects[0]->enabled)
    {
        assert(ppObjects[0]->type == LSA_OBJECT_TYPE_USER);
            
        SetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID);
        SetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID);
        
        pObjectInfo->Uid = ppObjects[0]->userInfo.uid;
        pObjectInfo->Gid = ppObjects[0]->userInfo.gid;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszSid);
    LsaUtilFreeSecurityObjectList(1, ppObjects);

    return status;
}

static
NTSTATUS
LsaMapSecurityResolveObjectInfoFromAuthUserInfo(
    IN HANDLE hLsaConnection,
    IN PLSA_AUTH_USER_INFO pAuthUserInfo,
    OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSID pDomainSid = NULL;

    status = RtlAllocateSidFromCString(&pDomainSid, pAuthUserInfo->pszDomainSid);
    GOTO_CLEANUP_ON_STATUS(status);

    // Sid and PrimaryGroupSid
    status = LsaMapSecurityConstructSid(pDomainSid, 
                                        pAuthUserInfo->dwUserRid, 
                                        &objectInfo.Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityConstructSid(pDomainSid, 
                                        pAuthUserInfo->dwPrimaryGroupRid, 
                                        &objectInfo.PrimaryGroupSid);
    GOTO_CLEANUP_ON_STATUS(status);

    // The rest
    status = LsaMapSecurityCompleteObjectInfoFromSid(hLsaConnection, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeObjectInfo(&objectInfo);
    }

    RTL_FREE(&pDomainSid);

    *pObjectInfo = objectInfo;

    return status;
}

static
NTSTATUS
LsaMapSecurityResolveObjectInfoFromPac(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN PAC_LOGON_INFO* pPac,
    OUT PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    HANDLE hLsaConnection = NULL;
    
    status = LsaMapSecurityConstructSid(
        pPac->info3.base.domain_sid, 
        pPac->info3.base.rid, 
        &objectInfo.Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityConstructSid(
        pPac->info3.base.domain_sid,
        pPac->info3.base.primary_gid,
        &objectInfo.PrimaryGroupSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityOpenConnection(Context, &hLsaConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityCompleteObjectInfoFromSid(hLsaConnection, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeObjectInfo(&objectInfo);
    }

    LsaMapSecurityCloseConnection(Context, &hLsaConnection);

    *pObjectInfo = objectInfo;

    return status;
}

static
NTSTATUS
LsaMapSecurityDuplicateSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PSID* Sid,
    IN PSID OriginalSid
    )
{
    return RtlDuplicateSid(Sid, OriginalSid);
}

static
VOID
LsaMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PSID* Sid
    )
{
    RTL_FREE(Sid);
}

static
VOID
LsaMapSecurityFreeCString(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PSTR* String
    )
{
    LwRtlCStringFree(String);
}

static
NTSTATUS
LsaMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    BOOLEAN isUser = FALSE;
    ULONG id = 0;

    status = LsaMapSecurityResolveObjectInfoBySid(Context, Sid, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    isUser = IsSetFlag(objectInfo.Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_IS_USER);
    id = isUser ? objectInfo.Uid : objectInfo.Gid;

cleanup:
    if (!NT_SUCCESS(status))
    {
        isUser = FALSE;
        id = 0;
    }

    LsaMapSecurityFreeObjectInfo(&objectInfo);

    *IsUser = isUser;
    *Id = id;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSID sid = NULL;

    status = LsaMapSecurityResolveObjectInfo(Context, IsUser, NULL, &Id, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    sid = objectInfo.Sid;
    objectInfo.Sid = NULL;

cleanup:
    LsaMapSecurityFreeObjectInfo(&objectInfo);

    *Sid = sid;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetSidFromName(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN PCSTR pszName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSID sid = NULL;

    status = LsaMapSecurityResolveObjectInfo(Context, IsUser, pszName, NULL, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    sid = objectInfo.Sid;
    objectInfo.Sid = NULL;

cleanup:
    LsaMapSecurityFreeObjectInfo(&objectInfo);

    *Sid = sid;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetNameFromSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PSTR* Domain,
    OUT PSTR* Name,
    OUT PBOOLEAN IsUser,
    IN PSID Sid
    )
{
    DWORD error = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PSTR name = NULL;
    PSTR domain = NULL;
    HANDLE hConnection = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST queryList;
    LSA_OBJECT_TYPE objectType = LSA_OBJECT_TYPE_UNDEFINED;
    PSTR sidString = NULL;
    BOOLEAN isUser = FALSE;

    if (!Sid)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAllocateCStringFromSid(&sidString, Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    queryList.ppszStrings = (PCSTR*) &sidString;

    error = LsaFindObjects(
                hConnection,
                NULL,
                0,
                objectType,
                LSA_QUERY_TYPE_BY_SID,
                1,
                queryList,
                &ppObjects);
    if (error == LW_ERROR_SUCCESS && (ppObjects[0] == NULL || !ppObjects[0]->enabled))
    {
        error = LW_ERROR_NO_SUCH_OBJECT;
    }
    status = LsaLsaErrorToNtStatus(error);
    GOTO_CLEANUP_ON_STATUS(status);

    switch (ppObjects[0]->type)
    {
        case LSA_OBJECT_TYPE_USER:
        case LSA_OBJECT_TYPE_COMPUTER:
            isUser = TRUE;
            break;

        case LSA_OBJECT_TYPE_GROUP:
            isUser = FALSE;
            break;

        default:
            status = LsaLsaErrorToNtStatus(LW_ERROR_NO_SUCH_OBJECT);
            GOTO_CLEANUP_ON_STATUS(status);
    }

    status = LwRtlCStringDuplicate(
                 &domain,
                 SAFE_LOG_STRING(ppObjects[0]->pszNetbiosDomainName));
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlCStringDuplicate(
                 &name,
                 SAFE_LOG_STRING(ppObjects[0]->pszSamAccountName));
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        isUser = FALSE;

        if (domain)
        {
            LwRtlCStringFree(&domain);
            name = NULL;
        }

        if (name)
        {
            LwRtlCStringFree(&name);
            name = NULL;
        }
    }

    *Domain = domain;
    *Name = name;
    *IsUser = isUser;

    if (sidString)
    {
        LwRtlCStringFree(&sidString);
    }

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(1, ppObjects);
    }

    return status;
}

static
VOID
LsaMapSecurityFreeAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation
    )
{
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = *CreateInformation;

    if (createInformation)
    {
        ULONG i = 0;

        RTL_FREE(&createInformation->User->User.Sid);
        for (i = 0; i < createInformation->Groups->GroupCount; i++)
        {
            RTL_FREE(&createInformation->Groups->Groups[i].Sid);
        }
        RTL_FREE(&createInformation->Owner->Owner);
        RTL_FREE(&createInformation->PrimaryGroup->PrimaryGroup);
        RTL_FREE(&createInformation->DefaultDacl->DefaultDacl);
        RTL_FREE(&createInformation);
        *CreateInformation = NULL;
    }
}


static
NTSTATUS
LsaMapSecurityAllocateAccessTokenCreateInformation(
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN ULONG GroupCount,
    IN ULONG PrivilegeCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    ULONG size = 0;
    ULONG privilegesSize = 0;
    PVOID location = NULL;

    //
    // Compute size for everything except the actual SIDs.  This includes
    // the flexible arrays using GroupCount and PrivilegeCount.
    //

    status = RtlSafeMultiplyULONG(&size, GroupCount, sizeof(createInformation->Groups->Groups[0]));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->User));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->Groups));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->Privileges));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeMultiplyULONG(
                         &privilegesSize,
                         PrivilegeCount,
                         sizeof(createInformation->Privileges->Privileges[0]));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, privilegesSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->Owner));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->PrimaryGroup));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->DefaultDacl));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&size, size, sizeof(*createInformation->Unix));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&createInformation, ACCESS_TOKEN_CREATE_INFORMATION, size);
    GOTO_CLEANUP_ON_STATUS(status);

    location = createInformation;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation));

    createInformation->User = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->User));

    createInformation->Groups = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Groups));
    location = LwRtlOffsetToPointer(location, GroupCount * sizeof(createInformation->Groups->Groups[0]));

    createInformation->Privileges = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Privileges));
    location = LwRtlOffsetToPointer(
                         location,
                         PrivilegeCount *
                         sizeof(createInformation->Privileges->Privileges[0]));

    createInformation->Owner = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Owner));

    createInformation->PrimaryGroup = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->PrimaryGroup));

    createInformation->DefaultDacl = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->DefaultDacl));

    createInformation->Unix = location;
    location = LwRtlOffsetToPointer(location, sizeof(*createInformation->Unix));

    assert(LwRtlOffsetToPointer(createInformation, size) == location);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&createInformation);
    }

    *CreateInformation = createInformation;

    return status;
}

static
BOOLEAN
LsaMapSecurityIsGidInGidList(
    IN ULONG Gid,
    IN ULONG GidCount,
    IN PULONG pGidList
    )
{
    BOOLEAN isFound = FALSE;
    ULONG i = 0;

    for (i = 0; i < GidCount; i++)
    {
        if (Gid == pGidList[i])
        {
            isFound = TRUE;
            break;
        }
    }

    return isFound;
}

static
BOOLEAN
LsaMapSecurityIsGidInGroupInfoList(
    IN ULONG Gid,
    IN ULONG GroupInfoCount,
    IN PLSA_SECURITY_OBJECT* ppGroupObjects
    )
{
    BOOLEAN isFound = FALSE;
    ULONG i = 0;

    for (i = 0; i < GroupInfoCount; i++)
    {
        if (ppGroupObjects[i] && Gid == ppGroupObjects[i]->groupInfo.gid)
        {
            isFound = TRUE;
            break;
        }
    }

    return isFound;
}

static
VOID
LsaMapSecurityAddExtraGid(
    IN ULONG Gid,
    IN OUT PULONG ExtraGidCount,
    IN OUT PULONG ExtraGidList,
    IN ULONG ExtraGidMaximumCount,
    IN ULONG GroupInfoCount,
    IN PLSA_SECURITY_OBJECT* ppGroupObjects
    )
{
    ULONG extraGidCount = *ExtraGidCount;

    if (extraGidCount < ExtraGidMaximumCount)
    {
        if (!LsaMapSecurityIsGidInGidList(Gid, extraGidCount, ExtraGidList) &&
            !LsaMapSecurityIsGidInGroupInfoList(Gid, GroupInfoCount, ppGroupObjects))
        {
            ExtraGidList[extraGidCount] = Gid;
            *ExtraGidCount = extraGidCount + 1;
        }
    }
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromObjectInfo(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo,
    IN OPTIONAL PULONG Gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    ULONG i = 0;
    ULONG gid = Gid ? *Gid : 0;
    ULONG extraGidList[2] = { 0 };
    PSID extraGidSidList[LW_ARRAY_SIZE(extraGidList)] = { 0 };
    ULONG extraGidCount = 0;
    HANDLE hConnection = NULL;
    DWORD dwGroupCount = 0;
    ULONG privilegesSidListCount = 0;
    PSTR* ppszPrivilegesSidList = NULL;
    PLUID_AND_ATTRIBUTES pPrivileges = NULL;
    DWORD privilegeCount = 0;
    DWORD systemAccessRights = 0;
    PSTR* ppszGroupSids = NULL;
    PSTR pszSid = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;

    if (IsSetFlag(
            pObjectInfo->Flags,
            LSA_MAP_SECURITY_OBJECT_INFO_FLAG_ACCOUNT_DISABLED))
    {
        status = STATUS_ACCOUNT_DISABLED;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlAllocateCStringFromSid(&pszSid, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    dwError = LsaQueryMemberOf(
                    hConnection,
                    NULL,
                    0,
                    1,
                    &pszSid,
                    &dwGroupCount,
                    &ppszGroupSids);
    if (IS_NOT_FOUND_ERROR(dwError))
    {
        dwError = 0;
        dwGroupCount = 0;
    }
    else
    {
        status = LsaLsaErrorToNtStatus(dwError);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (dwGroupCount)
    {
        QueryList.ppszStrings = (PCSTR*) ppszGroupSids;

        dwError = LsaFindObjects(
            hConnection,
            NULL,
            0,
            LSA_OBJECT_TYPE_GROUP,
            LSA_QUERY_TYPE_BY_SID,
            dwGroupCount,
            QueryList,
            &ppObjects);
        status = LsaLsaErrorToNtStatus(dwError);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    //
    // Take into account extra GIDs that came as primary GID from
    // object info or from passed in primary GID.
    //

    if (IsSetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID))
    {
        LsaMapSecurityAddExtraGid(
                pObjectInfo->Gid,
                &extraGidCount,
                extraGidList,
                LW_ARRAY_SIZE(extraGidList),
                dwGroupCount,
                ppObjects);
    }

    if (Gid)
    {
        LsaMapSecurityAddExtraGid(
                gid,
                &extraGidCount,
                extraGidList,
                LW_ARRAY_SIZE(extraGidList),
                dwGroupCount,
                ppObjects);
    }

    //
    // Resolve extra GIDs into SIDs
    //

    for (i = 0; i < extraGidCount; i++)
    {
        status = LsaMapSecurityGetSidFromId(
                        Context,
                        &extraGidSidList[i],
                        FALSE,
                        extraGidList[i]);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    //
    // Get the privileges list given user SID and all SIDs
    // in the user's access token
    //

    privilegesSidListCount = dwGroupCount + extraGidCount + 1;
    status = RTL_ALLOCATE(
                   &ppszPrivilegesSidList,
                   PSTR,
                   sizeof(ppszPrivilegesSidList[0]) * privilegesSidListCount);
    GOTO_CLEANUP_ON_STATUS(status);

    // Group SIDs
    for (i = 0; i < dwGroupCount; i++)
    {
        status = RtlCStringDuplicate(
                    &ppszPrivilegesSidList[i],
                    ppszGroupSids[i]);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Extra GIDs
    for (i = 0; i < extraGidCount; i++)
    {
        status = RtlAllocateCStringFromSid(
                    &ppszPrivilegesSidList[dwGroupCount + i],
                    extraGidSidList[i]);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // User SID
    status = RtlCStringDuplicate(
                &ppszPrivilegesSidList[dwGroupCount + extraGidCount],
                pszSid);
    GOTO_CLEANUP_ON_STATUS(status);

    dwError = LsaPrivsEnumAccountRightsSids(
                hConnection,
                ppszPrivilegesSidList,
                privilegesSidListCount,
                &pPrivileges,
                &privilegeCount,
                &systemAccessRights);
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    LsaMapSecurityCloseConnection(Context, &hConnection);
    hConnection = NULL;

    //
    // Allocate token create information with enough space
    // for any potential extra GIDs.
    //

    status = LsaMapSecurityAllocateAccessTokenCreateInformation(
                    &createInformation,
                    dwGroupCount + extraGidCount,
                    privilegeCount);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_UNIX
    //

    createInformation->Unix->Uid = pObjectInfo->Uid;

    // TODO-Should the passed in GID take precedence over the LSASS one?
    if (IsSetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID))
    {
        createInformation->Unix->Gid = pObjectInfo->Gid;
    }
    else if (Gid)
    {
        createInformation->Unix->Gid = gid;
    }
    else
    {
        // TODO-Would need to put in some nobody-like gid.
        assert(FALSE);
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    // TODO-Should the token even have a umask, and if so,
    // where should it come from?
    createInformation->Unix->Umask = 0022;

    //
    // TOKEN_USER
    //

    status = RtlDuplicateSid(&createInformation->User->User.Sid, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_GROUPS
    //

    for (i = 0; i < dwGroupCount; i++)
    {
        PSID_AND_ATTRIBUTES group = &createInformation->Groups->Groups[createInformation->Groups->GroupCount];

        if (ppObjects[i])
        {
            status = RtlAllocateSidFromCString(
                &group->Sid,
                ppObjects[i]->pszObjectSid);
            GOTO_CLEANUP_ON_STATUS(status);
            
            group->Attributes = SE_GROUP_ENABLED;
            
            createInformation->Groups->GroupCount++;
        }
    }

    for (i = 0; i < extraGidCount; i++)
    {
        PSID_AND_ATTRIBUTES group = &createInformation->Groups->Groups[createInformation->Groups->GroupCount];

        group->Sid = extraGidSidList[i];
        extraGidSidList[i] = NULL;

        group->Attributes = SE_GROUP_ENABLED;

        createInformation->Groups->GroupCount++;
    }

    //
    // TOKEN_PRIVILEGES
    //

    for (i = 0; i < privilegeCount; i++)
    {
        PLUID_AND_ATTRIBUTES privilege = &createInformation->Privileges->Privileges[i];

        privilege->Luid = pPrivileges[i].Luid;
        privilege->Attributes = pPrivileges[i].Attributes;
    }

    createInformation->Privileges->PrivilegeCount = privilegeCount;

    //
    // TOKEN_OWNER
    //

    status = RtlDuplicateSid(&createInformation->Owner->Owner, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_PRIMARY_GROUP
    //

    if (pObjectInfo->PrimaryGroupSid)
    {
        status = RtlDuplicateSid(
            &createInformation->PrimaryGroup->PrimaryGroup,
            pObjectInfo->PrimaryGroupSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = LsaMapSecurityGetSidFromId(
            Context,
            &createInformation->PrimaryGroup->PrimaryGroup,
            FALSE,
            createInformation->Unix->Gid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    //
    // TOKEN_DEFAULT_DACL
    //

    status = LsaMapSecurityCreateTokenDefaultDacl(
                 &createInformation->DefaultDacl->DefaultDacl,
                 createInformation->Owner->Owner);
    GOTO_CLEANUP_ON_STATUS(status);


cleanup:
    if (hConnection)
    {
        LsaMapSecurityCloseConnection(Context, &hConnection);
    }

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);
    }

    LwFreeStringArray(ppszGroupSids, dwGroupCount);
    LsaUtilFreeSecurityObjectList(dwGroupCount, ppObjects);

    if (ppszPrivilegesSidList)
    {
        for (i = 0; i < privilegesSidListCount; i++)
        {
            RTL_FREE(&ppszPrivilegesSidList[i]);
        }
    }
    RTL_FREE(&ppszPrivilegesSidList);

    RTL_FREE(&pPrivileges);

    for (i = 0; i < extraGidCount; i++)
    {
        RTL_FREE(&extraGidSidList[i]);
    }

    RTL_FREE(&pszSid);

    *CreateInformation = createInformation;

    return status;
}

static
NTSTATUS
LsaMapSecurityMergeStringLists(
    IN DWORD dwCount1,
    IN PSTR* ppszStrings1,
    IN DWORD dwCount2,
    IN PSTR* ppszStrings2,
    OUT PDWORD pdwMergedCount,
    OUT PSTR** pppszMergedStrings
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR* ppszMergedStrings = NULL;
    DWORD dwMergedCount = dwCount1;
    DWORD dwSecondIndex = 0;
    DWORD dwFirstIndex = 0;
    BOOLEAN bFound = FALSE;

    status = RTL_ALLOCATE(&ppszMergedStrings, PSTR, sizeof(PSTR) * (dwCount1 + dwCount2));
    GOTO_CLEANUP_ON_STATUS(status);

    memcpy(ppszMergedStrings, ppszStrings1, sizeof(*ppszStrings1) * dwCount1);
    
    for (dwSecondIndex = 0; dwSecondIndex < dwCount2; dwSecondIndex++)
    {
        bFound = FALSE;
        for (dwFirstIndex = 0; dwFirstIndex < dwCount1; dwFirstIndex++)
        {
            if (!strcasecmp(ppszStrings1[dwFirstIndex], ppszStrings2[dwSecondIndex]))
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            ppszMergedStrings[dwMergedCount++] = ppszStrings2[dwSecondIndex];
        }
    }

    *pdwMergedCount = dwMergedCount;
    *pppszMergedStrings = ppszMergedStrings;

cleanup:

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(ppszMergedStrings);
    }

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromObjectInfoAndGroups(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PLSA_MAP_SECURITY_OBJECT_INFO pObjectInfo,
    IN DWORD dwInputSidCount,
    IN PSID* ppInputSids
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    ULONG i = 0;
    HANDLE hConnection = NULL;
    PSTR* ppszInputSids = NULL;
    PSTR* ppszSuppGroupSids = NULL;
    PSTR* ppszGroupSids = NULL;
    DWORD dwGroupCount = 0;
    DWORD dwSuppGroupCount = 0;
    DWORD privilegeCount = 0;
    PLUID_AND_ATTRIBUTES pPrivileges = NULL;
    DWORD systemAccessRights = 0;
    PSID pPrimaryGidSid = NULL;
    PSTR pszPrimaryGidSid = NULL;
    
    /* Allocate array for string forms of sids, plus extra slots for user sid and sid from primary UNIX gid */
    status = RTL_ALLOCATE(&ppszInputSids, PSTR, sizeof(*ppszInputSids) * (dwInputSidCount + 2));
    GOTO_CLEANUP_ON_STATUS(status);

    for (i = 0; i < dwInputSidCount; i++)
    {
        status = RtlAllocateCStringFromSid(&ppszInputSids[i], ppInputSids[i]);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Add user SID itself to list */
    status = RtlAllocateCStringFromSid(&ppszInputSids[dwInputSidCount++], pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityOpenConnection(Context, &hConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsSetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID))
    {
        status = LsaMapSecurityGetSidFromId(
                        Context,
                        &pPrimaryGidSid,
                        FALSE,
                        pObjectInfo->Gid);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlAllocateCStringFromSid(&pszPrimaryGidSid, pPrimaryGidSid);
        GOTO_CLEANUP_ON_STATUS(status);
        
        for (i = 0; i < dwInputSidCount; i++)
        {
            if (!strcasecmp(ppszInputSids[i], pszPrimaryGidSid))
            {
                break;
            }
        }
         
        if (i == dwInputSidCount)
        {
            ppszInputSids[dwInputSidCount++] = pszPrimaryGidSid;
            pszPrimaryGidSid = NULL;
        }
    }

    dwError = LsaQueryMemberOf(
                    hConnection,
                    NULL,
                    LSA_FIND_FLAGS_LOCAL,
                    dwInputSidCount,
                    ppszInputSids,
                    &dwSuppGroupCount,
                    &ppszSuppGroupSids);
    if (IS_NOT_FOUND_ERROR(dwError))
    {
        dwError = 0;
        dwGroupCount = 0;
    }
    else
    {
        status = LsaLsaErrorToNtStatus(dwError);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = LsaMapSecurityMergeStringLists(
        dwInputSidCount,
        ppszInputSids,
        dwSuppGroupCount,
        ppszSuppGroupSids,
        &dwGroupCount,
        &ppszGroupSids);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // Get the list of privileges given the list of SIDs
    //

    dwError = LsaPrivsEnumAccountRightsSids(
        hConnection,
        ppszGroupSids,
        dwGroupCount,
        &pPrivileges,
        &privilegeCount,
        &systemAccessRights);
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);
    
    LsaMapSecurityCloseConnection(Context, &hConnection);
    hConnection = NULL;

    //
    // Allocate token create information with enough space
    // for any potential extra GIDs.
    //

    status = LsaMapSecurityAllocateAccessTokenCreateInformation(
        &createInformation,
        dwGroupCount,
        privilegeCount);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_UNIX
    //

    if (IsSetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_UID) && 
        IsSetFlag(pObjectInfo->Flags, LSA_MAP_SECURITY_OBJECT_INFO_FLAG_VALID_GID))
    {
        createInformation->Unix->Uid = pObjectInfo->Uid;
        createInformation->Unix->Gid = pObjectInfo->Gid;

        // TODO-Should the token even have a umask, and if so,
        // where should it come from?
        createInformation->Unix->Umask = 0022;
    }
    else
    {
        createInformation->Unix = NULL;
    }
    

    //
    // TOKEN_USER
    //

    status = RtlDuplicateSid(&createInformation->User->User.Sid, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_GROUPS
    //

    for (i = 0; i < dwGroupCount; i++)
    {
        PSID_AND_ATTRIBUTES group = &createInformation->Groups->Groups[createInformation->Groups->GroupCount];

        status = RtlAllocateSidFromCString(
            &group->Sid,
            ppszGroupSids[i]);
        GOTO_CLEANUP_ON_STATUS(status);
        
        group->Attributes = SE_GROUP_ENABLED;
        
        createInformation->Groups->GroupCount++;
    }

    //
    // TOKEN_PRIVILEGES
    //

    for (i = 0; i < privilegeCount; i++)
    {
        PLUID_AND_ATTRIBUTES privilege = &createInformation->Privileges->Privileges[i];

        privilege->Luid       = pPrivileges[i].Luid;
        privilege->Attributes = pPrivileges[i].Attributes;
    }

    createInformation->Privileges->PrivilegeCount = privilegeCount;

    //
    // TOKEN_OWNER
    //

    status = RtlDuplicateSid(&createInformation->Owner->Owner, pObjectInfo->Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // TOKEN_PRIMARY_GROUP
    //

    if (pObjectInfo->PrimaryGroupSid)
    {
        status = RtlDuplicateSid(
            &createInformation->PrimaryGroup->PrimaryGroup,
            pObjectInfo->PrimaryGroupSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = LsaMapSecurityGetSidFromId(
            Context,
            &createInformation->PrimaryGroup->PrimaryGroup,
            FALSE,
            createInformation->Unix->Gid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    //
    // TOKEN_DEFAULT_DACL
    //

    status = LsaMapSecurityCreateTokenDefaultDacl(
                 &createInformation->DefaultDacl->DefaultDacl,
                 createInformation->Owner->Owner);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (hConnection)
    {
        LsaMapSecurityCloseConnection(Context, &hConnection);
    }

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);
    }

    RTL_FREE(&pszPrimaryGidSid);
    RTL_FREE(&pPrimaryGidSid);
    RTL_FREE(&ppszGroupSids);
    LwFreeStringArray(ppszSuppGroupSids, dwSuppGroupCount);
    LwFreeStringArray(ppszInputSids, dwInputSidCount);

    *CreateInformation = createInformation;

    return status;
}


static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformation(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN OPTIONAL PUNICODE_STRING Username,
    IN OPTIONAL PULONG Uid,
    IN OPTIONAL PULONG Gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN_CREATE_INFORMATION createInformation = NULL;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = { 0 };
    PSTR pszUsername = NULL;

    if (IS_BOTH_OR_NEITHER(Username, Uid) ||
        (Gid && !Uid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (Username)
    {
        status = LwRtlCStringAllocateFromUnicodeString(&pszUsername, Username);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = LsaMapSecurityResolveObjectInfo(
                    Context,
                    TRUE,
                    pszUsername,
                    Uid,
                    &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityGetAccessTokenCreateInformationFromObjectInfo(
                    Context,
                    &createInformation,
                    &objectInfo,
                    Gid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &createInformation);
    }

    LwRtlCStringFree(&pszUsername);
    LsaMapSecurityFreeObjectInfo(&objectInfo);

    *CreateInformation = createInformation;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetPacInfoFromGssContext(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PAC_LOGON_INFO** ppPac,
    IN LW_MAP_SECURITY_GSS_CONTEXT GssContext
    )
{
    static const CHAR szLogonInfoUrn[] = "urn:mspac:logon-info";
    NTSTATUS status = STATUS_SUCCESS;
    OM_uint32 minorStatus = 0;
    OM_uint32 majorStatus = 0;
    gss_name_t srcName = GSS_C_NO_NAME;
    gss_buffer_desc logonInfoUrn =
    {
        .length = sizeof(szLogonInfoUrn) - 1,
        .value = (PBYTE) szLogonInfoUrn
    };
    gss_buffer_desc pacData = {0};
    gss_buffer_desc displayData = {0};
    PAC_LOGON_INFO *pPac = NULL;
    int more = -1;

    majorStatus = gss_inquire_context(
        &minorStatus,
        GssContext,
        &srcName,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    if (majorStatus != GSS_S_COMPLETE)
    {
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS(status);
    }
    
    majorStatus = gss_get_name_attribute(
        &minorStatus,
        srcName,
        &logonInfoUrn,
        NULL,
        NULL,
        &pacData,
        &displayData,
        &more);
    if (majorStatus != GSS_S_COMPLETE)
    {
        // Try to convert the minorStatus as a Win32 error.  At worst we'll end
        // up with a squashed error code if it is not a valid Win32 error. But to
        // be fair we were always returning STATUS_UNSUCCESSFUL here before.

        status = LwWin32ErrorToNtStatus(minorStatus);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    
    if (pacData.value == NULL)
    {
        status = STATUS_INVALID_USER_BUFFER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = DecodePacLogonInfo(
        pacData.value,
        pacData.length,
        &pPac);
    GOTO_CLEANUP_ON_STATUS(status);

    *ppPac = pPac;

cleanup:

    if (!NT_SUCCESS(status))
    {
        *ppPac = NULL;
        if (pPac)
        {
            FreePacLogonInfo(pPac);
        }
    }

    if (pacData.value)
    {
        gss_release_buffer(&minorStatus, &pacData);
    }

    if (displayData.value)
    {
        gss_release_buffer(&minorStatus, &displayData);
    }

    if (srcName)
    {
        gss_release_name(&minorStatus, &srcName);
    }

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromUid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN ULONG Uid,
    IN PULONG Gid
    )
{
    return LsaMapSecurityGetAccessTokenCreateInformation(
                Context,
                CreateInformation,
                NULL,
                &Uid,
                Gid);
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromUsername(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN PUNICODE_STRING Username
    )
{
    return LsaMapSecurityGetAccessTokenCreateInformation(
                Context,
                CreateInformation,
                Username,
                NULL,
                NULL);
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromGssContext(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* CreateInformation,
    IN LW_MAP_SECURITY_GSS_CONTEXT GssContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PAC_LOGON_INFO* pPac = NULL;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = {0};
    DWORD dwInputSidCount = 0;
    PSID* ppInputSids = NULL;
    DWORD dwIndex = 0;
    PSID pSid = NULL;

    status = LsaMapSecurityGetPacInfoFromGssContext(
        Context,
        &pPac,
        GssContext);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsaMapSecurityResolveObjectInfoFromPac(Context, pPac, &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&ppInputSids, PSID, sizeof(PSID) * 
                          (pPac->info3.base.groups.dwCount +
                           pPac->res_groups.dwCount +
                           pPac->info3.sidcount));
    GOTO_CLEANUP_ON_STATUS(status);

    for (dwIndex = 0; dwIndex < pPac->info3.base.groups.dwCount; dwIndex++)
    {
        status = LsaMapSecurityConstructSid(
            pPac->info3.base.domain_sid,
            pPac->info3.base.groups.pRids[dwIndex].dwRid,
            &pSid);
        GOTO_CLEANUP_ON_STATUS(status);

        ppInputSids[dwInputSidCount++] = pSid;
        pSid = NULL;
    }

    for (dwIndex = 0; dwIndex < pPac->res_groups.dwCount; dwIndex++)
    {
        status = LsaMapSecurityConstructSid(
            pPac->res_group_dom_sid,
            pPac->res_groups.pRids[dwIndex].dwRid,
            &pSid);
        GOTO_CLEANUP_ON_STATUS(status);

        ppInputSids[dwInputSidCount++] = pSid;
        pSid = NULL;
    }
   
    for (dwIndex = 0; dwIndex < pPac->info3.sidcount; dwIndex++)
    {
        status = RtlDuplicateSid(&pSid, pPac->info3.sids[dwIndex].sid);
        GOTO_CLEANUP_ON_STATUS(status);

        ppInputSids[dwInputSidCount++] = pSid;
        pSid = NULL;
    }

    status = LsaMapSecurityGetAccessTokenCreateInformationFromObjectInfoAndGroups(
        Context,
        CreateInformation,
        &objectInfo,
        dwInputSidCount,
        ppInputSids);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    RTL_FREE(&pSid);

    for (dwIndex = 0; dwIndex < dwInputSidCount; dwIndex++)
    {
        RTL_FREE(&ppInputSids[dwIndex]);
    }

    RTL_FREE(&ppInputSids);

    if (pPac)
    {
        FreePacLogonInfo(pPac);
    }

    LsaMapSecurityFreeObjectInfo(&objectInfo);    
    
    return status;
}

static
VOID
LsaMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_PLUGIN_CONTEXT* Context
    )
{
    PLW_MAP_SECURITY_PLUGIN_CONTEXT context = *Context;

    if (context)
    {
        RTL_FREE(&context);
        *Context = NULL;
    }
}

static
VOID
LsaMapSecurityFreeNtlmLogonResult(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    IN OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmResult
    )
{
    PLW_MAP_SECURITY_NTLM_LOGON_RESULT pNtlmResult = *ppNtlmResult;

    if (pNtlmResult)
    {
        if (pNtlmResult->pszUsername)
        {
            LwRtlCStringFree(&pNtlmResult->pszUsername);
        }
        RTL_FREE(&pNtlmResult);

        *ppNtlmResult = pNtlmResult;
    }
}

static
NTSTATUS
LsaMapSecurityGetLocalGuestAccountSid(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT pContext,
    OUT PSID* ppGuestSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PSID pGuestSid = NULL;

    status = LsaMapSecurityOpenConnection(pContext, &hLsaConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LsapMapSecurityGetLocalGuestAccountSid(
                    hLsaConnection,
                    &pGuestSid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeSid(pContext, &pGuestSid);
    }

    LsaMapSecurityCloseConnection(pContext, &hLsaConnection);

    *ppGuestSid = pGuestSid;

    return status;
}

static
NTSTATUS
LsaMapSecurityGetGuestAuthUserInfo(
    IN HANDLE hLsaConnection,
    OUT PLSA_AUTH_USER_INFO* ppUserInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    PSID pGuestSid = NULL;
    PSTR pszGuestSid = NULL;
    LSA_QUERY_LIST queryList = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwGroupCount = 0;
    PSTR* ppszGroupSids = NULL;
    LSA_AUTH_USER_INFO userInfo = { 0 };
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    PSID pSid = NULL;
    PSID pDomainSid = NULL;
    int i = 0;

    status = LsapMapSecurityGetLocalGuestAccountSid(
                    hLsaConnection,
                    &pGuestSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAllocateCStringFromSid(&pszGuestSid, pGuestSid);
    GOTO_CLEANUP_ON_STATUS(status);

    queryList.ppszStrings = (PCSTR*)&pszGuestSid;

    dwError = LsaFindObjects(
                  hLsaConnection,
                  LSA_PROVIDER_TAG_LOCAL,
                  0,
                  LSA_OBJECT_TYPE_USER,
                  LSA_QUERY_TYPE_BY_SID,
                  1,
                  queryList,
                  &ppObjects);
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    if (ppObjects[0]->userInfo.bAccountDisabled)
    {
        status = STATUS_ACCOUNT_DISABLED;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Build LSA_AUTH_USER_INFO from LSA_SECURITY_OBJECT, at least the info we
    // need for NTLM logon
    dwError = LsaQueryMemberOf(
                    hLsaConnection,
                    LSA_PROVIDER_TAG_LOCAL,
                    0,
                    1,
                    &ppObjects[0]->pszObjectSid,
                    &dwGroupCount,
                    &ppszGroupSids);
    if (IS_NOT_FOUND_ERROR(dwError))
    {
        dwError = 0;
    }
    else
    {
        status = LsaLsaErrorToNtStatus(dwError);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pUserInfo, LSA_AUTH_USER_INFO, sizeof(*pUserInfo));
    GOTO_CLEANUP_ON_STATUS(status);

    *pUserInfo = userInfo;  // zero all fields

    // Fill pUserInfo only with data needed in the caller function
    // Session key
    dwError = LsaDataBlobAllocate(&pUserInfo->pSessionKey, NTLM_SESSION_KEY_SIZE);
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    RtlZeroMemory(pUserInfo->pSessionKey->pData, NTLM_SESSION_KEY_SIZE);

    // UPN
    if (ppObjects[0]->userInfo.pszUPN)
    {
        status = RtlCStringDuplicate(&pUserInfo->pszUserPrincipalName,
                                     ppObjects[0]->userInfo.pszUPN);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Account
    if (ppObjects[0]->pszSamAccountName)
    {
        status = RtlCStringDuplicate(&pUserInfo->pszAccount,
                                     ppObjects[0]->pszSamAccountName);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // UserRid
    status = RtlAllocateSidFromCString(&pSid, ppObjects[0]->pszObjectSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlGetRidSid(&pUserInfo->dwUserRid, pSid);
    GOTO_CLEANUP_ON_STATUS(status);

    // Domain SID
    status = RtlDuplicateSid(&pDomainSid, pSid);
    GOTO_CLEANUP_ON_STATUS(status);

    pDomainSid->SubAuthorityCount--;

    status = RtlAllocateCStringFromSid(&pUserInfo->pszDomainSid, pDomainSid);
    GOTO_CLEANUP_ON_STATUS(status);

    // Primary group RID
    RTL_FREE(&pSid);
    status = RtlAllocateSidFromCString(&pSid, ppObjects[0]->userInfo.pszPrimaryGroupSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlGetRidSid(&pUserInfo->dwPrimaryGroupRid, pSid);
    GOTO_CLEANUP_ON_STATUS(status);
    
    // Groups
    pUserInfo->dwNumRids = 0;
    pUserInfo->dwNumSids = dwGroupCount;

    status = RTL_ALLOCATE(&pUserInfo->pSidAttribList, LSA_SID_ATTRIB, 
                          sizeof(*pUserInfo->pSidAttribList) * dwGroupCount);
    GOTO_CLEANUP_ON_STATUS(status);

    // for the case we fail to allocate somewhere in the
    // middle of the next loop
    RtlZeroMemory(pUserInfo->pSidAttribList, 
                  sizeof(*pUserInfo->pSidAttribList) * dwGroupCount);    

    for (i = 0; i < pUserInfo->dwNumSids; ++i)
    {
        status = RtlCStringDuplicate(&pUserInfo->pSidAttribList[i].pszSid,
                                     ppszGroupSids[i]);
        GOTO_CLEANUP_ON_STATUS(status);

        pUserInfo->pSidAttribList[i].dwAttrib = 
            LSA_SID_ATTR_GROUP_MANDATORY |
            LSA_SID_ATTR_GROUP_ENABLED_BY_DEFAULT |
            LSA_SID_ATTR_GROUP_ENABLED;        
    }


    *ppUserInfo = pUserInfo;

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaFreeAuthUserInfo(&pUserInfo);
    }

    RTL_FREE(&pSid);
    RTL_FREE(&pDomainSid);
    RTL_FREE(&pGuestSid);
    RTL_FREE(&pszGuestSid);
    LwFreeStringArray(ppszGroupSids, dwGroupCount);
    LsaUtilFreeSecurityObjectList(1, ppObjects);

    return status;
}

static
NTSTATUS
LsaMapSecurityGetAccessTokenCreateInformationFromNtlmLogon(
    IN PLW_MAP_SECURITY_PLUGIN_CONTEXT Context,
    OUT PACCESS_TOKEN_CREATE_INFORMATION* ppCreateInformation,
    IN PLW_MAP_SECURITY_NTLM_LOGON_INFO pNtlmInfo,
    OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = NULL;
    LSA_AUTH_USER_PARAMS lsaUserParams = { 0 };
    LW_LSA_DATA_BLOB Challenge = { 0 };
    LW_LSA_DATA_BLOB LMResp = { 0 };
    LW_LSA_DATA_BLOB NTResp = { 0 };
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    PLW_MAP_SECURITY_NTLM_LOGON_RESULT pNtlmResult = NULL;
    PACCESS_TOKEN_CREATE_INFORMATION pCreateInformation = NULL;
    LSA_MAP_SECURITY_OBJECT_INFO objectInfo = {0};
    DWORD dwInputSidCount = 0;
    PSID* ppInputSids = NULL;
    PSID pDomainSid = NULL;
    DWORD dwIndex = 0;
    PSID pSid = NULL;

    status = LsaMapSecurityOpenConnection(Context, &hLsaConnection);
    GOTO_CLEANUP_ON_STATUS(status);

    // Create LSA_AUTH_USER_PARAMS
    Challenge.dwLen = sizeof(pNtlmInfo->Challenge);
    Challenge.pData = pNtlmInfo->Challenge;
    LMResp.dwLen = pNtlmInfo->ulLmResponseSize;
    LMResp.pData = pNtlmInfo->pLmResponse;
    NTResp.dwLen = pNtlmInfo->ulNtResponseSize;
    NTResp.pData = pNtlmInfo->pNtResponse;

    status = LwRtlCStringAllocateFromWC16String(
                &lsaUserParams.pszAccountName,
                pNtlmInfo->pwszAccountName);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwRtlCStringAllocateFromWC16String(
                &lsaUserParams.pszDomain,
                pNtlmInfo->pwszDomain);
    GOTO_CLEANUP_ON_STATUS(status);
    
    lsaUserParams.AuthType = LSA_AUTH_CHAP;
    lsaUserParams.pszWorkstation = NULL;
    lsaUserParams.pass.chap.pChallenge = &Challenge;
    lsaUserParams.pass.chap.pLM_resp = &LMResp;
    lsaUserParams.pass.chap.pNT_resp = &NTResp;

    status = RTL_ALLOCATE(&pNtlmResult, LW_MAP_SECURITY_NTLM_LOGON_RESULT, 
                          sizeof(*pNtlmResult));
    GOTO_CLEANUP_ON_STATUS(status);

    // Authenticate
    dwError = LsaAuthenticateUserEx(
                  hLsaConnection,
                  NULL,
                  &lsaUserParams,
                  &pUserInfo);
    if (dwError == LW_ERROR_NOT_HANDLED)
    {
        // Attempt to fallback to Guest access
        status = LsaMapSecurityGetGuestAuthUserInfo(hLsaConnection, &pUserInfo);
        pNtlmResult->bMappedToGuest = TRUE;
    }
    else
    {
        status = LsaLsaErrorToNtStatus(dwError);
    }
    GOTO_CLEANUP_ON_STATUS(status);

    // Copy data to the result struct
    if (pUserInfo->pszUserPrincipalName)
    {
        status = LwRtlCStringDuplicate(
                    &pNtlmResult->pszUsername, 
                    pUserInfo->pszUserPrincipalName);
    }
    else if (pUserInfo->pszAccount)
    {
        status = LwRtlCStringDuplicate(
                    &pNtlmResult->pszUsername, 
                    pUserInfo->pszAccount);
    }
    GOTO_CLEANUP_ON_STATUS(status);
    
    assert(pUserInfo->pSessionKey->dwLen == NTLM_SESSION_KEY_SIZE);    
    RtlCopyMemory(pNtlmResult->SessionKey, 
                  pUserInfo->pSessionKey->pData, 
                  NTLM_SESSION_KEY_SIZE);

    // Create CreateInformation
    status = LsaMapSecurityResolveObjectInfoFromAuthUserInfo(
                hLsaConnection,
                pUserInfo,
                &objectInfo);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pUserInfo->dwNumRids != 0 || pUserInfo->dwNumSids != 0)
    {
        status = RTL_ALLOCATE(&ppInputSids, PSID, 
                              sizeof(*ppInputSids) * (pUserInfo->dwNumRids + pUserInfo->dwNumSids));
        GOTO_CLEANUP_ON_STATUS(status);
    
        status = RtlAllocateSidFromCString(&pDomainSid, pUserInfo->pszDomainSid);
        GOTO_CLEANUP_ON_STATUS(status);
    
        for (dwIndex = 0; dwIndex < pUserInfo->dwNumRids; dwIndex++)
        {
            status = LsaMapSecurityConstructSid(
                pDomainSid,
                pUserInfo->pRidAttribList[dwIndex].Rid,
                &pSid);
            GOTO_CLEANUP_ON_STATUS(status);
    
            ppInputSids[dwInputSidCount++] = pSid;
            pSid = NULL;
        }
    
        for (dwIndex = 0; dwIndex < pUserInfo->dwNumSids; dwIndex++)
        {
            status = RtlAllocateSidFromCString(&pSid, pUserInfo->pSidAttribList[dwIndex].pszSid);
            GOTO_CLEANUP_ON_STATUS(status);
    
            ppInputSids[dwInputSidCount++] = pSid;
            pSid = NULL;
        }
    }

    status = LsaMapSecurityGetAccessTokenCreateInformationFromObjectInfoAndGroups(
                Context,
                &pCreateInformation,
                &objectInfo,
                dwInputSidCount,
                ppInputSids);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeNtlmLogonResult(Context, &pNtlmResult);
        LsaMapSecurityFreeAccessTokenCreateInformation(Context, &pCreateInformation);
    }

    LwRtlCStringFree(&lsaUserParams.pszAccountName);
    LwRtlCStringFree(&lsaUserParams.pszDomain);
    LsaFreeAuthUserInfo(&pUserInfo);

    RTL_FREE(&pDomainSid);
    RTL_FREE(&pSid);

    for (dwIndex = 0; dwIndex < dwInputSidCount; dwIndex++)
    {
        RTL_FREE(&ppInputSids[dwIndex]);
    }

    RTL_FREE(&ppInputSids);
    
    LsaMapSecurityCloseConnection(Context, &hLsaConnection);
    LsaMapSecurityFreeObjectInfo(&objectInfo);    

    *ppNtlmResult = pNtlmResult;
    *ppCreateInformation = pCreateInformation;

    return status;
}

static LW_MAP_SECURITY_PLUGIN_INTERFACE gLsaMapSecurityPluginInterface = {
    .FreeContext = LsaMapSecurityFreeContext,
    .GetIdFromSid = LsaMapSecurityGetIdFromSid,
    .GetSidFromId = LsaMapSecurityGetSidFromId,
    .GetSidFromName = LsaMapSecurityGetSidFromName,
    .GetNameFromSid = LsaMapSecurityGetNameFromSid,
    .DuplicateSid = LsaMapSecurityDuplicateSid,
    .FreeSid = LsaMapSecurityFreeSid,
    .FreeCString = LsaMapSecurityFreeCString,
    .GetAccessTokenCreateInformationFromUid = LsaMapSecurityGetAccessTokenCreateInformationFromUid,
    .GetAccessTokenCreateInformationFromUsername = LsaMapSecurityGetAccessTokenCreateInformationFromUsername,
    .GetAccessTokenCreateInformationFromGssContext = LsaMapSecurityGetAccessTokenCreateInformationFromGssContext,
    .FreeAccessTokenCreateInformation = LsaMapSecurityFreeAccessTokenCreateInformation,
    .GetAccessTokenCreateInformationFromNtlmLogon = LsaMapSecurityGetAccessTokenCreateInformationFromNtlmLogon,
    .FreeNtlmLogonResult = LsaMapSecurityFreeNtlmLogonResult,
    .GetLocalGuestAccountSid = LsaMapSecurityGetLocalGuestAccountSid,
};

NTSTATUS
MapSecurityPluginCreateContext(
    OUT PLW_MAP_SECURITY_PLUGIN_CONTEXT* Context,
    OUT PLW_MAP_SECURITY_PLUGIN_INTERFACE* Interface
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_PLUGIN_CONTEXT context = NULL;
    // compiler type check for this function
    LWMSP_CREATE_CONTEXT_CALLBACK unused = MapSecurityPluginCreateContext;

    assert(unused);

    status = RTL_ALLOCATE(&context, LW_MAP_SECURITY_PLUGIN_CONTEXT, sizeof(*context));
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        LsaMapSecurityFreeContext(&context);
    }

    *Context = context;
    *Interface = NT_SUCCESS(status) ? &gLsaMapSecurityPluginInterface : NULL;

    return status;
}


static
NTSTATUS
LsaMapSecurityCreateTokenDefaultDacl(
    OUT PACL *ppDacl,
    IN PSID pOwnerSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulDaclSize = 0;
    PACL pDacl = NULL;

    ulDaclSize = ACL_HEADER_SIZE +
        sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(pOwnerSid) +
        sizeof(ULONG);

    status = LW_RTL_ALLOCATE(&pDacl, VOID, ulDaclSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAcl(pDacl, ulDaclSize, ACL_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAddAccessAllowedAceEx(
                 pDacl,
                 ACL_REVISION,
                 0,
                 GENERIC_ALL,
                 pOwnerSid);
    GOTO_CLEANUP_ON_STATUS(status);

    *ppDacl = pDacl;

cleanup:
    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pDacl);
    }
    
    return status;
}

static
NTSTATUS
LsapMapSecurityGetLocalGuestAccountSid(
    IN HANDLE hLsaConnection,
    OUT PSID* ppGuestSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSASTATUS pLsaStatus = NULL;
    PSID pGuestSid = NULL;
    PSID pDomainSid = NULL;
    ULONG guestSidSize = 
            RtlLengthRequiredSid(1 + SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT + 1);

    dwError = LsaGetStatus2(
                    hLsaConnection,
                    LSA_PROVIDER_TAG_LOCAL,
                    &pLsaStatus);
    status = LsaLsaErrorToNtStatus(dwError);
    GOTO_CLEANUP_ON_STATUS(status);

    if (!pLsaStatus->pAuthProviderStatusList->pszDomainSid)
    {
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pGuestSid, SID, guestSidSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAllocateSidFromCString(
                    &pDomainSid,
                    pLsaStatus->pAuthProviderStatusList->pszDomainSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCopySid(guestSidSize, pGuestSid, pDomainSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAppendRidSid(
                    guestSidSize,
                    pGuestSid,
                    LSA_MAP_SECURITY_MAP_TO_GUEST_RID);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pGuestSid);
    }

    RTL_FREE(&pDomainSid);
    LsaFreeStatus(pLsaStatus);
    pLsaStatus = NULL;

    *ppGuestSid = pGuestSid;

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
