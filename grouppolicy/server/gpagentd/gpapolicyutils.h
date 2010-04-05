#ifndef GPAPOLICYUTILS_H_
#define GPAPOLICYUTILS_H_

VOID
GPAFreeGPOList(
    PGROUP_POLICY_OBJECT pGPOObject
    );

#define GPA_SAFE_FREE_GPO_LIST(pGPOList) \
	if (pGPOList) {                      \
		GPAFreeGPOList(pGPOList);        \
		pGPOList = NULL;                 \
	}

CENTERROR
GPAGetGPOList(
    PGPUSER pUser,
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    PGROUP_POLICY_OBJECT * ppGPOList
    );

CENTERROR
GPAPrependGPList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pPrependGPOList,
    PGROUP_POLICY_OBJECT * ppGPOCurrentList
    );

CENTERROR
GPAReverseGPOList(
    PGROUP_POLICY_OBJECT pGPOList,
    PGROUP_POLICY_OBJECT *ppGPOList
    );

CENTERROR
GPAComputeModDelGroupPolicyList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT *ppGPOModifiedList,
    PGROUP_POLICY_OBJECT *ppGPODeletedList
    );

CENTERROR
GPAComputeCSEModDelList(
    DWORD dwCSEType,
    PSTR pszCSEGUID,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList,
    PGROUP_POLICY_OBJECT *ppGPOCSEModList,
    PGROUP_POLICY_OBJECT *ppGPOCSEDelList
    );

CENTERROR
GPACopyGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT *ppGPOCopyObject
    );

void
GPAPostCSEEvent(
    CENTERROR ceError,
    PGPUSER pUserInfo,
    PSTR szCSEName,
    PSTR szCSEDllName,
    PGROUP_POLICY_OBJECT pGPOCSEModList,
    PGROUP_POLICY_OBJECT pGPOCSEDelList
    );

void
GPAPostAgentEvent(
    CENTERROR ceError,
    int       issue,
    PSTR      szMessage
    );

#endif /*GPAPOLICYUTILS_H_*/
