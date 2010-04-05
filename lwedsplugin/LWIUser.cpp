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
    PCSTR        pszDisplayName,
    PCSTR        pszNFSHomeDirectory,
    PCSTR        pszHomeDirectory,
    PCSTR        pszOrigNFSHomeDirectory,
    PCSTR        pszOrigHomeDirectory,
    PMCXVALUE    pMCXValues,
    PAD_USER_ATTRIBUTES padUserADInfo,
    PLWIUSER *   ppLWIUser
    )
{
    long macError = eDSNoErr;
    PLWIUSER pLWIUser = NULL;

    macError = LWAllocateMemory(sizeof(LWIUSER), (PVOID*)&pLWIUser);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pUser->pw_name && *pUser->pw_name)
    {
        macError = LWAllocateString(pUser->pw_name, &pLWIUser->pw_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszDisplayName)
    {
        macError = LWAllocateString(pszDisplayName, &pLWIUser->pw_display_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_passwd && *pUser->pw_passwd)
    {
        macError = LWAllocateString(pUser->pw_passwd, &pLWIUser->pw_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pLWIUser->pw_uid = pUser->pw_uid;

    pLWIUser->pw_gid = pUser->pw_gid;

    pLWIUser->pw_change = pUser->pw_change;

    if (pUser->pw_class && *pUser->pw_class)
    {
        macError = LWAllocateString(pUser->pw_class, &pLWIUser->pw_class);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_gecos && *pUser->pw_gecos)
    {
        macError = LWAllocateString(pUser->pw_gecos, &pLWIUser->pw_gecos);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (!pszNFSHomeDirectory && pUser->pw_dir && *pUser->pw_dir)
    {
        macError = LWAllocateString(pUser->pw_dir, &pLWIUser->pw_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (pszNFSHomeDirectory)
    {
        macError = LWAllocateString(pszNFSHomeDirectory, &pLWIUser->pw_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszHomeDirectory)
    {
        macError = LWAllocateString(pszHomeDirectory, &pLWIUser->pw_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszOrigNFSHomeDirectory)
    {
        macError = LWAllocateString(pszOrigNFSHomeDirectory, &pLWIUser->pw_orig_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszOrigHomeDirectory)
    {
        macError = LWAllocateString(pszOrigHomeDirectory, &pLWIUser->pw_orig_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_shell && *pUser->pw_shell)
    {
        macError = LWAllocateString(pUser->pw_shell, &pLWIUser->pw_shell);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pLWIUser->pw_expire = pUser->pw_expire;

    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pLWIUser->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (padUserADInfo)
    {
        macError = ADUAdapter_CopyADUserInfo(padUserADInfo, &pLWIUser->padUserInfo);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppLWIUser = pLWIUser;

    return macError;

cleanup:

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

    macError = LWAllocateMemory(sizeof(LWIUSER), (PVOID*)&pCopy);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pUser->pw_name && *pUser->pw_name)
    {
        macError = LWAllocateString(pUser->pw_name, &pCopy->pw_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_display_name && *pUser->pw_display_name)
    {
        macError = LWAllocateString(pUser->pw_display_name, &pCopy->pw_display_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_passwd && *pUser->pw_passwd)
    {
        macError = LWAllocateString(pUser->pw_passwd, &pCopy->pw_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pCopy->pw_uid = pUser->pw_uid;

    pCopy->pw_gid = pUser->pw_gid;

    pCopy->pw_change = pUser->pw_change;

    if (pUser->pw_class && *pUser->pw_class)
    {
        macError = LWAllocateString(pUser->pw_class, &pCopy->pw_class);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_gecos && *pUser->pw_gecos)
    {
        macError = LWAllocateString(pUser->pw_gecos, &pCopy->pw_gecos);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_nfs_home_dir && *pUser->pw_nfs_home_dir)
    {
        macError = LWAllocateString(pUser->pw_nfs_home_dir, &pCopy->pw_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pUser->pw_home_dir && *pUser->pw_home_dir)
    {
        macError = LWAllocateString(pUser->pw_home_dir, &pCopy->pw_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pUser->pw_orig_nfs_home_dir && *pUser->pw_orig_nfs_home_dir)
    {
        macError = LWAllocateString(pUser->pw_orig_nfs_home_dir, &pCopy->pw_orig_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pUser->pw_orig_home_dir && *pUser->pw_orig_home_dir)
    {
        macError = LWAllocateString(pUser->pw_orig_home_dir, &pCopy->pw_orig_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pUser->pw_shell && *pUser->pw_shell)
    {
        macError = LWAllocateString(pUser->pw_shell, &pCopy->pw_shell);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
	
    if (pUser->pMCXValues)
    {
        macError = CopyMCXValueList(pUser->pMCXValues, &pCopy->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pUser->padUserInfo)
    {
        macError = ADUAdapter_CopyADUserInfo(pUser->padUserInfo, &pCopy->padUserInfo);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pCopy->pw_expire = pUser->pw_expire;

    *ppCopyUser = pCopy;

    return macError;

cleanup:

    FreeLWIUser(pCopy);

    return macError;
}

long
CreateLWIUser(
    PCSTR szName,
    PCSTR szDisplayName,
    PCSTR szPassword,
    PCSTR szClass,
    PCSTR szGecos,
    PCSTR szHomedir,
    PCSTR szShell,
    uid_t uid,
    gid_t gid,
    PMCXVALUE pMCXValues,
    PLWIUSER* ppLWIUser
    )
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;
    
    macError = LWAllocateMemory(sizeof(LWIUSER), (PVOID*)&pUser);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (szName)
    {
        macError = LWAllocateString(szName, &pUser->pw_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szDisplayName)
    {
        macError = LWAllocateString(szDisplayName, &pUser->pw_display_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szPassword)
    {
        macError = LWAllocateString(szPassword, &pUser->pw_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szClass)
    {
        macError = LWAllocateString(szClass, &pUser->pw_class);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szGecos)
    {
        macError = LWAllocateString(szGecos, &pUser->pw_gecos);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szHomedir)
    {
        macError = LWAllocateString(szHomedir, &pUser->pw_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szShell)
    {
        macError = LWAllocateString(szShell, &pUser->pw_shell);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pUser->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pUser->pw_uid = uid;
    pUser->pw_gid = gid;
    
    *ppLWIUser = pUser;
    pUser = NULL;

cleanup:

    FreeLWIUser(pUser);
    
    return macError;
}

void
FreeLWIUser(PLWIUSER pLWIUser)
{
    if (pLWIUser)
    {
        if (pLWIUser->pw_name)
            LWFreeString(pLWIUser->pw_name);
        if (pLWIUser->pw_display_name)
            LWFreeString(pLWIUser->pw_display_name);
        if (pLWIUser->pw_passwd)
            LWFreeString(pLWIUser->pw_passwd);
        if (pLWIUser->pw_class)
            LWFreeString(pLWIUser->pw_class);
        if (pLWIUser->pw_gecos)
            LWFreeString(pLWIUser->pw_gecos);
        if (pLWIUser->pw_nfs_home_dir)
            LWFreeString(pLWIUser->pw_nfs_home_dir);
        if (pLWIUser->pw_home_dir)
            LWFreeString(pLWIUser->pw_home_dir);
        if (pLWIUser->pw_orig_home_dir)
            LWFreeString(pLWIUser->pw_orig_home_dir);
        if (pLWIUser->pw_orig_nfs_home_dir)
            LWFreeString(pLWIUser->pw_orig_nfs_home_dir);
        if (pLWIUser->pw_shell)
            LWFreeString(pLWIUser->pw_shell);
	    if (pLWIUser->pMCXValues)
	        FreeMCXValueList(pLWIUser->pMCXValues);
        if (pLWIUser->padUserInfo)
            ADUAdapter_FreeADUserInfo(pLWIUser->padUserInfo);

        LWFreeMemory(pLWIUser);
    }
}
