/*
 *  LWIUser.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIUser.h"

long
BuildLWIUser(
    const struct passwd* pUser,
    PLWIUSER * ppLWIUser
    )
{
    long macError = eDSNoErr;
    PLWIUSER pLWIUser = NULL;

    macError = LWIAllocateMemory(sizeof(LWIUSER), (PVOID*)&pLWIUser);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pUser->pw_name && *pUser->pw_name)
    {
        macError = LWIAllocateString(pUser->pw_name, &pLWIUser->pw_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_passwd && *pUser->pw_passwd)
    {
        macError = LWIAllocateString(pUser->pw_passwd, &pLWIUser->pw_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pLWIUser->pw_uid = pUser->pw_uid;

    pLWIUser->pw_gid = pUser->pw_gid;

    pLWIUser->pw_change = pUser->pw_change;

    if (pUser->pw_class && *pUser->pw_class)
    {
        macError = LWIAllocateString(pUser->pw_class, &pLWIUser->pw_class);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_gecos && *pUser->pw_gecos)
    {
        macError = LWIAllocateString(pUser->pw_gecos, &pLWIUser->pw_gecos);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_dir && *pUser->pw_dir)
    {
        macError = LWIAllocateString(pUser->pw_dir, &pLWIUser->pw_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_shell && *pUser->pw_shell)
    {
        macError = LWIAllocateString(pUser->pw_shell, &pLWIUser->pw_shell);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pLWIUser->pw_expire = pUser->pw_expire;

    *ppLWIUser = pLWIUser;

    return macError;

cleanup:

    if (pLWIUser)
        FreeLWIUser(pLWIUser);

    return macError;
}

long
CloneLWIUser(
    const PLWIUSER pUser,
    PLWIUSER * ppCopyUser
    )
{
    long macError = eDSNoErr;
    PLWIUSER pCopy = NULL;

    macError = LWIAllocateMemory(sizeof(LWIUSER), (PVOID*)&pCopy);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pUser->pw_name && *pUser->pw_name)
    {
        macError = LWIAllocateString(pUser->pw_name, &pCopy->pw_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_passwd && *pUser->pw_passwd)
    {
        macError = LWIAllocateString(pUser->pw_passwd, &pCopy->pw_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pCopy->pw_uid = pUser->pw_uid;

    pCopy->pw_gid = pUser->pw_gid;

    pCopy->pw_change = pUser->pw_change;

    if (pUser->pw_class && *pUser->pw_class)
    {
        macError = LWIAllocateString(pUser->pw_class, &pCopy->pw_class);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_gecos && *pUser->pw_gecos)
    {
        macError = LWIAllocateString(pUser->pw_gecos, &pCopy->pw_gecos);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_dir && *pUser->pw_dir)
    {
        macError = LWIAllocateString(pUser->pw_dir, &pCopy->pw_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_shell && *pUser->pw_shell)
    {
        macError = LWIAllocateString(pUser->pw_shell, &pCopy->pw_shell);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pCopy->pw_expire = pUser->pw_expire;

    *ppCopyUser = pCopy;

    return macError;

cleanup:

    if (pCopy)
        FreeLWIUser(pCopy);

    return macError;
}

void
FreeLWIUser(PLWIUSER pLWIUser)
{
    if (pLWIUser->pw_name)
        LWIFreeString(pLWIUser->pw_name);
    if (pLWIUser->pw_passwd)
        LWIFreeString(pLWIUser->pw_passwd);
    if (pLWIUser->pw_class)
        LWIFreeString(pLWIUser->pw_class);
    if (pLWIUser->pw_gecos)
        LWIFreeString(pLWIUser->pw_gecos);
    if (pLWIUser->pw_dir)
        LWIFreeString(pLWIUser->pw_dir);
    if (pLWIUser->pw_shell)
        LWIFreeString(pLWIUser->pw_shell);
    LWIFreeMemory(pLWIUser);
}
