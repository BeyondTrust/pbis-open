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
        PLWIUSER * ppLWIUser
        );

    long
    CloneLWIUser(
        const PLWIUSER pUser,
        PLWIUSER * ppCopyUser
        );

    void
    FreeLWIUser(PLWIUSER pLWIUser);

#ifdef __cplusplus
}
#endif

#endif /* __LWIUSER_H__ */

