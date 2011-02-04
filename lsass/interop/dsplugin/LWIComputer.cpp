/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
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
    
    macError = LwAllocateMemory(sizeof(LWICOMPUTER), (PVOID*)&pComputer);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (szName)
    {
        macError = LwAllocateString(szName, &pComputer->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szShortname)
    {
        macError = LwAllocateString(szShortname, &pComputer->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
        
    if (szComment)
    {
        macError = LwAllocateString(szComment, &pComputer->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szGUID)
    {
        macError = LwAllocateString(szGUID, &pComputer->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szEthernetID)
    {
        macError = LwAllocateString(szEthernetID, &pComputer->ethernetID);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szIPaddress)
    {
        macError = LwAllocateString(szIPaddress, &pComputer->IPaddress);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szKeyword)
    {
        macError = LwAllocateMemory(sizeof(pComputer->keywords[0]) * 2, (PVOID*)&pComputer->keywords);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LwAllocateString(szKeyword, &pComputer->keywords[0]);
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
        LW_SAFE_FREE_STRING(pLWIComputer->comment);
    if (pLWIComputer->ethernetID)
        LW_SAFE_FREE_STRING(pLWIComputer->ethernetID);
    if (pLWIComputer->guid)
        LW_SAFE_FREE_STRING(pLWIComputer->guid);
    if (pLWIComputer->IPaddress)
        LW_SAFE_FREE_STRING(pLWIComputer->IPaddress);
    if (pLWIComputer->name)
        LW_SAFE_FREE_STRING(pLWIComputer->name);
    if (pLWIComputer->shortname)
        LW_SAFE_FREE_STRING(pLWIComputer->shortname);
    
    if (pLWIComputer->keywords)
    {
        for (int index = 0; pLWIComputer->keywords[index]; index++)
        {
            LW_SAFE_FREE_STRING(pLWIComputer->keywords[index]);
        }
        LwFreeMemory(pLWIComputer->keywords);
    }
    
    if (pLWIComputer->URLs)
    {
        for (int index = 0; pLWIComputer->URLs[index]; index++)
        {
            LW_SAFE_FREE_STRING(pLWIComputer->URLs[index]);
        }
        LwFreeMemory(pLWIComputer->URLs);
    }
	
	if (pLWIComputer->pMCXValues)
	    FreeMCXValueList(pLWIComputer->pMCXValues);
        
    LwFreeMemory(pLWIComputer);
}
