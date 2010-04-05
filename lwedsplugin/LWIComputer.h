/*
 *  LWIComputer.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWICOMPUTER_H__
#define __LWICOMPUTER_H__

#include "LWIPlugIn.h"

#ifdef __cplusplus
extern "C" {
#endif

long
CreateLWIComputer(
    PCSTR szName,
    PCSTR szShortname,
    PCSTR szComment,
    PCSTR szGUID,
    PCSTR szEthernetID,
    PCSTR szIPaddress,
    PCSTR szKeyword,
	PMCXVALUE pMCXValues,
    PLWICOMPUTER* ppLWIComputer
    );
    
void
FreeLWIComputer(
    PLWICOMPUTER pLWIComputer
    );

#ifdef __cplusplus
}
#endif

#endif /* __LWICOMPUTER_H__ */

