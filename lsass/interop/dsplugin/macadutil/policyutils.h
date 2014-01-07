/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef ADUPOLICYUTILS_H_
#define ADUPOLICYUTILS_H_

VOID
ADUFreeGPOList(
    PGROUP_POLICY_OBJECT pGPOObject
    );

#define ADU_SAFE_FREE_GPO_LIST(pGPOList) \
	if (pGPOList) {                      \
		ADUFreeGPOList(pGPOList);        \
		pGPOList = NULL;                 \
	}
	
BOOLEAN
ADUFindMatch(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT pGPOObjectList,
    PGROUP_POLICY_OBJECT *ppGPOMatchedObject,
    BOOLEAN * pfNewVersion
    );

DWORD	
ADUComputeDeletedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPODeletedList
    );
	
DWORD
ADUComputeModifiedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPOModifiedList
    );

DWORD
ADUGetAllMCXGPOList(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PGROUP_POLICY_OBJECT * ppGPOList
    );

DWORD
ADUGetMCXGPO(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PCSTR  pszGPOName,
    PGROUP_POLICY_OBJECT * ppGPOList
    );
    
DWORD
ADUPrependGPList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pPrependGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCurrentList
    );

DWORD
ADUReverseGPOList(
    PGROUP_POLICY_OBJECT pGPOList,
    PGROUP_POLICY_OBJECT *ppGPOList
    );

DWORD
ADUComputeModDelGroupPolicyList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT *ppGPOModifiedList,
    PGROUP_POLICY_OBJECT *ppGPODeletedList
    );

DWORD
ADUComputeCSEModDelList(
    DWORD dwCSEType,
    PSTR pszCSEGUID,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList,
    PGROUP_POLICY_OBJECT *ppGPOCSEModList,
    PGROUP_POLICY_OBJECT *ppGPOCSEDelList
    );

DWORD
ADUCopyGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT *ppGPOCopyObject
    );
	
DWORD
ADUComputeCSEList(
    DWORD dwCSEType,
    PCSTR pszCSEGUID,
    PGROUP_POLICY_OBJECT  pGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCSEList
    );

#endif /*ADUPOLICYUTILS_H_*/
