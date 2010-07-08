/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        netshare.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        Net Share Internal Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

static
DWORD
MapNameToSid(
    HANDLE hLsa,
    PCWSTR pwszName,
    PSID* ppSid
    );

static
DWORD
MapSidToName(
    HANDLE hLsa,
    PSID pSid,
    PWSTR* ppwszName
    );

static
DWORD
MapBuiltinNameToSid(
    PSID *ppSid,
    PCWSTR pwszName
    );

static
DWORD
MapBuiltinSidToName(
    PWSTR *ppwszName,
    PSID pSid
    );

static
DWORD
ConstructSecurityDescriptor(
    DWORD dwAllowUserCount,
    PWSTR* ppwszAllowUsers,
    DWORD dwDenyUserCount,
    PWSTR* ppwszDenyUsers,
    BOOLEAN bReadOnly,
    PSECURITY_DESCRIPTOR_RELATIVE* ppRelative,
    PDWORD pdwRelativeSize
    );

static
DWORD
DeconstructSecurityDescriptor(
    DWORD dwLength,
    PSECURITY_DESCRIPTOR_RELATIVE pRelative,
    PDWORD pdwAllowUserCount,
    PWSTR** pppwszAllowUsers,
    PDWORD pdwDenyUserCount,
    PWSTR** pppwszDenyUsers,
    PBOOLEAN pbReadOnly
    );



DWORD
NetExecShareAdd(
    NET_SHARE_ADD_OR_SET_INFO_PARAMS ShareAddInfo
    )
{
    static const DWORD dwLevel = 502;

    DWORD dwError = 0;
    SHARE_INFO_502 shareInfo = {0};
    DWORD dwParmErr = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    DWORD dwSecDescSize = 0;

    dwError = ConstructSecurityDescriptor(
    	ShareAddInfo.dwAllowUserCount,
    	ShareAddInfo.ppwszAllowUsers,
    	ShareAddInfo.dwDenyUserCount,
    	ShareAddInfo.ppwszDenyUsers,
    	ShareAddInfo.bReadOnly && !ShareAddInfo.bReadWrite,
        &pSecDesc,
        &dwSecDescSize);
    BAIL_ON_LTNET_ERROR(dwError);

    shareInfo.shi502_netname = ShareAddInfo.pwszShareName ? ShareAddInfo.pwszShareName : ShareAddInfo.pwszTarget;
    shareInfo.shi502_path = ShareAddInfo.pwszPath;

    shareInfo.shi502_type = 0; // SHARE_SERVICE_DISK_SHARE
    shareInfo.shi502_remark = ShareAddInfo.pwszComment;
    shareInfo.shi502_reserved = dwSecDescSize;
    shareInfo.shi502_security_descriptor = (PBYTE) pSecDesc;

    dwError = NetShareAddW(
    	ShareAddInfo.pwszServerName,
        dwLevel,
        (PBYTE)&shareInfo,
        &dwParmErr);
    BAIL_ON_LTNET_ERROR(dwError);

cleanup:

    LTNET_SAFE_FREE_MEMORY(pSecDesc);

    return dwError;

error:

    goto cleanup;
}


