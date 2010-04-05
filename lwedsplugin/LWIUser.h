/*
 *  LWIUser.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIUSER_H__
#define __LWIUSER_H__

#include "LWIPlugIn.h"

#ifdef __cplusplus
extern "C" {
#endif

long
BuildLWIUser(
    const struct passwd* pUser,
    PCSTR        pszDisplayName,
    PCSTR        pszNFSHomeDirectory,
    PCSTR        pszHomeDirectory,
    PCSTR        pszOrigNFSHomeDirectory,
    PCSTR        pszOrigHomeDirectory,
    PMCXVALUE    pMCXValues,
    PAD_USER_ATTRIBUTES padUserInfo,
    PLWIUSER *   ppLWIUser
    );

long
CloneLWIUser(
    const PLWIUSER pUser,
    PLWIUSER * ppCopyUser
    );

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
    );
    
void
FreeLWIUser(
    PLWIUSER pLWIUser
    );

#ifdef __cplusplus
}
#endif

#endif /* __LWIUSER_H__ */

