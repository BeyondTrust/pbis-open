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
    
    macError = LwAllocateMemory(sizeof(LWICOMPUTERGROUP), (PVOID*)&pComputerGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (szName)
    {
        macError = LwAllocateString(szName, &pComputerGroup->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szShortname)
    {
        macError = LwAllocateString(szShortname, &pComputerGroup->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
        
    if (szComment)
    {
        macError = LwAllocateString(szComment, &pComputerGroup->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szGUID)
    {
        macError = LwAllocateString(szGUID, &pComputerGroup->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szComputer)
    {
        /* Add the computer to the Computer Group's Computers attribute */
        macError = LwAllocateMemory(sizeof(pComputerGroup->computers[0]) * 2, (PVOID*) &pComputerGroup->computers);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LwAllocateString(szComputer, &pComputerGroup->computers[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        /* Also add the computer to the Computer Group's Membership attribute */
        macError = LwAllocateMemory(sizeof(pComputerGroup->membership[0]) * 2, (PVOID*) &pComputerGroup->membership);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LwAllocateString(szComputer, &pComputerGroup->membership[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szComputerGUID)
    {
        /* Add the computer identifier to the Computer Group's Members attribute */
        macError = LwAllocateMemory(sizeof(pComputerGroup->members[0]) * 2, (PVOID*) &pComputerGroup->members);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LwAllocateString(szComputerGUID, &pComputerGroup->members[0]);
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
            LW_SAFE_FREE_STRING(pLWIComputerGroup->name);
    
        if (pLWIComputerGroup->shortname)
            LW_SAFE_FREE_STRING(pLWIComputerGroup->shortname);
        
        if (pLWIComputerGroup->comment)
            LW_SAFE_FREE_STRING(pLWIComputerGroup->comment);
    
        if (pLWIComputerGroup->guid)
            LW_SAFE_FREE_STRING(pLWIComputerGroup->guid);
        
        if (pLWIComputerGroup->computers)
        {
            for (int index = 0; pLWIComputerGroup->computers[index]; index++)
            {
                LW_SAFE_FREE_STRING(pLWIComputerGroup->computers[index]);
            }
            LwFreeMemory(pLWIComputerGroup->computers);
        }

        if (pLWIComputerGroup->members)
        {
            for (int index = 0; pLWIComputerGroup->members[index]; index++)
            {
                LW_SAFE_FREE_STRING(pLWIComputerGroup->members[index]);
            }
            LwFreeMemory(pLWIComputerGroup->members);
        }

        if (pLWIComputerGroup->membership)
        {
            for (int index = 0; pLWIComputerGroup->membership[index]; index++)
            {
                LW_SAFE_FREE_STRING(pLWIComputerGroup->membership[index]);
            }
            LwFreeMemory(pLWIComputerGroup->membership);
        }
	
	    if (pLWIComputerGroup->pMCXValues)
	        FreeMCXValueList(pLWIComputerGroup->pMCXValues);

        LwFreeMemory(pLWIComputerGroup);
    }
}
