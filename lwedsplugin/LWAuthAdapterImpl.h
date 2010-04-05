/*
 *  LWAuthAdapterImpl.h
 *  LWIDSPlugIn
 *
 *  Created by Glenn Curtis on 6/20/08.
 *  Copyright 2007 Likewise Software. All rights reserved.
 *
 *  This class is a wrapper which will call into one of the two authentication mechanisms we support.
 *  If we are running as LSASS mode, then the LWISAuthAdapter class methods will be used.
 *
 */

#ifndef __LWAUTHADAPTERIMPL_H__
#define __LWAUTHADAPTERIMPL_H__

#include "LWIPlugIn.h"


class LWAuthAdapterImpl
{
public:

    LWAuthAdapterImpl() {}
    virtual ~LWAuthAdapterImpl() {}

public:
	
	virtual MACERROR Initialize() = 0;
	virtual void Cleanup() = 0;

	virtual void setpwent(void) = 0;

	virtual void endpwent(void) = 0;

	virtual long getpwent(struct passwd *result,
                              char *buffer,
                              size_t buflen,
                              int *errnop) = 0;

	virtual long getpwuid(uid_t uid,
                              struct passwd *result,
                              char *buffer,
                              size_t buflen,
                              int *errnop) = 0;

	virtual long getpwnam(const char *name,
                              struct passwd *result,
                              char *buffer,
                              size_t buflen,
                              int *errnop) = 0;

	virtual void setgrent(void) = 0;

	virtual void endgrent(void) = 0;

	virtual long getgrent(struct group *result,
                              char *buffer,
                              size_t buflen,
                              int *errnop) = 0;
     
	virtual long getgrgid(gid_t gid,
                              struct group *result,
                              char *buffer,
                              size_t buflen,
                              int *errnop) = 0;

	virtual long getgrnam(const char *name,
                              struct group *result,
                              char *buffer,
                              size_t buflen,
                              int *errnop) = 0;

	virtual uint32_t authenticate(const char *username,
                                      const char *password,
                                      bool is_auth_only) = 0;

	virtual uint32_t change_password(const char *username,
                                         const char *old_password,
                                         const char *password) = 0;

	virtual uint32_t get_user_groups(const char *user,
                                         gid_t **groups,
                                         int *num_groups) = 0;

        virtual void free_user_groups(gid_t * groups) = 0;
	
};

extern LWAuthAdapterImpl * MyAuthAdapter;
 
#endif /* __LWAUTHADAPTERIMPL_H__ */
