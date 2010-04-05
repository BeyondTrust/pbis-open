/*
 *  LWIComputerList.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWICOMPUTERLIST_H__
#define __LWICOMPUTERLIST_H__

#include "LWIPlugIn.h"

#ifdef __cplusplus
extern "C" {
#endif

long
CreateLWIComputerList(
    PCSTR szName,
    PCSTR szShortname,
    PCSTR szComment,
    PCSTR szGUID,
    int   primaryId,
    PCSTR szComputer,
	PMCXVALUE pMCXValues,
    PLWICOMPUTERLIST* ppLWIComputerList
    );
    
void
FreeLWIComputerList(
    PLWICOMPUTERLIST pLWIComputerList
    );

#ifdef __cplusplus
}
#endif

#endif /* __LWICOMPUTERLIST_H__ */