DWORD
NetExecShareEnum(
    NET_SHARE_ENUM_INFO_PARAMS ShareEnumInfo
    )
{
    static const DWORD dwLevel = 2;
    static const DWORD dwMaxLen = 1024;

    DWORD dwError = 0;
    PSHARE_INFO_2 pShareInfo = NULL;
    DWORD dwNumShares = 0;
    DWORD dwTotalShares = 0;
    DWORD dwVisitedShares = 0;
    DWORD dwResume = 0;
    DWORD dwIndex = 0;

    PSTR* ppszShareName = NULL;
    PSTR* ppszSharePath = NULL;
    PSTR* ppszShareComment = NULL;
    DWORD dwShareNameLenMax = 0;
    DWORD dwSharePathLenMax = 0;
    DWORD dwShareCommentLenMax = 0;
    DWORD dwShareNameLen = 0;
    DWORD dwSharePathLen = 0;
    DWORD dwShareCommentLen = 0;

    do
    {
        dwError = NetShareEnumW(
            ShareEnumInfo.pwszServerName,
            dwLevel,
            (PBYTE*)&pShareInfo,
            dwMaxLen,
            &dwNumShares,
            &dwTotalShares,
            &dwResume);
        if (dwError == ERROR_MORE_DATA)
        {
            dwError = 0;
        }
        BAIL_ON_LTNET_ERROR(dwError);

        if (!ppszShareName)
        {
            dwError = LwNetAllocateMemory((dwTotalShares+1)*sizeof(PCSTR), (PVOID *)&ppszShareName);
            BAIL_ON_LTNET_ERROR(dwError);
        }

        if (!ppszSharePath)
        {
            dwError = LwNetAllocateMemory((dwTotalShares+1)*sizeof(PCSTR), (PVOID *)&ppszSharePath);
            BAIL_ON_LTNET_ERROR(dwError);
        }

        if (!ppszShareComment)
        {
            dwError = LwNetAllocateMemory((dwTotalShares+1)*sizeof(PCSTR), (PVOID *)&ppszShareComment);
            BAIL_ON_LTNET_ERROR(dwError);
        }

        for (dwIndex = 0; dwIndex < dwNumShares; dwIndex++)
        {
            dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi2_netname,
                                   &ppszShareName[dwIndex+dwVisitedShares]);
            BAIL_ON_LTNET_ERROR(dwError);

            dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi2_path,
                                   &ppszSharePath[dwIndex+dwVisitedShares]);
            BAIL_ON_LTNET_ERROR(dwError);

            if (pShareInfo[dwIndex].shi2_remark)
            {
                dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi2_remark,
                                       &ppszShareComment[dwIndex+dwVisitedShares]);
                BAIL_ON_LTNET_ERROR(dwError);
            }
        }

        if (pShareInfo)
        {
            NetApiBufferFree(pShareInfo);
            pShareInfo = NULL;
        }

        dwVisitedShares += dwNumShares;

    } while (dwVisitedShares < dwTotalShares);

    dwError = LwNetAllocateString(NET_SHARE_NAME_TITLE, &ppszShareName[dwTotalShares]);
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNetAllocateString(NET_SHARE_PATH_TITLE, &ppszSharePath[dwTotalShares]);
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNetAllocateString(NET_SHARE_COMMENT_TITLE, &ppszShareComment[dwTotalShares]);
    BAIL_ON_LTNET_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwTotalShares + 1; dwIndex++)
    {
        dwShareNameLen = strlen(ppszShareName[dwIndex]);
        if (dwShareNameLen>dwShareNameLenMax)
        {
            dwShareNameLenMax = dwShareNameLen;
        }

        dwSharePathLen = strlen(ppszSharePath[dwIndex]);
        if (dwSharePathLen>dwSharePathLenMax)
        {
            dwSharePathLenMax = dwSharePathLen;
        }

        if (ppszShareComment[dwIndex])
        {
            dwShareCommentLen = strlen(ppszShareComment[dwIndex]);
            if (dwShareCommentLen>dwShareCommentLenMax)
            {
            	dwShareCommentLenMax = dwShareCommentLen;
            }
        }
    }

    //print share enum header

    printf("  %s%*s",
           NET_SHARE_NAME_TITLE,
           (int) (strlen(NET_SHARE_NAME_TITLE)-dwShareNameLenMax),
           "");
    printf("  %s%*s",
           NET_SHARE_PATH_TITLE,
           (int) (strlen(NET_SHARE_PATH_TITLE)-dwSharePathLenMax),
           "");
    printf("  %s%*s\n",
           NET_SHARE_COMMENT_TITLE,
           (int) (strlen(NET_SHARE_COMMENT_TITLE)-dwShareCommentLenMax),
           "");

    for (dwIndex = 0; dwIndex < dwShareNameLenMax+dwSharePathLenMax+dwShareCommentLenMax+10; dwIndex++)
    	printf("%s", "-");

    printf("\n");


    for (dwIndex = 0; dwIndex < dwTotalShares; dwIndex++)
    {
        printf("  %s%*s",
               ppszShareName[dwIndex],
               (int) (strlen(ppszShareName[dwIndex])-dwShareNameLenMax),
               "");

        printf("  %s%*s",
               ppszSharePath[dwIndex],
               (int) (strlen(ppszSharePath[dwIndex])-dwSharePathLenMax),
               "");

        if (ppszShareComment[dwIndex])
        {
            printf("  %s%*s",
                   ppszShareComment[dwIndex],
                   (int) (strlen(ppszShareComment[dwIndex])-dwShareCommentLenMax),
                   "");
        }

        printf("\n");
    }

