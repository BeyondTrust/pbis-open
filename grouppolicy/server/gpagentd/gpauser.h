#ifndef __GPUSER_H__
#define __GPUSER_H__

typedef enum
{
    GP_USER_POLICY_NEW = 0,
    GP_USER_POLICY_PROCESSING,
    GP_USER_POLICY_APPLIED,
    GP_USER_POLICY_FAILED
} GpUserPolicyStatus;
    
typedef struct __GPUSERCONTEXT
{
    pthread_mutex_t     lock;
    pthread_cond_t cond;
    
    PGPUSER      pUserInfo;
    time_t       lastRefreshTime;
    GpUserPolicyStatus policyStatus;
    PGROUP_POLICY_OBJECT pGPOList;
    
    struct __GPUSERCONTEXT *pNext;
    
} GPUSERCONTEXT, *PGPUSERCONTEXT;

#define GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserList) \
    if (pUserList) {                               \
       GPAFreeUserContextList(pUserList);          \
       pUserList = NULL;                           \
    }

void
GPAFreeUserContextList(
    PGPUSERCONTEXT pUserList
    );

CENTERROR
GPACloneUserContext(
        PGPUSERCONTEXT pContext,
        PGPUSERCONTEXT* ppCopyContext
        );

CENTERROR
GPAGetADUserInfoForLoginId(
    PCSTR pszLoginId,
    PGPUSERCONTEXT* ppUserContext
    );

CENTERROR
GPAGetADUserInfoForUID(
    PCSTR    pszLoginId,
	uid_t    uid,
	PGPUSERCONTEXT* ppUserContext
	);

CENTERROR
GPAGetCurrentADUsers(
    PSTR pszDistroName,
    PSTR pszDistroVersion,
    PGPUSERCONTEXT* ppUserList
    );

#endif /* __GPUSER_H__ */
