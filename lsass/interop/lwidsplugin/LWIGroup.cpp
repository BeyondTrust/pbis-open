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
    PLWIGROUP * ppLWIGroup
    )
{
    long macError = eDSNoErr;
    PLWIGROUP pLWIGroup = NULL;
    int nUsers = 0;
    int iUser = 0;

    macError = LWIAllocateMemory(sizeof(LWIGROUP), (PVOID*)&pLWIGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pGroup->gr_name && *pGroup->gr_name)
    {
        macError = LWIAllocateString(pGroup->gr_name, &pLWIGroup->gr_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGroup->gr_passwd && *pGroup->gr_passwd)
    {
        macError = LWIAllocateString(pGroup->gr_passwd, &pLWIGroup->gr_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pLWIGroup->gr_gid = pGroup->gr_gid;

    while (pGroup->gr_mem && pGroup->gr_mem[nUsers] && *pGroup->gr_mem[nUsers])
        nUsers++;

    if (nUsers)
    {
        macError = LWIAllocateMemory(sizeof(pLWIGroup->gr_mem[0]) * (nUsers+1), (PVOID*)&pLWIGroup->gr_mem);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (iUser = 0; iUser < nUsers; iUser++)
        {
            macError = LWIAllocateString(pGroup->gr_mem[iUser], &pLWIGroup->gr_mem[iUser]);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
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

    macError = LWIAllocateMemory(sizeof(LWIGROUP), (PVOID*)&pCopy);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pGroup->gr_name && *pGroup->gr_name)
    {
        macError = LWIAllocateString(pGroup->gr_name, &pCopy->gr_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pGroup->gr_passwd && *pGroup->gr_passwd)
    {
        macError = LWIAllocateString(pGroup->gr_passwd, &pCopy->gr_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pCopy->gr_gid = pGroup->gr_gid;

    while (pGroup->gr_mem && pGroup->gr_mem[nUsers] && *pGroup->gr_mem[nUsers])
        nUsers++;

    if (nUsers)
    {
        macError = LWIAllocateMemory(sizeof(pCopy->gr_mem[0]) * (nUsers+1), (PVOID*)&pCopy->gr_mem);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (iUser = 0; iUser < nUsers; iUser++)
        {
            macError = LWIAllocateString(pGroup->gr_mem[iUser], &pCopy->gr_mem[iUser]);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

    *ppCopyGroup = pCopy;

    return macError;

cleanup:

    if (pCopy)
        FreeLWIGroup(pCopy);

    return macError;
}

void
FreeLWIGroup(PLWIGROUP pLWIGroup)
{
    if (pLWIGroup->gr_name)
        LWIFreeString(pLWIGroup->gr_name);

    if (pLWIGroup->gr_passwd)
        LWIFreeString(pLWIGroup->gr_passwd);

    if (pLWIGroup->gr_mem)
    {
        for (int index = 0; pLWIGroup->gr_mem[index]; index++)
        {
            LWIFreeString(pLWIGroup->gr_mem[index]);
        }
        LWIFreeMemory(pLWIGroup->gr_mem);
    }

    LWIFreeMemory(pLWIGroup);
}
