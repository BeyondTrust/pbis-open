/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
  TODO:
  
  - --read-only --read-write
  - --clear-allow --clear-deny
*/
#include "config.h"
#include "nfssvcsys.h"

#include <lw/base.h>
#include <lw/security-api.h>
#include <lw/nfssvc.h>
#include <lsa/lsa.h>
#include <lwerror.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwio/io-types.h>
#include <stdio.h>
#include <sys/param.h>
#include <nfssvcdefs.h>
#include <nfssvcutils.h>

static struct
{
    PWSTR pwszServerName;
    DWORD dwAllowUserCount;
    PWSTR* ppwszAllowUsers;
    DWORD dwDenyUserCount;
    PWSTR* ppwszDenyUsers;
    PWSTR pwszName;
    PWSTR pwszPath;
    PWSTR pwszComment;
    PWSTR pwszTarget;
    BOOLEAN bReadOnly;
    BOOLEAN bReadWrite;
    BOOLEAN bClearAllow;
    BOOLEAN bClearDeny;
} gState = {0};

static
VOID
FreeStringArray(
    DWORD dwCount,
    PWSTR* ppwszArray
    )
{
    DWORD dwIndex = 0;

    if (ppwszArray)
    {
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            LW_SAFE_FREE_MEMORY(ppwszArray[dwIndex]);
        }

        LW_SAFE_FREE_MEMORY(ppwszArray);
    }
}

static
DWORD
AppendStringArray(
    PDWORD pdwCount,
    PWSTR** pppwszArray,
    PWSTR pwszString
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszNewArray = NULL;

    dwError = LwReallocMemory(
        *pppwszArray,
        OUT_PPVOID(&ppwszNewArray),
        sizeof(*ppwszNewArray) * (*pdwCount + 1));
    BAIL_ON_NFSSVC_ERROR(dwError);

    ppwszNewArray[(*pdwCount)++] = pwszString;
    
    *pppwszArray = ppwszNewArray;

error:

    return dwError;
}

static
DWORD
PrintStringAttribute(
    PCSTR pszName,
    PCWSTR pwszValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;

    dwError = LwWc16sToMbs(pwszValue, &pszValue);
    BAIL_ON_NFSSVC_ERROR(dwError);

    printf("%s=%s\n", pszName, pszValue);

error:

    LW_SAFE_FREE_STRING(pszValue);

    return dwError;
}

static
VOID
Usage(
    void
    )
{
    printf(
        "Usage: lw-share [ <options> ... ] enum\n"
        "       lw-share [ <options> ... ] enum-info\n"
        "       lw-share [ <options> ... ] add [ <add options> ... ] <share>\n"
        "       lw-share [ <options> ... ] del <share>\n"
        "       lw-share [ <options> ... ] get-info <share>\n"
        "       lw-share [ <options> ... ] set-info [ <set-info options> ... ] <share>\n");
}

static
VOID
Help(
    void
    )
{
    Usage();

    printf("\n"
           "Options:\n"
           "\n"
           "  --usage                 Show usage information\n"
           "  --help                  Show this help information\n"
           "  --server <server>       Specify target server (default: local machine)\n"
           "\n"
           "Options (add/set-info):\n"
           "\n"
           "  --path <path>           Share path\n"
           "  --comment <comment>     Share comment\n"
           "  --read-only             Make share read-only\n"
           "  --read-write            Make share readable and writable (default)\n"
           "  --allow <nt4 name>      Allow user/group access to share\n"
           "  --deny <nt4 name>       Deny user/group access to share\n"
           "\n"
           "Options (set-ifno):\n"
           "  --clear-allow           Clear allowed list\n"
           "  --clear-deny            Clear denied list\n");
}

