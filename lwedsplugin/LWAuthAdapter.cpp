/*
 *  LWAuthAdapter.cpp
 *  LWIDSPlugIn
 *
 *  Created by Glenn Curtis on 6/20/08.
 *  Copyright 2008 Likewise Software. All rights reserved.
 *
 */

#include "LWAuthAdapter.h"
#include "LWISAuthAdapter.h"
#include <sys/stat.h>

class LWAuthAdapterImpl * LWAuthAdapter::m_pImpl = NULL;

MACERROR LWAuthAdapter::Initialize(
    void
	)
{
    MACERROR macError = eDSNoErr;
    LWAuthAdapterImpl* pImpl = NULL;

    if (m_pImpl)
    {
        goto cleanup;
    }
	
    pImpl = new LWISAuthAdapter();
    macError = pImpl->Initialize();
    GOTO_CLEANUP_ON_MACERROR(macError);

    m_pImpl = pImpl;
    pImpl = NULL;

cleanup:

    if (pImpl)
    {
        pImpl->Cleanup();
        delete pImpl;
        pImpl = NULL;
    }

    return macError;
}

void LWAuthAdapter::Cleanup(
    void
	)
{
    if (m_pImpl)
    {
        m_pImpl->Cleanup();
        delete m_pImpl;
        m_pImpl = NULL;
    }
}

void
LWAuthAdapter::setpwent(
    void
	)
{
    if (m_pImpl)
        m_pImpl->setpwent();
}

void
LWAuthAdapter::endpwent(
    void
	)
{
    if (m_pImpl)
        m_pImpl->endpwent();
}

long
LWAuthAdapter::getpwent(
    struct passwd *result,
    char * buffer,
    size_t buflen,
    int *  errnop)
{
    if (m_pImpl)
        return m_pImpl->getpwent(result, buffer, buflen, errnop);
    else
        return NSS_STATUS_UNAVAIL;
}

long
LWAuthAdapter::getpwuid(
    uid_t uid,
    struct passwd *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    if (m_pImpl)
        return m_pImpl->getpwuid(uid, result, buffer, buflen, errnop);
    else
        return NSS_STATUS_UNAVAIL;
}

long
LWAuthAdapter::getpwnam(
    const char *name,
    struct passwd *result,
    char *buffer,
    size_t buflen,
    int *errnop
    )
{
    if (m_pImpl)
        return m_pImpl->getpwnam(name, result, buffer, buflen, errnop);
    else
        return NSS_STATUS_UNAVAIL;
}

void
LWAuthAdapter::setgrent(
    void
	)
{
    if (m_pImpl)
        m_pImpl->setgrent();
}

void
LWAuthAdapter::endgrent(
    void
	)
{
    if (m_pImpl)
        m_pImpl->endgrent();
}

long
LWAuthAdapter::getgrent(
    struct group *result,
    char * buffer,
    size_t buflen,
    int *  errnop
    )
{
    if (m_pImpl)
        return m_pImpl->getgrent(result, buffer, buflen, errnop);
    else
        return NSS_STATUS_UNAVAIL;
}

long
LWAuthAdapter::getgrgid(
    gid_t  gid,
    struct group *result,
    char * buffer,
    size_t buflen,
    int *  errnop
    )
{
    if (m_pImpl)
        return m_pImpl->getgrgid(gid, result, buffer, buflen, errnop);
    else
        return NSS_STATUS_UNAVAIL;
}

long
LWAuthAdapter::getgrnam(
    const char *   name,
    struct group * result,
    char *         buffer,
    size_t         buflen,
    int *          errnop
    )
{
    if (m_pImpl)
        return m_pImpl->getgrnam(name, result, buffer, buflen, errnop);
    else
        return NSS_STATUS_UNAVAIL;
}

uint32_t
LWAuthAdapter::authenticate(
    const char *username,
    const char *password,
    bool        is_auth_only
    )
{
    if (m_pImpl)
        return m_pImpl->authenticate(username, password, is_auth_only);
    else
        return NSS_STATUS_UNAVAIL;
}

uint32_t
LWAuthAdapter::change_password(
    const char *username,
    const char *old_password,
    const char *password
    )
{
    if (m_pImpl)
        return m_pImpl->change_password(username, old_password, password);
    else
        return NSS_STATUS_UNAVAIL;
}

uint32_t
LWAuthAdapter::get_user_groups(
    const char *user,
    gid_t **groups,
    int *num_groups
    )
{
    if (m_pImpl)
        return m_pImpl->get_user_groups(user, groups, num_groups);
    else
        return NSS_STATUS_UNAVAIL;
}

void
LWAuthAdapter::free_user_groups(
    gid_t * groups
    )
{
    if (m_pImpl)
        m_pImpl->free_user_groups(groups);
}

