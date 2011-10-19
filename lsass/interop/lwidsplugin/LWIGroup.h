/*
 *  LWIGroup.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWIGROUP_H__
#define __LWIGROUP_H__

#include "LWIPlugIn.h"

#ifdef __cplusplus
extern "C" {
#endif

    long
    BuildLWIGroup(
        const struct group* pGroup,
        PLWIGROUP * ppLWIGroup
        );

    long
    CloneLWIGroup(
        const PLWIGROUP pGroup,
        PLWIGROUP * ppCopyGroup
        );

    void
    FreeLWIGroup(PLWIGROUP pLWIGroup);

#ifdef __cplusplus
}
#endif

#endif /* __LWIGROUP_H__ */