static
DWORD
ParseShareArgs(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PWSTR pwszArg = NULL;

    for (dwIndex = 1; dwIndex < argc; dwIndex++)
    {
        if (!strcmp(ppszArgv[dwIndex], "--allow"))
        {
            dwError = LwMbsToWc16s(ppszArgv[++dwIndex], &pwszArg);
            BAIL_ON_NFSSVC_ERROR(dwError);
            
            dwError = AppendStringArray(
                &gState.dwAllowUserCount,
                &gState.ppwszAllowUsers,
                pwszArg);
            BAIL_ON_NFSSVC_ERROR(dwError);

            pwszArg = NULL;
        }
        else if (!strcmp(ppszArgv[dwIndex], "--deny"))
        {
            dwError = LwMbsToWc16s(ppszArgv[++dwIndex], &pwszArg);
            BAIL_ON_NFSSVC_ERROR(dwError);
            
            dwError = AppendStringArray(
                &gState.dwDenyUserCount,
                &gState.ppwszDenyUsers,
                pwszArg);
            BAIL_ON_NFSSVC_ERROR(dwError);

            pwszArg = NULL;
        }
        else if (!strcmp(ppszArgv[dwIndex], "--name"))
        {
            dwError = LwMbsToWc16s(ppszArgv[++dwIndex], &gState.pwszName);
            BAIL_ON_NFSSVC_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[dwIndex], "--path"))
        {
            dwError = LwMbsToWc16s(ppszArgv[++dwIndex], &gState.pwszPath);
            BAIL_ON_NFSSVC_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[dwIndex], "--comment"))
        {
            dwError = LwMbsToWc16s(ppszArgv[++dwIndex], &gState.pwszComment);
            BAIL_ON_NFSSVC_ERROR(dwError);
        }
        else if (!strcmp(ppszArgv[dwIndex], "--read-only"))
        {
            gState.bReadOnly = TRUE;
        }
        else if (!strcmp(ppszArgv[dwIndex], "--read-write"))
        {
            gState.bReadWrite = TRUE;
        }
        else if (!strcmp(ppszArgv[dwIndex], "--clear-allow"))
        {
            gState.bClearAllow = TRUE;
        }
        else if (!strcmp(ppszArgv[dwIndex], "--clear-deny"))
        {
            gState.bClearDeny = TRUE;
        }
        else
        {
            dwError = LwMbsToWc16s(ppszArgv[dwIndex], &gState.pwszTarget);
            BAIL_ON_NFSSVC_ERROR(dwError);
            break;
        }
    }

error:

    LW_SAFE_FREE_MEMORY(pwszArg);

    return dwError;
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
    BAIL_ON_NFSSVC_ERROR(dwError);
    
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
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }
    
    dwError = LwNtStatusToWin32Error(
        RtlAllocateSidFromCString(ppSid, ppObjects[0]->pszObjectSid));
    BAIL_ON_NFSSVC_ERROR(dwError);
    
cleanup:

    LsaFreeSecurityObjectList(1, ppObjects);

    LW_SAFE_FREE_STRING(pszName);

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
    BAIL_ON_NFSSVC_ERROR(dwError);
    
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
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_NFSSVC_ERROR(dwError);
    }
    
    dwError = LwNtStatusToWin32Error(
        LwRtlWC16StringAllocatePrintfW(
            ppwszName,
            L"%s\\%s",
            ppObjects[0]->pszNetbiosDomainName,
            ppObjects[0]->pszSamAccountName));
    BAIL_ON_NFSSVC_ERROR(dwError);
    
cleanup:

    LsaFreeSecurityObjectList(1, ppObjects);

    LW_SAFE_FREE_STRING(pszSid);

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

    dwError = LwNtStatusToWin32Error(
                  RtlWC16StringAllocateFromCString(
                      &pwszEveryone, 
                      "Everyone"));
    BAIL_ON_NFSSVC_ERROR(dwError);
    

    if (LwRtlWC16StringIsEqual(pwszName, pwszEveryone, FALSE))
    {
        dwError = LwNtStatusToWin32Error(
                      RtlCreateWellKnownSid(
                          WinWorldSid,
                          NULL,
                          &Sid.sid,
                          &SidSize));
    }
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                  RtlDuplicateSid(ppSid, &Sid.sid));

cleanup:
    LW_RTL_FREE(&pwszEveryone);
    
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
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (RtlEqualSid(&Sid.sid, pSid))
    {
        dwError = LwNtStatusToWin32Error(
                      RtlWC16StringAllocateFromCString(
                          &pwszEveryone, 
                          "Everyone"));
        BAIL_ON_NFSSVC_ERROR(dwError);

    }

    *ppwszName = pwszEveryone;

cleanup:
    
    return dwError;