cleanup:

    if (ppszShareName)
    {
    	LwNetFreeStringArray(dwTotalShares, ppszShareName);
    }
    if (ppszSharePath)
    {
    	LwNetFreeStringArray(dwTotalShares, ppszSharePath);
    }
    if (ppszShareComment)
    {
    	LwNetFreeStringArray(dwTotalShares, ppszShareComment);
    }

    if (pShareInfo)
    {
    	NetApiBufferFree(pShareInfo);
        pShareInfo = NULL;
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
NetExecShareDel(
    NET_SHARE_DEL_INFO_PARAMS ShareDelInfo
    )
{
    DWORD dwError = 0;

    dwError = NetShareDelW(
    	ShareDelInfo.pwszServerName,
        ShareDelInfo.pwszShareName,
        0);
    BAIL_ON_LTNET_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
NetExecSetInfo(
    NET_SHARE_ADD_OR_SET_INFO_PARAMS ShareSetInfo
    )
{
    static const DWORD dwLevel = 502;

    DWORD dwError = 0;
    SHARE_INFO_502 newShareInfo = {0};
    PSHARE_INFO_502 pShareInfo = NULL;
    DWORD dwParmErr = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    DWORD dwSecDescSize = 0;
    DWORD dwAllowUserCount = 0;
    PWSTR* ppwszAllowUsers = NULL;
    DWORD dwDenyUserCount = 0;
    PWSTR* ppwszDenyUsers = NULL;
    BOOLEAN bReadOnly = FALSE;

    dwError = NetShareGetInfoW(
        ShareSetInfo.pwszServerName,
        ShareSetInfo.pwszShareName,
        dwLevel,
        (PBYTE*)(&pShareInfo));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = DeconstructSecurityDescriptor(
        pShareInfo->shi502_reserved,
        (PSECURITY_DESCRIPTOR_RELATIVE) pShareInfo->shi502_security_descriptor,
        &dwAllowUserCount,
        &ppwszAllowUsers,
        &dwDenyUserCount,
        &ppwszDenyUsers,
        &bReadOnly);
    BAIL_ON_LTNET_ERROR(dwError);

    newShareInfo = *pShareInfo;

    if (ShareSetInfo.pwszShareName)
    {
        newShareInfo.shi502_netname = ShareSetInfo.pwszShareName;
    }

    if (ShareSetInfo.pwszComment)
    {
        newShareInfo.shi502_remark = ShareSetInfo.pwszComment;
    }

    if (ShareSetInfo.pwszPath)
    {
        newShareInfo.shi502_path = ShareSetInfo.pwszPath;
    }

    dwError = ConstructSecurityDescriptor(
        ShareSetInfo.dwAllowUserCount
        || ShareSetInfo.bClearAllow ? ShareSetInfo.dwAllowUserCount : dwAllowUserCount,
        ShareSetInfo.dwAllowUserCount || ShareSetInfo.bClearAllow ? ShareSetInfo.ppwszAllowUsers : ppwszAllowUsers,
        ShareSetInfo.dwDenyUserCount || ShareSetInfo.bClearDeny ? ShareSetInfo.dwDenyUserCount : dwDenyUserCount,
        ShareSetInfo.dwDenyUserCount || ShareSetInfo.bClearDeny ? ShareSetInfo.ppwszDenyUsers : ppwszDenyUsers,
        ShareSetInfo.bReadOnly || ShareSetInfo.bReadWrite ? (ShareSetInfo.bReadOnly && !ShareSetInfo.bReadWrite) : bReadOnly,
        &pSecDesc,
        &dwSecDescSize);
    BAIL_ON_LTNET_ERROR(dwError);

    newShareInfo.shi502_type = pShareInfo->shi502_type;
    newShareInfo.shi502_reserved = dwSecDescSize;
    newShareInfo.shi502_security_descriptor = (PBYTE) pSecDesc;

    dwError = NetShareSetInfoW(
        ShareSetInfo.pwszServerName,
        ShareSetInfo.pwszShareName,
        dwLevel,
        (PBYTE)&newShareInfo,
        &dwParmErr);
    BAIL_ON_LTNET_ERROR(dwError);

cleanup:

    if (pShareInfo)
    {
        LwNetFreeMemory(pShareInfo);
    }

    LwNetFreeWC16StringArray(dwAllowUserCount, ppwszAllowUsers);
    LwNetFreeWC16StringArray(dwDenyUserCount, ppwszDenyUsers);

    LTNET_SAFE_FREE_MEMORY(pSecDesc);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
MapNameToSid(
    HANDLE hLsa,
    PCWSTR pwszName,
    PSID* ppSid
    )
{
    DWORD dwError = 0;
    PSTR pszName = NULL;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LwWc16sToMbs(pwszName, &pszName);
    BAIL_ON_LTNET_ERROR(dwError);

    QueryList.ppszStrings = (PCSTR*) &pszName;

    dwError = LsaFindObjects(
        hLsa,
        NULL,
        0,
        LSA_OBJECT_TYPE_UNDEFINED,
        LSA_QUERY_TYPE_BY_NAME,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LTNET_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LTNET_ERROR(dwError);
    }

    dwError = LwNetAllocateSidFromCString(ppSid, ppObjects[0]->pszObjectSid);
    BAIL_ON_LTNET_ERROR(dwError);

cleanup:

    LsaFreeSecurityObjectList(1, ppObjects);

    LTNET_SAFE_FREE_STRING(pszName);

    return dwError;

error:

    *ppSid = NULL;

    goto cleanup;
}

static
DWORD
MapSidToName(
    HANDLE hLsa,
    PSID pSid,
    PWSTR* ppwszName
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LwNtStatusToWin32Error(
        RtlAllocateCStringFromSid(&pszSid, pSid));
    BAIL_ON_LTNET_ERROR(dwError);

    QueryList.ppszStrings = (PCSTR*) &pszSid;

    dwError = LsaFindObjects(
        hLsa,
        NULL,
        0,
        LSA_OBJECT_TYPE_UNDEFINED,
        LSA_QUERY_TYPE_BY_SID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LTNET_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LTNET_ERROR(dwError);
    }

    dwError = LwAllocateWc16sPrintfW(
        ppwszName,
        L"%s\\%s",
        ppObjects[0]->pszNetbiosDomainName,
        ppObjects[0]->pszSamAccountName);
    BAIL_ON_LTNET_ERROR(dwError);

cleanup:

    LsaFreeSecurityObjectList(1, ppObjects);

    LTNET_SAFE_FREE_STRING(pszSid);

    return dwError;

error:

    *ppwszName = NULL;

    goto cleanup;
}


static
DWORD
MapBuiltinNameToSid(
    PSID *ppSid,
    PCWSTR pwszName
    )
{
    DWORD dwError = 0;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } Sid;
    ULONG SidSize = sizeof(Sid.buffer);
    PWSTR pwszEveryone = NULL;

    dwError = LwNetWC16StringAllocateFromCString(
        &pwszEveryone,
        "Everyone");
    BAIL_ON_LTNET_ERROR(dwError);


    if (LwRtlWC16StringIsEqual(pwszName, pwszEveryone, FALSE))
    {
        dwError = LwNtStatusToWin32Error(
            RtlCreateWellKnownSid(
                WinWorldSid,
                NULL,
                &Sid.sid,
                &SidSize));
    }
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlDuplicateSid(ppSid, &Sid.sid));

cleanup:
    LwNetWC16StringFree(pwszEveryone);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
MapBuiltinSidToName(
    PWSTR *ppwszName,
    PSID pSid
    )
{
    DWORD dwError = 0;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } Sid;
    ULONG SidSize = sizeof(Sid.buffer);
    PWSTR pwszEveryone = NULL;

    dwError = LwNtStatusToWin32Error(
        RtlCreateWellKnownSid(
            WinWorldSid,
            NULL,
            &Sid.sid,
            &SidSize));
    BAIL_ON_LTNET_ERROR(dwError);

    if (RtlEqualSid(&Sid.sid, pSid))
    {
        dwError = LwNtStatusToWin32Error(
            RtlWC16StringAllocateFromCString(
                &pwszEveryone,
                "Everyone"));
        BAIL_ON_LTNET_ERROR(dwError);

    }

    *ppwszName = pwszEveryone;

cleanup:

    return dwError;

error:
    LwNetWC16StringFree(pwszEveryone);

    goto cleanup;
}

static
DWORD
ConstructSecurityDescriptor(
    DWORD dwAllowUserCount,
    PWSTR* ppwszAllowUsers,
    DWORD dwDenyUserCount,
    PWSTR* ppwszDenyUsers,
    BOOLEAN bReadOnly,
    PSECURITY_DESCRIPTOR_RELATIVE* ppRelative,
    PDWORD pdwRelativeSize
    )
{
    DWORD dwError = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pRelative = NULL;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } Owner;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } Group;
    ULONG OwnerSidSize = sizeof(Owner.buffer);
    ULONG GroupSidSize = sizeof(Group.buffer);
    DWORD dwDaclSize = 0;
    PACL pDacl = NULL;
    DWORD dwIndex = 0;
    PSID pSid = NULL;
    ULONG ulRelativeSize = 0;
    HANDLE hLsa = NULL;
    ACCESS_MASK mask = bReadOnly ?
        (FILE_GENERIC_READ|FILE_GENERIC_EXECUTE) :
        FILE_ALL_ACCESS;

    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateWellKnownSid(
            WinBuiltinAdministratorsSid,
            NULL,
            &Owner.sid,
            &OwnerSidSize));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateWellKnownSid(
            WinBuiltinPowerUsersSid,
            NULL,
            &Group.sid,
            &GroupSidSize));
    BAIL_ON_LTNET_ERROR(dwError);

    dwDaclSize = ACL_HEADER_SIZE +
        dwAllowUserCount * (sizeof(ACCESS_ALLOWED_ACE) + SID_MAX_SIZE) +
        dwDenyUserCount * (sizeof(ACCESS_DENIED_ACE) + SID_MAX_SIZE) +
        RtlLengthSid(&Owner.sid) + RtlLengthSid(&Group.sid);

    dwError = LwNetAllocateMemory(
        dwDaclSize,
        OUT_PPVOID(&pDacl));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION));
    BAIL_ON_LTNET_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwDenyUserCount; dwIndex++)
    {
        dwError = MapNameToSid(hLsa, ppwszDenyUsers[dwIndex], &pSid);
        if (dwError != LW_ERROR_SUCCESS)
        {
            dwError = MapBuiltinNameToSid(&pSid, ppwszDenyUsers[dwIndex]);
        }

        BAIL_ON_LTNET_ERROR(dwError);

        dwError = LwNtStatusToWin32Error(
            RtlAddAccessDeniedAceEx(
                pDacl,
                ACL_REVISION,
                0,
                FILE_ALL_ACCESS,
                pSid));
        BAIL_ON_LTNET_ERROR(dwError);

        RTL_FREE(&pSid);
    }

    for (dwIndex = 0; dwIndex < dwAllowUserCount; dwIndex++)
    {
        dwError = MapNameToSid(hLsa, ppwszAllowUsers[dwIndex], &pSid);
        if (dwError != LW_ERROR_SUCCESS)
        {
            dwError = MapBuiltinNameToSid(&pSid, ppwszAllowUsers[dwIndex]);
        }
        BAIL_ON_LTNET_ERROR(dwError);

        dwError = LwNtStatusToWin32Error(
            RtlAddAccessAllowedAceEx(
                pDacl,
                ACL_REVISION,
                0,
                mask,
                pSid));
        BAIL_ON_LTNET_ERROR(dwError);

        RTL_FREE(&pSid);
    }

    dwError = LwNetAllocateMemory(
        SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
        OUT_PPVOID(&pAbsolute));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateSecurityDescriptorAbsolute(
            pAbsolute,
            SECURITY_DESCRIPTOR_REVISION));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetOwnerSecurityDescriptor(
            pAbsolute,
            &Owner.sid,
            FALSE));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetGroupSecurityDescriptor(
            pAbsolute,
            &Group.sid,
            FALSE));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetDaclSecurityDescriptor(
            pAbsolute,
            TRUE,
            pDacl,
            FALSE));
    BAIL_ON_LTNET_ERROR(dwError);

    RtlAbsoluteToSelfRelativeSD(
        pAbsolute,
        NULL,
        &ulRelativeSize);

    dwError = LwNetAllocateMemory(ulRelativeSize, OUT_PPVOID(&pRelative));
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlAbsoluteToSelfRelativeSD(
            pAbsolute,
            pRelative,
            &ulRelativeSize));
    BAIL_ON_LTNET_ERROR(dwError);

    *ppRelative = pRelative;
    *pdwRelativeSize = ulRelativeSize;

