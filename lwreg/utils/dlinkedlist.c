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
 *        BeyondTrust Lwistry
 *
 *        Doubly linked list
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
RegDLinkedListPrepend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pList = NULL;

    dwError = RegAllocateMemory(sizeof(*pList), (PVOID*)&pList);
    BAIL_ON_REG_ERROR(dwError);

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
        RegMemoryFree(pList);
    }

    goto cleanup;
}

DWORD
RegDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pList = NULL;

    dwError = RegAllocateMemory(sizeof(*pList), (PVOID*)&pList);
    BAIL_ON_REG_ERROR(dwError);

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
        RegMemoryFree(pList);
    }

    goto cleanup;
}

BOOLEAN
RegDLinkedListDelete(
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
       RegMemoryFree(pCandidate);
    }

    return bFound;
}

VOID
RegDLinkedListForEach(
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
RegDLinkedListFree(
    PDLINKEDLIST pList
    )
{
    while (pList)
    {
        PDLINKEDLIST pTmp = pList;
        pList = pList->pNext;
        RegMemoryFree(pTmp);
    }
}
