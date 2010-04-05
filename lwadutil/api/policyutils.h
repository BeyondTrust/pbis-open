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
    DWORD  dwPolicyType,
    PCSTR  pszObjectDN,
    PSTR  pszClientGUID,
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
    PSTR  pszCSEGUID,
    PGROUP_POLICY_OBJECT  pGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCSEList
    );

#endif /*ADUPOLICYUTILS_H_*/
