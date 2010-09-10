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
CreateLWIUser(
    PCSTR pszName,
    PCSTR pszDisplayName,
    PCSTR pszPassword,
    PCSTR pszClass,
    PCSTR pszGecos,
    PCSTR pszNFSHomeDirectory,
    PCSTR pszHomeDirectory,
    PCSTR pszOrigNFSHomeDirectory,
    PCSTR pszOrigHomeDirectory,
    PCSTR pszShell,
    uid_t uid,
    gid_t gid,
    PAD_USER_ATTRIBUTES padUserADInfo,
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

