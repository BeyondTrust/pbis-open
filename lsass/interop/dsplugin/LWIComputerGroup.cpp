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

        LwFreeMemory(pLWIComputerGroup);
    }
}