error:
    LW_RTL_FREE(&pwszEveryone);

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
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateWellKnownSid(
            WinBuiltinAdministratorsSid,
            NULL,
            &Owner.sid,
            &OwnerSidSize));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateWellKnownSid(
            WinBuiltinPowerUsersSid,
            NULL,
            &Group.sid,
            &GroupSidSize));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwDaclSize = ACL_HEADER_SIZE +
        dwAllowUserCount * (sizeof(ACCESS_ALLOWED_ACE) + SID_MAX_SIZE) +
        dwDenyUserCount * (sizeof(ACCESS_DENIED_ACE) + SID_MAX_SIZE) +
        RtlLengthSid(&Owner.sid) + RtlLengthSid(&Group.sid);

    dwError = LwAllocateMemory(
        dwDaclSize,
        OUT_PPVOID(&pDacl));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION));
    BAIL_ON_NFSSVC_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwDenyUserCount; dwIndex++)
    {
        dwError = MapNameToSid(hLsa, ppwszDenyUsers[dwIndex], &pSid);
        if (dwError != LW_ERROR_SUCCESS)
        {
            dwError = MapBuiltinNameToSid(&pSid, ppwszDenyUsers[dwIndex]);
        }
        
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = LwNtStatusToWin32Error(
            RtlAddAccessDeniedAceEx(
                pDacl,
                ACL_REVISION,
                0,
                FILE_ALL_ACCESS,
                pSid));
        BAIL_ON_NFSSVC_ERROR(dwError);

        RTL_FREE(&pSid);
    }

   for (dwIndex = 0; dwIndex < dwAllowUserCount; dwIndex++)
    {
        dwError = MapNameToSid(hLsa, ppwszAllowUsers[dwIndex], &pSid);
        if (dwError != LW_ERROR_SUCCESS)
        {
            dwError = MapBuiltinNameToSid(&pSid, ppwszAllowUsers[dwIndex]);
        }
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwError = LwNtStatusToWin32Error(
            RtlAddAccessAllowedAceEx(
                pDacl,
                ACL_REVISION,
                0,
                mask,
                pSid));
        BAIL_ON_NFSSVC_ERROR(dwError);

        RTL_FREE(&pSid);
    }
    
    dwError = LwAllocateMemory(
        SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
        OUT_PPVOID(&pAbsolute));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateSecurityDescriptorAbsolute(
            pAbsolute,
            SECURITY_DESCRIPTOR_REVISION));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetOwnerSecurityDescriptor(
            pAbsolute,
            &Owner.sid,
            FALSE));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetGroupSecurityDescriptor(
            pAbsolute,
            &Group.sid,
            FALSE));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetDaclSecurityDescriptor(
            pAbsolute,
            TRUE,
            pDacl,
            FALSE));
    BAIL_ON_NFSSVC_ERROR(dwError);

    RtlAbsoluteToSelfRelativeSD(
        pAbsolute,
        NULL,
        &ulRelativeSize);

    dwError = LwAllocateMemory(ulRelativeSize, OUT_PPVOID(&pRelative));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlAbsoluteToSelfRelativeSD(
            pAbsolute,
            pRelative,
            &ulRelativeSize));
    BAIL_ON_NFSSVC_ERROR(dwError);
    
    *ppRelative = pRelative;
    *pdwRelativeSize = ulRelativeSize;

cleanup:

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    RTL_FREE(&pSid);
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pAbsolute);

    return dwError;

error:

    *ppRelative = NULL;
    *pdwRelativeSize = 0;

    LW_SAFE_FREE_MEMORY(pRelative);
    
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
    BAIL_ON_NFSSVC_ERROR(dwError);

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
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    dwError = LwAllocateMemory(ulSize, OUT_PPVOID(&pAbsolute));
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (ulDaclSize)
    {
        dwError = LwAllocateMemory(ulDaclSize, OUT_PPVOID(&pDacl));
        BAIL_ON_NFSSVC_ERROR(dwError);
    }
     
    if (ulSaclSize)
    {
        dwError = LwAllocateMemory(ulSaclSize, OUT_PPVOID(&pSacl));
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    if (ulOwnerSize)
    {
        dwError = LwAllocateMemory(ulOwnerSize, OUT_PPVOID(&pOwner));
        BAIL_ON_NFSSVC_ERROR(dwError);
    }

    if (ulGroupSize)
    {
        dwError = LwAllocateMemory(ulGroupSize, OUT_PPVOID(&pGroup));
        BAIL_ON_NFSSVC_ERROR(dwError);
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
    BAIL_ON_NFSSVC_ERROR(dwError);

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
                    BAIL_ON_NFSSVC_ERROR(dwError);
                    
                    dwError = AppendStringArray(
                        &dwAllowUserCount,
                        &ppwszAllowUsers,
                        pwszUser);
                    BAIL_ON_NFSSVC_ERROR(dwError);
                    
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
                    BAIL_ON_NFSSVC_ERROR(dwError);
                    
                    dwError = AppendStringArray(
                        &dwDenyUserCount,
                        &ppwszDenyUsers,
                        pwszUser);
                    BAIL_ON_NFSSVC_ERROR(dwError);
                    
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

    LW_SAFE_FREE_MEMORY(pSacl);
    LW_SAFE_FREE_MEMORY(pOwner);
    LW_SAFE_FREE_MEMORY(pGroup);
    LW_SAFE_FREE_MEMORY(pwszUser);
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pAbsolute);

    return dwError;

error:

    *pppwszAllowUsers = NULL;
    *pdwAllowUserCount = 0;
    *pppwszDenyUsers = NULL;
    *pdwDenyUserCount = 0;

    goto cleanup;
}

