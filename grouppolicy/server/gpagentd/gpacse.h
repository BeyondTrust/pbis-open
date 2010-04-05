#ifndef __GPACSE_H__
#define __GPACSE_H__

typedef struct __CSEINFO {
    pthread_rwlock_t lock;
    PGROUP_POLICY_CLIENT_EXTENSION pGPCSEList;
} GPOCSEINFO, *PGPOCSEINFO;

void
InitCSEGlobals();

CENTERROR
LoadClientSideExtensions();

void
DestroyClientSideExtensions(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientSideExts
    );

BOOLEAN
UserPoliciesAreAvailable();

#endif /* __GPACSE_H__ */
