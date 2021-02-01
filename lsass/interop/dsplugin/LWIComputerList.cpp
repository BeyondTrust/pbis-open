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

#include "LWIComputerList.h"

long
CreateLWIComputerList(
    PCSTR szName,
    PCSTR szShortname,
    PCSTR szComment,
    PCSTR szGUID,
    int   primaryId,
    PCSTR szComputer,
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


        LwFreeMemory(pLWIComputerList);
    }
}
