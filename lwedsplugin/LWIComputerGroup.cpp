/*
 *  LWIComputerGroup.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIComputerGroup.h"

long
CreateLWIComputerGroup(
    PCSTR szName,
    PCSTR szShortname,
    PCSTR szComment,
    PCSTR szGUID,
    int   primaryId,
    PCSTR szComputer,
    PCSTR szComputerGUID,
    PMCXVALUE pMCXValues,
    PLWICOMPUTERGROUP* ppLWIComputerGroup
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTERGROUP pComputerGroup = NULL;
    
    macError = LWAllocateMemory(sizeof(LWICOMPUTERGROUP), (PVOID*)&pComputerGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (szName)
    {
        macError = LWAllocateString(szName, &pComputerGroup->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szShortname)
    {
        macError = LWAllocateString(szShortname, &pComputerGroup->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
        
    if (szComment)
    {
        macError = LWAllocateString(szComment, &pComputerGroup->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szGUID)
    {
        macError = LWAllocateString(szGUID, &pComputerGroup->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szComputer)
    {
        /* Add the computer to the Computer Group's Computers attribute */
        macError = LWAllocateMemory(sizeof(pComputerGroup->computers[0]) * 2, (PVOID*) &pComputerGroup->computers);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LWAllocateString(szComputer, &pComputerGroup->computers[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        /* Also add the computer to the Computer Group's Membership attribute */
        macError = LWAllocateMemory(sizeof(pComputerGroup->membership[0]) * 2, (PVOID*) &pComputerGroup->membership);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LWAllocateString(szComputer, &pComputerGroup->membership[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szComputerGUID)
    {
        /* Add the computer identifier to the Computer Group's Members attribute */
        macError = LWAllocateMemory(sizeof(pComputerGroup->members[0]) * 2, (PVOID*) &pComputerGroup->members);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LWAllocateString(szComputerGUID, &pComputerGroup->members[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pComputerGroup->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
	    
    pComputerGroup->primaryId = primaryId;
    
    *ppLWIComputerGroup = pComputerGroup;
    pComputerGroup = NULL;

cleanup:

    FreeLWIComputerGroup(pComputerGroup);
    
    return macError;
}

void
FreeLWIComputerGroup(PLWICOMPUTERGROUP pLWIComputerGroup)
{
    if (pLWIComputerGroup)
    {
        if (pLWIComputerGroup->name)
            LWFreeString(pLWIComputerGroup->name);
    
        if (pLWIComputerGroup->shortname)
            LWFreeString(pLWIComputerGroup->shortname);
        
        if (pLWIComputerGroup->comment)
            LWFreeString(pLWIComputerGroup->comment);
    
        if (pLWIComputerGroup->guid)
            LWFreeString(pLWIComputerGroup->guid);
        
        if (pLWIComputerGroup->computers)
        {
            for (int index = 0; pLWIComputerGroup->computers[index]; index++)
            {
                LWFreeString(pLWIComputerGroup->computers[index]);
            }
            LWFreeMemory(pLWIComputerGroup->computers);
        }

        if (pLWIComputerGroup->members)
        {
            for (int index = 0; pLWIComputerGroup->members[index]; index++)
            {
                LWFreeString(pLWIComputerGroup->members[index]);
            }
            LWFreeMemory(pLWIComputerGroup->members);
        }

        if (pLWIComputerGroup->membership)
        {
            for (int index = 0; pLWIComputerGroup->membership[index]; index++)
            {
                LWFreeString(pLWIComputerGroup->membership[index]);
            }
            LWFreeMemory(pLWIComputerGroup->membership);
        }
	
	    if (pLWIComputerGroup->pMCXValues)
	        FreeMCXValueList(pLWIComputerGroup->pMCXValues);

        LWFreeMemory(pLWIComputerGroup);
    }
}
