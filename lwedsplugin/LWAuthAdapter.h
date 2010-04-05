/*
 *  LWAuthAdapter.h
 *  LWIDSPlugIn
 *
 *  Created by Glenn Curtis on 6/20/08.
 *  Copyright 2007 Likewise Software. All rights reserved.
 *
 *  This class is a wrapper which will call into one of the two authentication mechanisms we support.
 *  If we are running as LSASS mode, then the LWISAuthAdapter class methods will be used.
 *
 */

#ifndef __LWAUTHADAPTER_H__
#define __LWAUTHADAPTER_H__

#include "LWAuthAdapterImpl.h"
#include "LWIPlugIn.h"

//
// From LSASS NSS
//
typedef enum
{
  NSS_STATUS_TRYAGAIN = -2,
  NSS_STATUS_UNAVAIL,
  NSS_STATUS_NOTFOUND,
  NSS_STATUS_SUCCESS,
  NSS_STATUS_RETURN
} NSS_STATUS;

class LWAuthAdapter
{
private:

    // cannot instantiate
    LWAuthAdapter();
    ~LWAuthAdapter();

public:
	
    static MACERROR Initialize();
    static void Cleanup();

    static void setpwent(void);

    static void endpwent(void);

    static long getpwent(struct passwd *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop);

    static long getpwuid(uid_t uid,
                         struct passwd *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop);

    static long getpwnam(const char *name,
                         struct passwd *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop);

    static void setgrent(void);

    static void endgrent(void);

    static long getgrent(struct group *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop);

    static long getgrgid(gid_t gid,
                         struct group *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop);

    static long getgrnam(const char *name,
                         struct group *result,
                         char *buffer,
                         size_t buflen,
                         int *errnop);

    static uint32_t authenticate(const char *username,
                                 const char *password,
                                 bool is_auth_only);

    static uint32_t change_password(const char *username,
                                    const char *old_password,
                                    const char *password);

    static uint32_t get_user_groups(const char *user,
                                    gid_t **groups,
                                    int *num_groups);

    static void free_user_groups(gid_t * groups);

private:
	
	static class LWAuthAdapterImpl * m_pImpl;
};

#endif /* __LWAUTHADAPTER_H__ */
