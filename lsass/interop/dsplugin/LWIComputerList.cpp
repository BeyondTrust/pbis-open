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
    
    macError = LwAllocateMemory(sizeof(LWICOMPUTERLIST), (PVOID*)&pComputerList);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (szName)
    {
        macError = LwAllocateString(szName, &pComputerList->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szShortname)
    {
        macError = LwAllocateString(szShortname, &pComputerList->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
        
    if (szComment)
    {
        macError = LwAllocateString(szComment, &pComputerList->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (szGUID)
    {
        macError = LwAllocateString(szGUID, &pComputerList->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szComputer)
    {
        /* Add the computer to the Computer List's Computers attribute */
        macError = LwAllocateMemory(sizeof(pComputerList->computers[0]) * 2, (PVOID*) &pComputerList->computers);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = LwAllocateString(szComputer, &pComputerList->computers[0]);
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
            LW_SAFE_FREE_STRING(pLWIComputerList->name);

        if (pLWIComputerList->shortname)
            LW_SAFE_FREE_STRING(pLWIComputerList->shortname);

        if (pLWIComputerList->comment)
            LW_SAFE_FREE_STRING(pLWIComputerList->comment);

        if (pLWIComputerList->guid)
            LW_SAFE_FREE_STRING(pLWIComputerList->guid);

        if (pLWIComputerList->computers)
        {
            for (int index = 0; pLWIComputerList->computers[index]; index++)
            {
                LW_SAFE_FREE_STRING(pLWIComputerList->computers[index]);
            }
            LwFreeMemory(pLWIComputerList->computers);
        }
	
	    if (pLWIComputerList->pMCXValues)
	        FreeMCXValueList(pLWIComputerList->pMCXValues);
    
        LwFreeMemory(pLWIComputerList);
    }
}
