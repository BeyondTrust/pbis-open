#ifndef GPAPOLICYUTILS_H_
#define GPAPOLICYUTILS_H_

#include "macadutil.h"

VOID
GPAFreeGPOList(
    PGROUP_POLICY_OBJECT pGPOObject
    );

#define GPA_SAFE_FREE_GPO_LIST(pGPOList) \
	if (pGPOList) {                      \
		GPAFreeGPOList(pGPOList);        \
		pGPOList = NULL;                 \
	}
	
BOOLEAN
GPAFindMatch(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT pGPOObjectList,
    PGROUP_POLICY_OBJECT *ppGPOMatchedObject,
    BOOLEAN * pfNewVersion
    );

long	
GPAComputeDeletedList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGROUP_POLICY_OBJECT * ppGPODeletedList
    );

long
GPACopyGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject,
    PGROUP_POLICY_OBJECT *ppGPOCopyObject
    );

#endif /*GPAPOLICYUTILS_H_*/
