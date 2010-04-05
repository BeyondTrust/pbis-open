/*
 *  LWIComputerGroup.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWICOMPUTERGROUP_H__
#define __LWICOMPUTERGROUP_H__

#include "LWIPlugIn.h"

#ifdef __cplusplus
extern "C" {
#endif

long
CreateLWIComputerGroup(
    PCSTR szName,
    PCSTR szShortname,
    PCSTR szComment,
    PCSTR szGUID,
    int  primaryId,
    PCSTR szComputer,
    PCSTR szComputerGUID,
	PMCXVALUE pMCXValues,
    PLWICOMPUTERGROUP* ppLWIComputerGroup
    );
    
void
FreeLWIComputerGroup(
    PLWICOMPUTERGROUP pLWIComputerGroup
    );

#ifdef __cplusplus
}
#endif

#endif /* __LWICOMPUTERGROUP_H__ */
