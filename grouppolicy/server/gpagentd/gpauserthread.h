#ifndef __GPAUSERTHREAD_H__
#define __GPAUSERTHREAD_H__

CENTERROR
GPAStartUserPolicyThread();

CENTERROR
GPAStopUserPolicyThread();

CENTERROR
GPARefreshUserPolicies();

CENTERROR
GPAProcessLogin(
    PCSTR pszUsername
    );

CENTERROR
GPAProcessLogout(
    PCSTR pszUsername
    );

#endif /* __GPAUSERTHREAD_H__ */