static
DWORD
DumpShareInfo(
    PSHARE_INFO_502 pShareInfo
    )
{
    DWORD dwError = 0;
    DWORD dwAllowUserCount = 0;
    PWSTR* ppwszAllowUsers = NULL;
    DWORD dwDenyUserCount = 0;
    PWSTR* ppwszDenyUsers = NULL;
    BOOLEAN bReadOnly = FALSE;
    PSTR pszUserName = NULL;
    DWORD dwIndex = 0;

    dwError = PrintStringAttribute("name", pShareInfo->shi502_netname);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = PrintStringAttribute("path", pShareInfo->shi502_path);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = PrintStringAttribute("comment", pShareInfo->shi502_remark);
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (pShareInfo->shi502_security_descriptor)
    {
        dwError = DeconstructSecurityDescriptor(
            pShareInfo->shi502_reserved,
            (PSECURITY_DESCRIPTOR_RELATIVE) pShareInfo->shi502_security_descriptor,
            &dwAllowUserCount,
            &ppwszAllowUsers,
            &dwDenyUserCount,
            &ppwszDenyUsers,
            &bReadOnly);
        BAIL_ON_NFSSVC_ERROR(dwError);

        if (dwAllowUserCount)
        {
            printf("allow=");
            
            for (dwIndex = 0; dwIndex < dwAllowUserCount; dwIndex++)
            {
                dwError = LwWc16sToMbs(ppwszAllowUsers[dwIndex], &pszUserName);
                BAIL_ON_NFSSVC_ERROR(dwError);
                
                printf("%s", pszUserName);
                if (dwIndex < dwAllowUserCount - 1)
                {
                    printf(",");
                }
            }
            
            printf("\n");
        }

        if (dwDenyUserCount)
        {
            printf("deny=");
            
            for (dwIndex = 0; dwIndex < dwDenyUserCount; dwIndex++)
            {
                dwError = LwWc16sToMbs(ppwszDenyUsers[dwIndex], &pszUserName);
                BAIL_ON_NFSSVC_ERROR(dwError);
                
                printf("%s", pszUserName);
                if (dwIndex < dwDenyUserCount - 1)
                {
                    printf(",");
                }
            }
            
            printf("\n");
        }

        printf("read_only=%s\n", bReadOnly ? "true" : "false");
    }

cleanup:

    FreeStringArray(dwAllowUserCount, ppwszAllowUsers);
    FreeStringArray(dwDenyUserCount, ppwszDenyUsers);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
Enum(
    int argc,
    char** ppszArgv
    )
{
    static const DWORD dwLevel = 0;
    static const DWORD dwMaxLen = 128;

    DWORD dwError = 0;
    PSHARE_INFO_0 pShareInfo = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwSeenEntries = 0;
    DWORD dwResume = 0;
    DWORD dwIndex = 0;
    PSTR pszShareName = NULL;

    do
    {
        dwError = NetShareEnum(
            gState.pwszServerName,
            dwLevel,
            OUT_PPVOID(&pShareInfo),
            dwMaxLen,
            &dwNumEntries,
            &dwTotalEntries,
            &dwResume);
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwSeenEntries += dwNumEntries;

        for (dwIndex = 0; dwIndex < dwNumEntries; dwIndex++)
        {
            dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi0_netname, &pszShareName);
            BAIL_ON_NFSSVC_ERROR(dwError);

            printf("%s\n", pszShareName);

            LW_SAFE_FREE_STRING(pszShareName);
        }

        if (pShareInfo)
        {
            NfsSvcFreeMemory(pShareInfo);
            pShareInfo = NULL;
        }
    } while (dwSeenEntries < dwTotalEntries);

cleanup:

    LW_SAFE_FREE_STRING(pszShareName);

    if (pShareInfo)
    {
        NfsSvcFreeMemory(pShareInfo);
        pShareInfo = NULL;
    }
    
    return dwError;

error:

    goto cleanup;
}

