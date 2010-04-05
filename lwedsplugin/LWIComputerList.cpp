/*
 *  LWIComputerList.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIComputerList.h"

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
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTERLIST pComputerList = NULL;
    
    macError = LWAllocateMemory(sizeof(LWICOMPUTERLIST), (PVOID*)&pComputerList);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (szName)
    {
        macError = LWAllocateString(szName, &pComputerList->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szShortname)
    {
        macError = LWAllocateString(szShortname, &pComputerList->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
        
    if (szComment)
    {
        macError = LWAllocateString(szComment, &pComputerList->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szGUID)
    {
        macError = LWAllocateString(szGUID, &pComputerList->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szComputer)
    {
        /* Add the computer to the Computer List's Computers attribute */
        macError = LWAllocateMemory(sizeof(pComputerList->computers[0]) * 2, (PVOID*) &pComputerList->computers);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LWAllocateString(szComputer, &pComputerList->computers[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pComputerList->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pComputerList->primaryId = primaryId;
    
    *ppLWIComputerList = pComputerList;
    pComputerList = NULL;

cleanup:

    FreeLWIComputerList(pComputerList);
    
    return macError;
}

void
FreeLWIComputerList(PLWICOMPUTERLIST pLWIComputerList)
{
    if (pLWIComputerList)
    {
        if (pLWIComputerList->name)
            LWFreeString(pLWIComputerList->name);

        if (pLWIComputerList->shortname)
            LWFreeString(pLWIComputerList->shortname);

        if (pLWIComputerList->comment)
            LWFreeString(pLWIComputerList->comment);

        if (pLWIComputerList->guid)
            LWFreeString(pLWIComputerList->guid);

        if (pLWIComputerList->computers)
        {
            for (int index = 0; pLWIComputerList->computers[index]; index++)
            {
                LWFreeString(pLWIComputerList->computers[index]);
            }
            LWFreeMemory(pLWIComputerList->computers);
        }
	
	    if (pLWIComputerList->pMCXValues)
	        FreeMCXValueList(pLWIComputerList->pMCXValues);
    
        LWFreeMemory(pLWIComputerList);
    }
}
