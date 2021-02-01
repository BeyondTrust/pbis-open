/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
