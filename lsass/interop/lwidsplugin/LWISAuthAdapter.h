/*
 *  LWISAuthAdapter.h
 *  LWIDSPlugIn
 *
 *  Created by Glenn Curtis on 6/20/08.
 *  Copyright 2007 Likewise Software. All rights reserved.
 *
 */

#ifndef __LWISAUTHADAPTER_H__
#define __LWISAUTHADAPTER_H__

#include "LWIPlugIn.h"
#include "LWAuthAdapterImpl.h"

//
// From LSASS
//
typedef enum
{
    LSASS_STATUS_SUCCESS  = 0,
    LSASS_STATUS_NOTFOUND = 1,
    LSASS_STATUS_UNAVAIL  = 2,
    LSASS_STATUS_TRYAGAIN = 3
} LSASS_STATUS;

class LWISAuthAdapter : public LWAuthAdapterImpl
{
public:

    LWISAuthAdapter();
    ~LWISAuthAdapter();

public:
	
    MACERROR Initialize();
    void Cleanup();

    void setpwent(void);

    void endpwent(void);

    long getpwent(struct passwd *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getpwuid(uid_t uid,
                  struct passwd *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getpwnam(const char *name,
                  struct passwd *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    void setgrent(void);

    void endgrent(void);

    long getgrent(struct group *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getgrgid(gid_t gid,
                  struct group *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    long getgrnam(const char *name,
                  struct group *result,
                  char *buffer,
                  size_t buflen,
                  int *errnop);

    uint32_t authenticate(const char *username,
                          const char *password,
                          bool is_auth_only);

    uint32_t change_password(const char *username,
                             const char *old_password,
                             const char *password);

    uint32_t get_principal(const char *username,
                           char** principal_name);

    void free_principal(char* principal_name);

    uint32_t get_user_groups(const char *user,
                             gid_t **groups,
                             int *num_groups);

    void free_user_groups(gid_t *groups);

private:

    static MACERROR LoadFunction(void* libHandle, const char* functionName, void** functionPointer);
    void EnterNSSLock();
    void LeaveNSSLock();
    
    pthread_mutex_t _nssLock;
};

#endif /* __LWISAUTHADAPTER_H__ */
