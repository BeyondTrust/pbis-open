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

#include "LWIComputer.h"

long
CreateLWIComputer(
    PCSTR szName,
    PCSTR szShortname,
    PCSTR szComment,
    PCSTR szGUID,
    PCSTR szEthernetID,
    PCSTR szIPaddress,
    PCSTR szKeyword,
    PLWICOMPUTER* ppLWIComputer
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;

    macError = LwAllocateMemory(sizeof(LWICOMPUTER), (PVOID*)&pComputer);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (szName)
    {
        macError = LwAllocateString(szName, &pComputer->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szShortname)
    {
        macError = LwAllocateString(szShortname, &pComputer->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szComment)
    {
        macError = LwAllocateString(szComment, &pComputer->comment);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szGUID)
    {
        macError = LwAllocateString(szGUID, &pComputer->guid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szEthernetID)
    {
        macError = LwAllocateString(szEthernetID, &pComputer->ethernetID);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szIPaddress)
    {
        macError = LwAllocateString(szIPaddress, &pComputer->IPaddress);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (szKeyword)
    {
        macError = LwAllocateMemory(sizeof(pComputer->keywords[0]) * 2, (PVOID*)&pComputer->keywords);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = LwAllocateString(szKeyword, &pComputer->keywords[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppLWIComputer = pComputer;
    pComputer = NULL;

cleanup:

    if (pComputer)
    {
        FreeLWIComputer(pComputer);
    }

    return macError;
}

void
FreeLWIComputer(PLWICOMPUTER pLWIComputer)
{
    if (pLWIComputer->comment)
        LW_SAFE_FREE_STRING(pLWIComputer->comment);
    if (pLWIComputer->ethernetID)
        LW_SAFE_FREE_STRING(pLWIComputer->ethernetID);
    if (pLWIComputer->guid)
        LW_SAFE_FREE_STRING(pLWIComputer->guid);
    if (pLWIComputer->IPaddress)
        LW_SAFE_FREE_STRING(pLWIComputer->IPaddress);
    if (pLWIComputer->name)
        LW_SAFE_FREE_STRING(pLWIComputer->name);
    if (pLWIComputer->shortname)
        LW_SAFE_FREE_STRING(pLWIComputer->shortname);

    if (pLWIComputer->keywords)
    {
        for (int index = 0; pLWIComputer->keywords[index]; index++)
        {
            LW_SAFE_FREE_STRING(pLWIComputer->keywords[index]);
        }
        LwFreeMemory(pLWIComputer->keywords);
    }

    if (pLWIComputer->URLs)
    {
        for (int index = 0; pLWIComputer->URLs[index]; index++)
        {
            LW_SAFE_FREE_STRING(pLWIComputer->URLs[index]);
        }
        LwFreeMemory(pLWIComputer->URLs);
    }

    LwFreeMemory(pLWIComputer);
}
