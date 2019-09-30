/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        dlinkedlist.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 * 
 *        Doubly linked list
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"


static
VOID
LwDLinkedListNodeCounter(
    PVOID pData,
    PVOID pUserData
    );


DWORD
LwDLinkedListPrepend(
    PLW_DLINKED_LIST* ppList,
    PVOID        pItem
    )
{
    DWORD dwError = 0;
    PLW_DLINKED_LIST pList = NULL;
    
    dwError = LwAllocateMemory(sizeof(LW_DLINKED_LIST), (PVOID*)&pList);
    BAIL_ON_LW_ERROR(dwError);
    
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
        LwFreeMemory(pList);
    }

    goto cleanup;
}

DWORD
LwDLinkedListAppend(
    PLW_DLINKED_LIST* ppList,
    PVOID        pItem
    )
{
    DWORD dwError = 0;
    PLW_DLINKED_LIST pList = NULL;
    
    dwError = LwAllocateMemory(sizeof(LW_DLINKED_LIST), (PVOID*)&pList);
    BAIL_ON_LW_ERROR(dwError);
    
    pList->pItem = pItem;
    
    if (*ppList) {
       PLW_DLINKED_LIST pLast = NULL;
       PLW_DLINKED_LIST pCur = *ppList;
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
        LwFreeMemory(pList);
    }

    goto cleanup;    
}

BOOLEAN
LwDLinkedListDelete(
    PLW_DLINKED_LIST* ppList,
    PVOID        pItem
    )
{
    BOOLEAN bFound = FALSE;
    PLW_DLINKED_LIST pList = (ppList ? *ppList : NULL);
    PLW_DLINKED_LIST pCandidate = NULL;
    
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
       LwFreeMemory(pCandidate);
    }
    
    return bFound;
}

VOID
LwDLinkedListForEach(
    PLW_DLINKED_LIST          pList,
    PFN_LW_DLINKED_LIST_FUNC pFunc,
    PVOID                pUserData
    )
{
    while (pList) {
        pFunc(pList->pItem, pUserData);
        pList = pList->pNext;
    }
}

DWORD
LwDLinkedListLength(
    PLW_DLINKED_LIST pList
    )
{
    DWORD dwCount = 0;
    LwDLinkedListForEach(pList,
                          LwDLinkedListNodeCounter,
                          &dwCount);

    return dwCount;
}


VOID
LwDLinkedListFree(
    PLW_DLINKED_LIST pList
    )
{
    while (pList)
    {
        PLW_DLINKED_LIST pTmp = pList;
        pList = pList->pNext;
        LwFreeMemory(pTmp);
    }
}


static
VOID
LwDLinkedListNodeCounter(
    PVOID pData,
    PVOID pUserData
    )
{
    PDWORD pdwCount = (PDWORD)pUserData;
    (*pdwCount)++;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

