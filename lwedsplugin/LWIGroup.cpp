/*
 *  LWIGroup.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/31/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "LWIGroup.h"

long
BuildLWIGroup(
    const struct group* pGroup,
    PMCXVALUE    pMCXValues,
    PLWIGROUP *  ppLWIGroup
    )
{
    long macError = eDSNoErr;
    PLWIGROUP pLWIGroup = NULL;
    int nUsers = 0;
    int iUser = 0;

    macError = LWAllocateMemory(sizeof(LWIGROUP), (PVOID*)&pLWIGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pGroup->gr_name && *pGroup->gr_name)
    {
        macError = LWAllocateString(pGroup->gr_name, &pLWIGroup->gr_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGroup->gr_passwd && *pGroup->gr_passwd)
    {
        macError = LWAllocateString(pGroup->gr_passwd, &pLWIGroup->gr_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pLWIGroup->gr_gid = pGroup->gr_gid;

    while (pGroup->gr_mem && pGroup->gr_mem[nUsers] && *pGroup->gr_mem[nUsers])
        nUsers++;

    if (nUsers)
    {
        macError = LWAllocateMemory(sizeof(pLWIGroup->gr_membership[0]) * (nUsers+1), (PVOID*)&pLWIGroup->gr_membership);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (iUser = 0; iUser < nUsers; iUser++)
        {
            macError = LWAllocateString(pGroup->gr_mem[iUser], &pLWIGroup->gr_membership[iUser]);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pLWIGroup->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppLWIGroup = pLWIGroup;


    return macError;

cleanup:

    return macError;
}

long
CloneLWIGroup(
    const PLWIGROUP pGroup,
    PLWIGROUP * ppCopyGroup
    )
{
    long macError = eDSNoErr;
    PLWIGROUP pCopy = NULL;
    int nUsers = 0;
    int iUser = 0;

    macError = LWAllocateMemory(sizeof(LWIGROUP), (PVOID*)&pCopy);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pGroup->gr_name && *pGroup->gr_name)
    {
        macError = LWAllocateString(pGroup->gr_name, &pCopy->gr_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pGroup->shortname && *pGroup->shortname)
    {
        macError = LWAllocateString(pGroup->shortname, &pGroup->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGroup->gr_passwd && *pGroup->gr_passwd)
    {
        macError = LWAllocateString(pGroup->gr_passwd, &pCopy->gr_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGroup->guid && *pGroup->guid)
    {
        macError = LWAllocateString(pGroup->guid, &pCopy->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pCopy->gr_gid = pGroup->gr_gid;

    nUsers = 0;
    while (pGroup->gr_membership && pGroup->gr_membership[nUsers] && *pGroup->gr_membership[nUsers])
        nUsers++;

    if (nUsers)
    {
        macError = LWAllocateMemory(sizeof(pCopy->gr_membership[0]) * (nUsers+1), (PVOID*)&pCopy->gr_membership);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (iUser = 0; iUser < nUsers; iUser++)
        {
            macError = LWAllocateString(pGroup->gr_membership[iUser], &pCopy->gr_membership[iUser]);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

    nUsers = 0;
    while (pGroup->gr_members && pGroup->gr_members[nUsers] && *pGroup->gr_members[nUsers])
        nUsers++;

    if (nUsers)
    {
        macError = LWAllocateMemory(sizeof(pCopy->gr_members[0]) * (nUsers+1), (PVOID*)&pCopy->gr_members);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (iUser = 0; iUser < nUsers; iUser++)
        {
            macError = LWAllocateString(pGroup->gr_members[iUser], &pCopy->gr_members[iUser]);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
	
    if (pGroup->pMCXValues)
    {
        macError = CopyMCXValueList(pGroup->pMCXValues, &pCopy->pMCXValues);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppCopyGroup = pCopy;

    return macError;

cleanup:

    FreeLWIGroup(pCopy);

    return macError;
}

long
CreateLWIGroup(
    PCSTR          pszName,
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
    
    macError = LWAllocateMemory(sizeof(LWIGROUP), (PVOID*)&pGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (pszName)
    {
        macError = LWAllocateString(pszName, &pGroup->gr_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pszPassword)
    {
        macError = LWAllocateString(pszPassword, &pGroup->gr_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pszShortname)
    {
        macError = LWAllocateString(pszShortname, &pGroup->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (pszComment)
    {
        macError = LWAllocateString(pszComment, &pGroup->comment);
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
            iCount++;
            pCur = pCur->pNext;
        }

        // iCount is now the size of the list + 1

        macError = LWAllocateMemory(sizeof(pGroup->gr_membership[0]) * iCount, (PVOID*)&pGroup->gr_membership);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LWAllocateMemory(sizeof(pGroup->gr_members[0]) * iCount, (PVOID*)&pGroup->gr_members);
        GOTO_CLEANUP_ON_MACERROR(macError);

        /* Now walk again to fill in the string array */
        iCount = 0;
        pCur = pMemberList;

        while(pCur)
        {
            /* For improved display on the Mac for the group membership list, we show the user's UPN */
            macError = LWAllocateString(pCur->pszUPN, &pGroup->gr_membership[iCount]);
            GOTO_CLEANUP_ON_MACERROR(macError);

            /* The UID needs to be converted to GUID format - same way that LWIQuery does it */
            sprintf(szBuf, LWI_UUID_UID_PREFIX "%.8X", pCur->uid);

            macError = LWAllocateString(szBuf, &pGroup->gr_members[iCount]);
            GOTO_CLEANUP_ON_MACERROR(macError);

            iCount++;
            pCur = pCur->pNext;
        }
    }
    
    if (pszGeneratedUID)
    {
        macError = LWAllocateString(pszGeneratedUID, &pGroup->guid);
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
            LWFreeString(pLWIGroup->gr_name);

        if (pLWIGroup->gr_passwd)
            LWFreeString(pLWIGroup->gr_passwd);
        
        if (pLWIGroup->shortname)
            LWFreeString(pLWIGroup->shortname);
        
        if (pLWIGroup->comment)
            LWFreeString(pLWIGroup->comment);

        if (pLWIGroup->gr_members)
        {
            for (int index = 0; pLWIGroup->gr_members[index]; index++)
            {
                LWFreeString(pLWIGroup->gr_members[index]);
            }
            LWFreeMemory(pLWIGroup->gr_members);
        }

        if (pLWIGroup->gr_membership)
        {
            for (int index = 0; pLWIGroup->gr_membership[index]; index++)
            {
                LWFreeString(pLWIGroup->gr_membership[index]);
            }
            LWFreeMemory(pLWIGroup->gr_membership);
        }
        
        if (pLWIGroup->guid)
            LWFreeString(pLWIGroup->guid);
	
	    if (pLWIGroup->pMCXValues)
	        FreeMCXValueList(pLWIGroup->pMCXValues);

        LWFreeMemory(pLWIGroup);
    }
}
