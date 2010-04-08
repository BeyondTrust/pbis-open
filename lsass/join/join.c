/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        join.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

#define LSA_JOIN_OU_PREFIX "OU="
#define LSA_JOIN_CN_PREFIX "CN="
#define LSA_JOIN_DC_PREFIX "DC="

#define LSA_JOIN_MAX_ALLOWED_CLOCK_DRIFT_SECONDS 60


DWORD
LsaNetJoinDomain(
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    PSTR  pszOU_DN = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszHostDnsDomain = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszOU = NULL;
    PWSTR pwszOSName = NULL;
    PWSTR pwszOSVersion = NULL;
    PWSTR pwszOSServicePack = NULL;
    DWORD dwOptions = (NETSETUP_JOIN_DOMAIN |
                       NETSETUP_ACCT_CREATE |
                       NETSETUP_DOMAIN_JOIN_IF_JOINED);
    PLSA_CREDS_FREE_INFO pAccessInfo = NULL;

    BAIL_ON_INVALID_STRING(pszHostname);
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszUsername);

    if (geteuid() != 0) {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( !(dwFlags & LSA_NET_JOIN_DOMAIN_NOTIMESYNC) )
    {
        dwError = LsaSyncTimeToDC(pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSetSMBCreds(
                pszDomain,
                pszUsername,
                pszPassword,
                TRUE,
                &pAccessInfo,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMbsToWc16s(
                    pszHostname,
                    &pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszHostDnsDomain))
    {
        dwError = LsaMbsToWc16s(
                        pszHostDnsDomain,
                        &pwszHostDnsDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaMbsToWc16s(
                    pszDomain,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszOU)) {
            
        dwError = LsaBuildOrgUnitDN(
                    pszDomain,
                    pszOU,
                    &pszOU_DN);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaMbsToWc16s(
                    pszOU_DN,
                    &pwszOU);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSName)) {
        dwError = LsaMbsToWc16s(
                    pszOSName,
                    &pwszOSName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSVersion)) {
        dwError = LsaMbsToWc16s(
                    pszOSVersion,
                    &pwszOSVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSServicePack)) {
        dwError = LsaMbsToWc16s(
                    pszOSServicePack,
                    &pwszOSServicePack);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = NetJoinDomainLocal(
            pwszHostname,
            pwszHostDnsDomain,
            pwszDomain,
            pwszOU,
            NULL,
            NULL,
            dwOptions,
            pwszOSName,
            pwszOSVersion,
            pwszOSServicePack);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaFreeSMBCreds(&pAccessInfo);

    LW_SAFE_FREE_STRING(pszOU_DN);
    LW_SAFE_FREE_MEMORY(pwszHostname);
    LW_SAFE_FREE_MEMORY(pwszHostDnsDomain);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszOU);
    LW_SAFE_FREE_MEMORY(pwszOSName);
    LW_SAFE_FREE_MEMORY(pwszOSVersion);
    LW_SAFE_FREE_MEMORY(pwszOSServicePack);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaBuildOrgUnitDN(
    PCSTR pszDomain,
    PCSTR pszOU,
    PSTR* ppszOU_DN
    )
{
    DWORD dwError = 0;
    PSTR  pszOuDN = NULL;
    // Do not free
    PSTR  pszOutputPos = NULL;
    PCSTR pszInputPos = NULL;
    PCSTR pszInputSectionEnd = NULL;
    size_t sOutputDnLen = 0;
    size_t sSectionLen = 0;
    DWORD nDomainParts = 0;
    
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszOU);
    
    // Figure out the length required to write the OU DN
    pszInputPos = pszOU;

    // skip leading slashes
    sSectionLen = strspn(pszInputPos, "/");
    pszInputPos += sSectionLen;

    while ((sSectionLen = strcspn(pszInputPos, "/")) != 0) {
        sOutputDnLen += sizeof(LSA_JOIN_OU_PREFIX) - 1;
        sOutputDnLen += sSectionLen;
        // For the separating comma
        sOutputDnLen++;
        
        pszInputPos += sSectionLen;
        
        sSectionLen = strspn(pszInputPos, "/");
        pszInputPos += sSectionLen;
    }
    
    // Figure out the length required to write the Domain DN
    pszInputPos = pszDomain;
    while ((sSectionLen = strcspn(pszInputPos, ".")) != 0) {
        sOutputDnLen += sizeof(LSA_JOIN_DC_PREFIX) - 1;
        sOutputDnLen += sSectionLen;
        nDomainParts++;
        
        pszInputPos += sSectionLen;
        
        sSectionLen = strspn(pszInputPos, ".");
        pszInputPos += sSectionLen;
    }

    // Add in space for the separating commas
    if (nDomainParts > 1)
    {
        sOutputDnLen += nDomainParts - 1;
    }
    
    dwError = LwAllocateMemory(
                    sizeof(CHAR) * (sOutputDnLen + 1),
                    (PVOID*)&pszOuDN);
    BAIL_ON_LSA_ERROR(dwError);

    pszOutputPos = pszOuDN;
    // Iterate through pszOU backwards and write to pszOuDN forwards
    pszInputPos = pszOU + strlen(pszOU) - 1;

    while(TRUE)
    {
        // strip trailing slashes
        while (pszInputPos >= pszOU && *pszInputPos == '/')
        {
            pszInputPos--;
        }

        if (pszInputPos < pszOU)
        {
            break;
        }

        // Find the end of this section (so that we can copy it to
        // the output string in forward order).
        pszInputSectionEnd = pszInputPos;
        while (pszInputPos >= pszOU && *pszInputPos != '/')
        {
            pszInputPos--;
        }
        sSectionLen = pszInputSectionEnd - pszInputPos;

        // Only "Computers" as the first element is a CN.
        if ((pszOutputPos ==  pszOuDN) &&
            (sSectionLen == sizeof("Computers") - 1) &&
            !strncasecmp(pszInputPos + 1, "Computers", sizeof("Computers") - 1))
        {
            // Add CN=<name>,
            memcpy(pszOutputPos, LSA_JOIN_CN_PREFIX,
                    sizeof(LSA_JOIN_CN_PREFIX) - 1);
            pszOutputPos += sizeof(LSA_JOIN_CN_PREFIX) - 1;
        }
        else
        {
            // Add OU=<name>,
            memcpy(pszOutputPos, LSA_JOIN_OU_PREFIX,
                    sizeof(LSA_JOIN_OU_PREFIX) - 1);
            pszOutputPos += sizeof(LSA_JOIN_OU_PREFIX) - 1;
        }

        memcpy(pszOutputPos,
                pszInputPos + 1,
                sSectionLen);
        pszOutputPos += sSectionLen;

        *pszOutputPos++ = ',';
    }

    // Make sure to overwrite any initial "CN=Computers" as "OU=Computers".
    // Note that it is safe to always set "OU=" as the start of the DN
    // unless the DN so far is exacly "CN=Computers,".
    if (strcasecmp(pszOuDN, LSA_JOIN_CN_PREFIX "Computers,"))
    {
        memcpy(pszOuDN, LSA_JOIN_OU_PREFIX, sizeof(LSA_JOIN_OU_PREFIX) - 1);
    }
    
    // Read the domain name foward in sections and write it back out
    // forward.
    pszInputPos = pszDomain;
    while (TRUE)
    {
        sSectionLen = strcspn(pszInputPos, ".");

        memcpy(pszOutputPos,
                LSA_JOIN_DC_PREFIX,
                sizeof(LSA_JOIN_DC_PREFIX) - 1);
        pszOutputPos += sizeof(LSA_JOIN_DC_PREFIX) - 1;
        
        memcpy(pszOutputPos, pszInputPos, sSectionLen);
        pszOutputPos += sSectionLen;
        
        pszInputPos += sSectionLen;
        
        sSectionLen = strspn(pszInputPos, ".");
        pszInputPos += sSectionLen;

        if (*pszInputPos != 0)
        {
            // Add a comma for the next entry
            *pszOutputPos++ = ',';
        }
        else
            break;

    }
    
    assert(pszOutputPos == pszOuDN + sizeof(CHAR) * (sOutputDnLen));
    *pszOutputPos = 0;

    *ppszOU_DN = pszOuDN;
    
cleanup:

    return dwError;
    
error:

    *ppszOU_DN = NULL;
    
    LW_SAFE_FREE_STRING(pszOuDN);

    goto cleanup;
}

DWORD
LsaNetTestJoinDomain(
    PBOOLEAN pbIsJoined
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsJoined = FALSE;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PSTR pszHostname = NULL;
    
    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsOpenPasswordStore(
                LWPS_PASSWORD_STORE_DEFAULT,
                &hStore);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsGetPasswordByHostName(
                hStore,
                pszHostname,
                &pPassInfo);

    switch(dwError)
    {
        case LWPS_ERROR_INVALID_ACCOUNT:
            bIsJoined = FALSE;
            dwError = 0;
            break;
        case 0:
            bIsJoined = TRUE;
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
    }
    
    *pbIsJoined = bIsJoined;
    
cleanup:

    if (pPassInfo) {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }
    
    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }
    
    LW_SAFE_FREE_STRING(pszHostname);

    return dwError;
    
error:

    *pbIsJoined = FALSE;

    goto cleanup;
}

VOID
LsaEnableDebugLog(
    VOID
    )
{
    NetEnableDebug();
}

VOID
LsaDisableDebugLog(
    VOID
    )
{
    NetDisableDebug();
}

DWORD
LsaSyncTimeToDC(
    PCSTR  pszDomain
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T dcTime = 0;
    time_t ttDCTime = 0;
    
    dwError = LWNetGetDCTime(
                    pszDomain,
                    &dcTime);
    BAIL_ON_LSA_ERROR(dwError);
    
    ttDCTime = (time_t) dcTime;
    
    if (labs(ttDCTime - time(NULL)) > LSA_JOIN_MAX_ALLOWED_CLOCK_DRIFT_SECONDS) {
        dwError = LwSetSystemTime(ttDCTime);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaBuildMachineAccountInfo(
    PLWPS_PASSWORD_INFO pInfo,
    PLSA_MACHINE_ACCT_INFO* ppAcctInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCT_INFO pAcctInfo = NULL;
    
    dwError = LwAllocateMemory(
                    sizeof(LSA_MACHINE_ACCT_INFO),
                    (PVOID*)&pAcctInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszDnsDomainName,
                    &pAcctInfo->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszDomainName,
                    &pAcctInfo->pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszHostname,
                    &pAcctInfo->pszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszMachineAccount,
                    &pAcctInfo->pszMachineAccount);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszMachinePassword,
                    &pAcctInfo->pszMachinePassword);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszSID,
                    &pAcctInfo->pszSID);
    BAIL_ON_LSA_ERROR(dwError);
    
    pAcctInfo->dwSchannelType = pInfo->dwSchannelType;
    pAcctInfo->last_change_time = pInfo->last_change_time;
    
    *ppAcctInfo = pAcctInfo;
    
cleanup:

    return dwError;
    
error:

    *ppAcctInfo = NULL;
    
    if (pAcctInfo) {
        LsaFreeMachineAccountInfo(pAcctInfo);
    }
    
    goto cleanup;
}

VOID
LsaFreeMachineAccountInfo(
    PLSA_MACHINE_ACCT_INFO pAcctInfo
    )
{
    LW_SAFE_FREE_STRING(pAcctInfo->pszDnsDomainName);
    LW_SAFE_FREE_STRING(pAcctInfo->pszDomainName);
    LW_SAFE_FREE_STRING(pAcctInfo->pszHostname);
    LW_SAFE_FREE_STRING(pAcctInfo->pszMachineAccount);
    LW_SAFE_FREE_STRING(pAcctInfo->pszMachinePassword);
    LW_SAFE_FREE_STRING(pAcctInfo->pszSID);
    LwFreeMemory(pAcctInfo);
}


DWORD
LsaEnableDomainGroupMembership(
    PCSTR pszDomainName
    )
{
    return LsaChangeDomainGroupMembership(pszDomainName,
					  TRUE);
}


DWORD
LsaChangeDomainGroupMembership(
    IN  PCSTR    pszDomainName,
    IN  BOOLEAN  bEnable
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    RPCSTATUS rpcStatus = RPC_S_OK;
    handle_t hLsaBinding = NULL;
    WCHAR wszLocalSystem[] = { '\\', '\\', '\0' };
    PWSTR pwszSystem = wszLocalSystem;
    DWORD dwLocalPolicyAccessMask = LSA_ACCESS_VIEW_POLICY_INFO;
    POLICY_HANDLE hLocalPolicy = NULL;
    LsaPolicyInformation *pInfo = NULL;
    PSID pDomainSid = NULL;
    handle_t hSamrBinding = NULL;
    DWORD dwLocalSamrAccessMask = SAMR_ACCESS_ENUM_DOMAINS |
                                  SAMR_ACCESS_OPEN_DOMAIN;
    SamrConnectInfo ConnReq;
    DWORD dwConnReqLevel = 0;
    SamrConnectInfo ConnInfo;
    DWORD dwConnLevel = 0;
    CONNECT_HANDLE hSamrConn = NULL;
    DWORD dwBuiltinDomainAccessMask = DOMAIN_ACCESS_OPEN_ACCOUNT;
    PSID pBuiltinDomainSid = NULL;
    DOMAIN_HANDLE hBuiltinDomain = NULL;
    DWORD dwAliasAccessMask = ALIAS_ACCESS_ADD_MEMBER |
                              ALIAS_ACCESS_REMOVE_MEMBER;
    ACCOUNT_HANDLE hAlias = NULL;
    PSID pDomainAdminsSid = NULL;
    PSID pDomainUsersSid = NULL;
    DWORD iGroup = 0;
    DWORD iMember = 0;

    PSID *pppAdminMembers[] = { &pDomainAdminsSid, NULL };
    PSID *pppUsersMembers[] = { &pDomainUsersSid, NULL };

    struct _LOCAL_GROUP_MEMBERS
    {
        DWORD   dwLocalGroupRid;
        PSID  **pppMembers;
    }
    Memberships[] =
    {
        { DOMAIN_ALIAS_RID_ADMINS,
          pppAdminMembers
        },
        { DOMAIN_ALIAS_RID_USERS,
          pppUsersMembers
        }
    };

    memset(&ConnReq, 0, sizeof(ConnReq));
    memset(&ConnInfo, 0, sizeof(ConnInfo));

    /*
     * Connect local lsa rpc server and get basic
     * domain information
     */
    rpcStatus = InitLsaBindingDefault(&hLsaBinding,
                                      NULL,
                                      NULL);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
    }

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszSystem,
                              NULL,
                              dwLocalPolicyAccessMask,
                              &hLocalPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaQueryInfoPolicy2(hLsaBinding,
                                   hLocalPolicy,
                                   LSA_POLICY_INFO_DOMAIN,
                                   &pInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pDomainSid = pInfo->domain.sid;

    dwError = LwCreateWellKnownSid(WinAccountDomainAdminsSid,
                                   pDomainSid,
                                   &pDomainAdminsSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwCreateWellKnownSid(WinAccountDomainUsersSid,
                                   pDomainSid,
                                   &pDomainUsersSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Connect local samr rpc server and open BUILTIN domain
     */
    rpcStatus = InitSamrBindingDefault(&hSamrBinding,
                                       NULL,
                                       NULL);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
    }

    ConnReq.info1.client_version = SAMR_CONNECT_POST_WIN2K;
    dwConnReqLevel = 1;

    ntStatus = SamrConnect5(hSamrBinding,
                            pwszSystem,
                            dwLocalSamrAccessMask,
                            dwConnReqLevel,
                            &ConnReq,
                            &dwConnLevel,
                            &ConnInfo,
                            &hSamrConn);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwCreateWellKnownSid(WinBuiltinDomainSid,
                                   NULL,
                                   &pBuiltinDomainSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrOpenDomain(hSamrBinding,
                              hSamrConn,
                              dwBuiltinDomainAccessMask,
                              pBuiltinDomainSid,
                              &hBuiltinDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Add requested domain groups or users to their
     * corresponding local groups
     */
    for (iGroup = 0;
         iGroup < sizeof(Memberships)/sizeof(Memberships[0]);
         iGroup++)
    {
        DWORD dwRid = Memberships[iGroup].dwLocalGroupRid;

        ntStatus = SamrOpenAlias(hSamrBinding,
                                 hBuiltinDomain,
                                 dwAliasAccessMask,
                                 dwRid,
                                 &hAlias);
        BAIL_ON_NT_STATUS(ntStatus);

        for (iMember = 0;
             Memberships[iGroup].pppMembers[iMember] != NULL;
             iMember++)
        {
            PSID *ppSid = Memberships[iGroup].pppMembers[iMember];

            if (bEnable)
            {
                ntStatus = SamrAddAliasMember(hSamrBinding,
                                              hAlias,
                                              (*ppSid));
                if (ntStatus == STATUS_MEMBER_IN_ALIAS)
                {
                    ntStatus = STATUS_SUCCESS;
                }
            }
            else
            {
                // This should not cause the join to fail even if we cannot
                // remove the group members

                ntStatus = SamrDeleteAliasMember(hSamrBinding,
                                                 hAlias,
                                                 (*ppSid));
                if ((ntStatus != STATUS_SUCCESS) && 
                    (ntStatus != STATUS_NO_SUCH_MEMBER))
                {
                    // Perhaps log an error here
                    ;
                }
                ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SamrClose(hSamrBinding, hAlias);
        BAIL_ON_NT_STATUS(ntStatus);

        hAlias = NULL;
    }

cleanup:
    if (hSamrBinding && hAlias)
    {
        SamrClose(hSamrBinding, hAlias);
    }

    if (hSamrBinding && hBuiltinDomain)
    {
        SamrClose(hSamrBinding, hBuiltinDomain);
    }

    if (hSamrBinding && hSamrConn)
    {
        SamrClose(hSamrBinding, hSamrConn);
    }

    if (pInfo)
    {
        LsaRpcFreeMemory(pInfo);
    }

    if (hLsaBinding && hLocalPolicy)
    {
        LsaClose(hLsaBinding, hLocalPolicy);
    }

    FreeSamrBinding(&hSamrBinding);
    FreeLsaBinding(&hLsaBinding);

    LW_SAFE_FREE_MEMORY(pBuiltinDomainSid);
    LW_SAFE_FREE_MEMORY(pDomainAdminsSid);
    LW_SAFE_FREE_MEMORY(pDomainUsersSid);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
