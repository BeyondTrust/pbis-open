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

#include "LWIGroup.h"

long
CreateLWIGroup(
    PCSTR          pszName,
    PCSTR          pszNameAsQueried,
    PCSTR          pszPassword,
    PCSTR          pszShortname,
    PCSTR          pszComment,
    PLWIMEMBERLIST pMemberList,
    PCSTR          pszGeneratedUID,
    gid_t          gid,
    PMCXVALUE      pMCXValues,
    PLWIGROUP*     ppLWIGroup
    )
{
    long macError = eDSNoErr;
    PLWIGROUP pGroup = NULL;
    
    macError = LwAllocateMemory(sizeof(LWIGROUP), (PVOID*)&pGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);
 
    if (pszName)
    {
        macError = LwAllocateString(pszName, &pGroup->gr_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
 
    if (pszNameAsQueried)
    {
        macError = LwAllocateString(pszNameAsQueried, &pGroup->gr_name_as_queried);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pszPassword)
    {
        macError = LwAllocateString(pszPassword, &pGroup->gr_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pszShortname)
    {
        macError = LwAllocateString(pszShortname, &pGroup->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pszComment)
    {
        macError = LwAllocateString(pszComment, &pGroup->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pMemberList)
    {
        int iCount = 1;
        PLWIMEMBERLIST pCur = pMemberList;
        char szBuf[LWI_GUID_LENGTH+1];

        /* Walk list to determing number of nodes to create array of the same size */
        while(pCur)
        {
            if (pCur->pszUPN || pCur->pszName)
            {
                iCount++;
            }
            pCur = pCur->pNext;
        }

        // iCount is now the size of the list + 1

        macError = LwAllocateMemory(sizeof(pGroup->gr_membership[0]) * iCount, (PVOID*)&pGroup->gr_membership);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LwAllocateMemory(sizeof(pGroup->gr_members[0]) * iCount, (PVOID*)&pGroup->gr_members);
        GOTO_CLEANUP_ON_MACERROR(macError);

        /* Now walk again to fill in the string array */
        iCount = 0;
        pCur = pMemberList;

        while(pCur)
        {
            BOOLEAN fAdded = FALSE;

            /* For improved display on the Mac for the group membership list, we show the user's UPN */
            if (pCur->pszUPN)
            {
                macError = LwAllocateString(pCur->pszUPN, &pGroup->gr_membership[iCount]);
                GOTO_CLEANUP_ON_MACERROR(macError);
                fAdded = TRUE;
            }
            else
            {
                if (pCur->pszName)
                {
                    macError = LwAllocateString(pCur->pszName, &pGroup->gr_membership[iCount]);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    fAdded = TRUE;
                }
            }

            /* The UID needs to be converted to GUID format - same way that LWIQuery does it */
            if (fAdded && pCur->uid != UNSET_GID_UID_ID)
            {
                sprintf(szBuf, LWI_UUID_UID_PREFIX "%.8X", pCur->uid);

                macError = LwAllocateString(szBuf, &pGroup->gr_members[iCount]);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }

            if (fAdded)
            {
                iCount++;
            }

            pCur = pCur->pNext;
        }
    }
    
    if (pszGeneratedUID)
    {
        macError = LwAllocateString(pszGeneratedUID, &pGroup->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pGroup->gr_gid = gid;

    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pGroup->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    *ppLWIGroup = pGroup;
    pGroup = NULL;

cleanup:

    FreeLWIGroup(pGroup);
    
    return macError;
}

void
FreeLWIGroup(PLWIGROUP pLWIGroup)
{
    if (pLWIGroup)
    {
        if (pLWIGroup->gr_name)
            LW_SAFE_FREE_STRING(pLWIGroup->gr_name);

        if (pLWIGroup->gr_name_as_queried)
            LW_SAFE_FREE_STRING(pLWIGroup->gr_name_as_queried);

        if (pLWIGroup->gr_passwd)
            LW_SAFE_FREE_STRING(pLWIGroup->gr_passwd);
        
        if (pLWIGroup->shortname)
            LW_SAFE_FREE_STRING(pLWIGroup->shortname);
        
        if (pLWIGroup->comment)
            LW_SAFE_FREE_STRING(pLWIGroup->comment);

        if (pLWIGroup->gr_members)
        {
            for (int index = 0; pLWIGroup->gr_members[index]; index++)
            {
                LW_SAFE_FREE_STRING(pLWIGroup->gr_members[index]);
            }
            LwFreeMemory(pLWIGroup->gr_members);
        }

        if (pLWIGroup->gr_membership)
        {
            for (int index = 0; pLWIGroup->gr_membership[index]; index++)
            {
                LW_SAFE_FREE_STRING(pLWIGroup->gr_membership[index]);
            }
            LwFreeMemory(pLWIGroup->gr_membership);
        }
        
        if (pLWIGroup->guid)
            LW_SAFE_FREE_STRING(pLWIGroup->guid);
	
	    if (pLWIGroup->pMCXValues)
	        FreeMCXValueList(pLWIGroup->pMCXValues);

        LwFreeMemory(pLWIGroup);
    }
}
