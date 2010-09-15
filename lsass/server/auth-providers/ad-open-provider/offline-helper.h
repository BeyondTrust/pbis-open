/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        provider-main.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */
#ifndef __OFFLINE_HELPER_H__
#define __OFFLINE_HELPER_H__

DWORD
AD_OfflineGetGroupMembers(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszGroupSid,
    OUT size_t* psMemberObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppMemberObjects
    );

DWORD
AD_OfflineFindObjectsBySidList(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

typedef BOOLEAN (*PFN_LSA_GATHER_SIDS_FROM_GROUP_MEMBERSHIP_CALLBACK)(
    IN PLSA_GROUP_MEMBERSHIP pMembership
    );

DWORD
AD_GatherSidsFromGroupMemberships(
    IN BOOLEAN bGatherParentSids,
    IN OPTIONAL PFN_LSA_GATHER_SIDS_FROM_GROUP_MEMBERSHIP_CALLBACK pfnIncludeCallback,
    IN size_t sMemberhipsCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMemberships,
    OUT size_t* psSidsCount,
    OUT PSTR** pppszSids
    );

struct _LSA_AD_GROUP_EXPANSION_DATA;
typedef struct _LSA_AD_GROUP_EXPANSION_DATA LSA_AD_GROUP_EXPANSION_DATA, *PLSA_AD_GROUP_EXPANSION_DATA;

DWORD
AD_GroupExpansionDataCreate(
    OUT PLSA_AD_GROUP_EXPANSION_DATA* ppExpansionData,
    IN DWORD dwMaxDepth
    );

DWORD
AD_GroupExpansionDataAddExpansionResults(
    IN PLSA_AD_GROUP_EXPANSION_DATA pExpansionData,
    IN DWORD dwExpandedGroupDepth,
    IN OUT size_t* psMembersCount,
    IN OUT PLSA_SECURITY_OBJECT** pppMembers
    );

DWORD
AD_GroupExpansionDataGetNextGroupToExpand(
    IN PLSA_AD_GROUP_EXPANSION_DATA pExpansionData,
    OUT PLSA_SECURITY_OBJECT* ppGroupToExpand,
    OUT PDWORD pdwGroupToExpandDepth
    );

DWORD
AD_GroupExpansionDataGetResults(
    IN PLSA_AD_GROUP_EXPANSION_DATA pExpansionData,
    OUT OPTIONAL PBOOLEAN pbIsFullyExpanded,
    OUT size_t* psUserMembersCount,
    OUT PLSA_SECURITY_OBJECT** pppUserMembers
    );

VOID
AD_GroupExpansionDataDestroy(
    IN OUT PLSA_AD_GROUP_EXPANSION_DATA pExpansionData
    );

#endif /* __OFFLINE_HELPER_H__ */