static
DWORD
EnumInfo(
    int argc,
    char** ppszArgv
    )
{
    static const DWORD dwLevel = 502;
    static const DWORD dwMaxLen = 128;

    DWORD dwError = 0;
    PSHARE_INFO_502 pShareInfo = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwSeenEntries = 0;
    DWORD dwResume = 0;
    DWORD dwIndex = 0;
    PSTR pszShareName = NULL;

    do
    {
        dwError = NetShareEnum(
            gState.pwszServerName,
            dwLevel,
            OUT_PPVOID(&pShareInfo),
            dwMaxLen,
            &dwNumEntries,
            &dwTotalEntries,
            &dwResume);
        BAIL_ON_NFSSVC_ERROR(dwError);

        dwSeenEntries += dwNumEntries;

        for (dwIndex = 0; dwIndex < dwNumEntries; dwIndex++)
        {
            dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi502_netname, &pszShareName);
            BAIL_ON_NFSSVC_ERROR(dwError);

            printf("[%s]\n", pszShareName);
            
            dwError = DumpShareInfo(&pShareInfo[dwIndex]);
            BAIL_ON_NFSSVC_ERROR(dwError);

            printf("\n");

            LW_SAFE_FREE_STRING(pszShareName);
        }

        if (pShareInfo)
        {
            NfsSvcFreeMemory(pShareInfo);
            pShareInfo = NULL;
        }
    } while (dwSeenEntries < dwTotalEntries);

cleanup:

    LW_SAFE_FREE_STRING(pszShareName);

    if (pShareInfo)
    {
        NfsSvcFreeMemory(pShareInfo);
        pShareInfo = NULL;
    }
    
    return dwError;

error:

    goto cleanup;
}

