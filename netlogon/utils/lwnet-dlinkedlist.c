/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-cfg.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#include "includes.h"

DWORD
LWNetDLinkedListPrepend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pList = NULL;
    
    dwError = LWNetAllocateMemory(sizeof(DLINKEDLIST), (PVOID*)&pList);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pList->pItem = pItem;
    
    if (*ppList) {
       (*ppList)->pPrev = pList;
       pList->pNext = *ppList;
       *ppList = pList;
    } else {
       *ppList = pList;
    }
    
cleanup:

    return dwError;
    
error:

    if (pList) {
        LWNetFreeMemory(pList);
    }

    goto cleanup;
}

DWORD
LWNetDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pList = NULL;
    
    dwError = LWNetAllocateMemory(sizeof(DLINKEDLIST), (PVOID*)&pList);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pList->pItem = pItem;
    
    if (*ppList) {
       PDLINKEDLIST pLast = NULL;
       PDLINKEDLIST pCur = *ppList;
       while (pCur) {
             pLast = pCur;
             pCur = pCur->pNext;
       } 
       pLast->pNext = pList;
       pList->pPrev = pLast;
    } else {
       *ppList = pList;
    }
    
cleanup:

    return dwError;
    
error:

    if (pList) {
        LWNetFreeMemory(pList);
    }

    goto cleanup;    
}

BOOLEAN
LWNetDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    )
{
    BOOLEAN bFound = FALSE;
    PDLINKEDLIST pList = (ppList ? *ppList : NULL);
    PDLINKEDLIST pCandidate = NULL;
    
    while (pList)
    {
        if (pList->pItem == pItem)
        {
            pCandidate = pList;
            bFound = TRUE;
            break;
        }
        pList = pList->pNext;
    }
    
    if (bFound) {
       if (pCandidate->pNext) {
          // Connect the next neighbor to our previous neighbor
          pCandidate->pNext->pPrev = pCandidate->pPrev;
       }
       if (pCandidate->pPrev) {
          // Connect the previous neighbor to our next neighbor
          pCandidate->pPrev->pNext = pCandidate->pNext;
       }
       if (*ppList == pCandidate) {
          *ppList = pCandidate->pNext;
       }
       pCandidate->pItem = NULL;
       LWNetFreeMemory(pCandidate);
    }
    
    return bFound;
}

VOID
LWNetDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    )
{
    while (pList) {
        pFunc(pList->pItem, pUserData);
        pList = pList->pNext;
    }
}

VOID
LWNetDLinkedListFree(
    PDLINKEDLIST pList
    )
{
    while (pList)
    {
        PDLINKEDLIST pTmp = pList;
        pList = pList->pNext;
        LWNetFreeMemory(pTmp);
    }
}
