/*
 *  LWIComputer.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIComputer.h"

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
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;
    
    macError = LWAllocateMemory(sizeof(LWICOMPUTER), (PVOID*)&pComputer);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (szName)
    {
        macError = LWAllocateString(szName, &pComputer->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szShortname)
    {
        macError = LWAllocateString(szShortname, &pComputer->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
        
    if (szComment)
    {
        macError = LWAllocateString(szComment, &pComputer->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szGUID)
    {
        macError = LWAllocateString(szGUID, &pComputer->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szEthernetID)
    {
        macError = LWAllocateString(szEthernetID, &pComputer->ethernetID);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szIPaddress)
    {
        macError = LWAllocateString(szIPaddress, &pComputer->IPaddress);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szKeyword)
    {
        macError = LWAllocateMemory(sizeof(pComputer->keywords[0]) * 2, (PVOID*)&pComputer->keywords);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LWAllocateString(szKeyword, &pComputer->keywords[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pComputer->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
	    
    *ppLWIComputer = pComputer;
    pComputer = NULL;

cleanup:

    if (pComputer)
    {
        FreeLWIComputer(pComputer);
    }
    
    return macError;
}

void
FreeLWIComputer(PLWICOMPUTER pLWIComputer)
{
    if (pLWIComputer->comment)
        LWFreeString(pLWIComputer->comment);
    if (pLWIComputer->ethernetID)
        LWFreeString(pLWIComputer->ethernetID);
    if (pLWIComputer->guid)
        LWFreeString(pLWIComputer->guid);
    if (pLWIComputer->IPaddress)
        LWFreeString(pLWIComputer->IPaddress);
    if (pLWIComputer->name)
        LWFreeString(pLWIComputer->name);
    if (pLWIComputer->shortname)
        LWFreeString(pLWIComputer->shortname);
    
    if (pLWIComputer->keywords)
    {
        for (int index = 0; pLWIComputer->keywords[index]; index++)
        {
            LWFreeString(pLWIComputer->keywords[index]);
        }
        LWFreeMemory(pLWIComputer->keywords);
    }
    
    if (pLWIComputer->URLs)
    {
        for (int index = 0; pLWIComputer->URLs[index]; index++)
        {
            LWFreeString(pLWIComputer->URLs[index]);
        }
        LWFreeMemory(pLWIComputer->URLs);
    }
	
	if (pLWIComputer->pMCXValues)
	    FreeMCXValueList(pLWIComputer->pMCXValues);
        
    LWFreeMemory(pLWIComputer);
}
