/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
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