static
DWORD
GetInfo(
    int argc,
    char** ppszArgv
    )
{
    static const DWORD dwLevel = 502;
    DWORD dwError = 0;
    PSHARE_INFO_502 pShareInfo = NULL;
    PWSTR pwszShareName = NULL;

    dwError = LwMbsToWc16s(ppszArgv[1], &pwszShareName);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NetShareGetInfo(
        gState.pwszServerName,
        pwszShareName,
        dwLevel,
        OUT_PPVOID(&pShareInfo));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = DumpShareInfo(pShareInfo);
    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:

    if (pShareInfo)
    {
        NfsSvcFreeMemory(pShareInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszShareName);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SetInfo(
    int argc,
    char** ppszArgv
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

    dwError = ParseShareArgs(
        argc,
        ppszArgv);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NetShareGetInfo(
        gState.pwszServerName,
        gState.pwszTarget,
        dwLevel,
        OUT_PPVOID(&pShareInfo));
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = DeconstructSecurityDescriptor(
        pShareInfo->shi502_reserved,
        (PSECURITY_DESCRIPTOR_RELATIVE) pShareInfo->shi502_security_descriptor,
        &dwAllowUserCount,
        &ppwszAllowUsers,
        &dwDenyUserCount,
        &ppwszDenyUsers,
        &bReadOnly);
    BAIL_ON_NFSSVC_ERROR(dwError);

    newShareInfo = *pShareInfo;

    if (gState.pwszName)
    {
        newShareInfo.shi502_netname = gState.pwszName;
    }

    if (gState.pwszComment)
    {
        newShareInfo.shi502_remark = gState.pwszComment;
    }

    if (gState.pwszPath)
    {
        newShareInfo.shi502_path = gState.pwszPath;
    }

    dwError = ConstructSecurityDescriptor(
        gState.dwAllowUserCount || gState.bClearAllow ? gState.dwAllowUserCount : dwAllowUserCount,
        gState.dwAllowUserCount || gState.bClearAllow ? gState.ppwszAllowUsers : ppwszAllowUsers,
        gState.dwDenyUserCount || gState.bClearDeny ? gState.dwDenyUserCount : dwDenyUserCount,
        gState.dwDenyUserCount || gState.bClearDeny ? gState.ppwszDenyUsers : ppwszDenyUsers,
        gState.bReadOnly || gState.bReadWrite ? (gState.bReadOnly && !gState.bReadWrite) : bReadOnly,
        &pSecDesc,
        &dwSecDescSize);
    BAIL_ON_NFSSVC_ERROR(dwError);
        
    newShareInfo.shi502_reserved = dwSecDescSize;
    newShareInfo.shi502_security_descriptor = (PBYTE) pSecDesc;

    dwError = NetShareSetInfo(
        gState.pwszServerName,
        gState.pwszTarget,
        dwLevel,
        &newShareInfo,
        &dwParmErr);
    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:

    if (pShareInfo)
    {
        NfsSvcFreeMemory(pShareInfo);
    }

    FreeStringArray(dwAllowUserCount, ppwszAllowUsers);
    FreeStringArray(dwDenyUserCount, ppwszDenyUsers);

    LW_SAFE_FREE_MEMORY(pSecDesc);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
Add(
    int argc,
    char** ppszArgv
    )
{
    static const DWORD dwLevel = 502;

    DWORD dwError = 0;
    SHARE_INFO_502 shareInfo = {0};
    DWORD dwParmErr = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    DWORD dwSecDescSize = 0;

    dwError = ParseShareArgs(
        argc,
        ppszArgv);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = ConstructSecurityDescriptor(
        gState.dwAllowUserCount,
        gState.ppwszAllowUsers,
        gState.dwDenyUserCount,
        gState.ppwszDenyUsers,
        gState.bReadOnly && !gState.bReadWrite,
        &pSecDesc,
        &dwSecDescSize);
    BAIL_ON_NFSSVC_ERROR(dwError);

    shareInfo.shi502_type = 0; // SHARE_SERVICE_DISK_SHARE
    shareInfo.shi502_netname = gState.pwszName ? gState.pwszName : gState.pwszTarget;
    shareInfo.shi502_remark = gState.pwszComment;
    shareInfo.shi502_path = gState.pwszPath;
    shareInfo.shi502_reserved = dwSecDescSize;
    shareInfo.shi502_security_descriptor = (PBYTE) pSecDesc;

    dwError = NetShareAdd(
        gState.pwszServerName,
        dwLevel,
        &shareInfo,
        &dwParmErr);
    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pSecDesc);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
Del(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszShareName = NULL;

    dwError = LwMbsToWc16s(ppszArgv[1], &pwszShareName);
    BAIL_ON_NFSSVC_ERROR(dwError);

    dwError = NetShareDel(
        gState.pwszServerName,
        pwszShareName,
        0);
    BAIL_ON_NFSSVC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pwszShareName);

    return dwError;

error:

    goto cleanup;
}


int
main(
    int argc,
    char** ppszArgv
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;

    dwError = NfsSvcInitMemory();
    BAIL_ON_NFSSVC_ERROR(dwError);

    if (argc < 2)
    {
        Usage();
        return 1;
    }

    for (dwIndex = 1; dwIndex < argc; dwIndex++)
    {
        if (!strcasecmp(ppszArgv[dwIndex], "--server"))
        {
            dwError = LwMbsToWc16s(ppszArgv[++dwIndex], &gState.pwszServerName);
            BAIL_ON_NFSSVC_ERROR(dwError);
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "--help"))
        {
            Help();
            return 1;
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "--usage"))
        {
            Help();
            return 1;
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "enum"))
        {
            dwError = Enum(argc - dwIndex, ppszArgv + dwIndex);
            BAIL_ON_NFSSVC_ERROR(dwError);
            break;
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "enum-info"))
        {
            dwError = EnumInfo(argc - dwIndex, ppszArgv + dwIndex);
            BAIL_ON_NFSSVC_ERROR(dwError);
            break;
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "get-info"))
        {
            dwError = GetInfo(argc - dwIndex, ppszArgv + dwIndex);
            BAIL_ON_NFSSVC_ERROR(dwError);
            break;
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "set-info"))
        {
            dwError = SetInfo(argc - dwIndex, ppszArgv + dwIndex);
            BAIL_ON_NFSSVC_ERROR(dwError);
            break;
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "add"))
        {
            dwError = Add(argc - dwIndex, ppszArgv + dwIndex);
            BAIL_ON_NFSSVC_ERROR(dwError);
            break;
        }
        else if (!strcasecmp(ppszArgv[dwIndex], "del"))
        {
            dwError = Del(argc - dwIndex, ppszArgv + dwIndex);
            BAIL_ON_NFSSVC_ERROR(dwError);
            break;
        }
        else
        {
            Usage();
            return 1;
        }
    }
    
error:

    if (dwError)
    {
        fprintf(stderr, "%s\n", LwWin32ExtErrorToName(dwError));
        return 1;
    }
    else
    {
        return 0;
    }
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