cleanup:

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    LTNET_SAFE_FREE_MEMORY(pSid);
    LTNET_SAFE_FREE_MEMORY(pDacl);
    LTNET_SAFE_FREE_MEMORY(pAbsolute);

    return dwError;

error:

    *ppRelative = NULL;
    *pdwRelativeSize = 0;

    LTNET_SAFE_FREE_MEMORY(pRelative);

    goto cleanup;
}

static
DWORD
DeconstructSecurityDescriptor(
    DWORD dwLength,
    PSECURITY_DESCRIPTOR_RELATIVE pRelative,
    PDWORD pdwAllowUserCount,
    PWSTR** pppwszAllowUsers,
    PDWORD pdwDenyUserCount,
    PWSTR** pppwszDenyUsers,
    PBOOLEAN pbReadOnly
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    ULONG ulSize = 0;
    ULONG ulDaclSize = 0;
    ULONG ulSaclSize = 0;
    ULONG ulOwnerSize = 0;
    ULONG ulGroupSize = 0;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pSacl = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    PACL pDacl = NULL;
    ULONG ulIndex = 0;
    PVOID pAce = NULL;
    PACCESS_ALLOWED_ACE pAllow = NULL;
    PACCESS_DENIED_ACE pDeny = NULL;
    DWORD dwAllowUserCount = 0;
    PWSTR* ppwszAllowUsers = NULL;
    DWORD dwDenyUserCount = 0;
    PWSTR* ppwszDenyUsers = NULL;
    PSID pSid = NULL;
    PWSTR pwszUser = NULL;
    HANDLE hLsa = NULL;
    ACCESS_MASK leastMask = FILE_ALL_ACCESS;

    dwError = LsaOpenServer(&hLsa);
    BAIL_ON_LTNET_ERROR(dwError);

    status = RtlSelfRelativeToAbsoluteSD(
        pRelative,
        pAbsolute,
        &ulSize,
        pDacl,
        &ulDaclSize,
        pSacl,
        &ulSaclSize,
        pOwner,
        &ulOwnerSize,
        pGroup,
        &ulGroupSize);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        dwError = LwNtStatusToWin32Error(status);
        BAIL_ON_LTNET_ERROR(dwError);
    }

    dwError = LwNetAllocateMemory(ulSize, OUT_PPVOID(&pAbsolute));
    BAIL_ON_LTNET_ERROR(dwError);

    if (ulDaclSize)
    {
        dwError = LwNetAllocateMemory(ulDaclSize, OUT_PPVOID(&pDacl));
        BAIL_ON_LTNET_ERROR(dwError);
    }

    if (ulSaclSize)
    {
        dwError = LwNetAllocateMemory(ulSaclSize, OUT_PPVOID(&pSacl));
        BAIL_ON_LTNET_ERROR(dwError);
    }

    if (ulOwnerSize)
    {
        dwError = LwNetAllocateMemory(ulOwnerSize, OUT_PPVOID(&pOwner));
        BAIL_ON_LTNET_ERROR(dwError);
    }

    if (ulGroupSize)
    {
        dwError = LwNetAllocateMemory(ulGroupSize, OUT_PPVOID(&pGroup));
        BAIL_ON_LTNET_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(
        RtlSelfRelativeToAbsoluteSD(
            pRelative,
            pAbsolute,
            &ulSize,
            pDacl,
            &ulDaclSize,
            pSacl,
            &ulSaclSize,
            pOwner,
            &ulOwnerSize,
            pGroup,
            &ulGroupSize));
    BAIL_ON_LTNET_ERROR(dwError);

    if (pDacl)
    {
        for (ulIndex = 0; ulIndex < RtlGetAclAceCount(pDacl); ulIndex++)
        {
            RtlGetAce(pDacl, ulIndex, &pAce);

            switch(((PACE_HEADER) pAce)->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                pAllow = pAce;
                pSid = (PSID) &pAllow->SidStart;

                if ((pAllow->Mask & FILE_GENERIC_READ) == FILE_GENERIC_READ)
                {
                    dwError = MapSidToName(hLsa, pSid, &pwszUser);
                    if (dwError != LW_ERROR_SUCCESS)
                    {
                        dwError = MapBuiltinSidToName(&pwszUser, pSid);
                    }
                    BAIL_ON_LTNET_ERROR(dwError);

                    dwError = LwNetAppendStringArray(
                        &dwAllowUserCount,
                        &ppwszAllowUsers,
                        pwszUser);
                    BAIL_ON_LTNET_ERROR(dwError);

                    pwszUser = NULL;

                    leastMask &= pAllow->Mask;
                }
                break;
            case ACCESS_DENIED_ACE_TYPE:
                pDeny = pAce;
                pSid = (PSID) &pDeny->SidStart;

                if ((pDeny->Mask & FILE_GENERIC_READ) == FILE_GENERIC_READ)
                {
                    dwError = MapSidToName(hLsa, pSid, &pwszUser);
                    if (dwError != LW_ERROR_SUCCESS)
                    {
                        dwError = MapBuiltinSidToName(&pwszUser, pSid);
                    }
                    BAIL_ON_LTNET_ERROR(dwError);

                    dwError = LwNetAppendStringArray(
                        &dwDenyUserCount,
                        &ppwszDenyUsers,
                        pwszUser);
                    BAIL_ON_LTNET_ERROR(dwError);

                    pwszUser = NULL;
                }
                break;
            default:
                break;
            }
        }
    }

    *pppwszAllowUsers = ppwszAllowUsers;
    *pdwAllowUserCount = dwAllowUserCount;
    *pppwszDenyUsers = ppwszDenyUsers;
    *pdwDenyUserCount = dwDenyUserCount;
    *pbReadOnly = !((leastMask & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE);

cleanup:

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    LTNET_SAFE_FREE_MEMORY(pSacl);
    LTNET_SAFE_FREE_MEMORY(pOwner);
    LTNET_SAFE_FREE_MEMORY(pGroup);
    LTNET_SAFE_FREE_MEMORY(pwszUser);
    LTNET_SAFE_FREE_MEMORY(pDacl);
    LTNET_SAFE_FREE_MEMORY(pAbsolute);

    return dwError;

error:

    *pppwszAllowUsers = NULL;
    *pdwAllowUserCount = 0;
    *pppwszDenyUsers = NULL;
    *pdwDenyUserCount = 0;

    goto cleanup;
}
