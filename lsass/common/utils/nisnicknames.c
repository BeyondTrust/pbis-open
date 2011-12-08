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
 *        nisnicknames.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        NIS Nicknames
 *
 * Authors: Manny Vellon (mvellon@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
VOID
LsaNISFreeNicknameInList(
    PVOID pItem,
    PVOID pUserData
    );

static
VOID
LsaNISFreeNickname(
    PLSA_NIS_NICKNAME pNickname
    );

DWORD
LsaNISGetNicknames(
    PCSTR         pszNicknameFilePath,
    PLW_DLINKED_LIST* ppNicknameList
    )
{
    typedef enum
    {
        NIS_NICKNAME_ALIAS = 0,
        NIS_NICKNAME_NAME
    } NISNicknameTokenType;
    DWORD dwError = 0;
    PLW_DLINKED_LIST pNicknameList = NULL;
    BOOLEAN bFileExists = FALSE;
    PLSA_NIS_NICKNAME pNickname = NULL;
    FILE* fp = NULL;
    NISNicknameTokenType nextTokenType = NIS_NICKNAME_ALIAS;

    dwError = LsaCheckFileExists(
                    pszNicknameFilePath,
                    &bFileExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bFileExists)
    {
        dwError = ERROR_FILE_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    fp = fopen(pszNicknameFilePath, "r");
    if (!fp)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    while (1)
    {
        CHAR  szBuf[1024+1];
        PSTR  pszToken = NULL;
        PCSTR pszDelim = " \t\r\n";

        szBuf[0] = '\0';

        if (fgets(szBuf, 1024, fp) == NULL)
        {
            if (feof(fp))
            {
                break;
            }
            else
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        LwStripWhitespace(szBuf, TRUE, TRUE);

        if (!szBuf[0] || (szBuf[0] == '#'))
        {
            // skip comments
            continue;
        }

        if ((pszToken = strchr(szBuf, '#')))
        {
            // Skip trailing comments
            *pszToken = '\0';
        }

        pszToken = szBuf;

        if (nextTokenType == NIS_NICKNAME_ALIAS)
        {
            size_t stLen = 0;

            stLen = strcspn(pszToken, pszDelim);
            if (!stLen)
            {
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LwAllocateMemory(
                            sizeof(LSA_NIS_NICKNAME),
                            (PVOID*)&pNickname);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwStrndup(
                            pszToken,
                            stLen,
                            &pNickname->pszMapAlias);
            BAIL_ON_LSA_ERROR(dwError);

            // Skip token
            pszToken += stLen;

            stLen = strspn(pszToken, pszDelim);
            if (stLen)
            {
                // Skip delimiter
                pszToken += stLen;
            }

            nextTokenType = NIS_NICKNAME_NAME;
        }

        // The name might appear on the same line
        // Or it might appear on the next line
        if (nextTokenType == NIS_NICKNAME_NAME)
        {
            if (LW_IS_NULL_OR_EMPTY_STR(pszToken))
            {
                continue;
            }

            // The rest of the line is the name
            // we already removed trailing comments
            dwError = LwAllocateString(
                            pszToken,
                            &pNickname->pszMapName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwDLinkedListAppend(
                            &pNicknameList,
                            pNickname);
            BAIL_ON_LSA_ERROR(dwError);

            pNickname = NULL;

            nextTokenType = NIS_NICKNAME_ALIAS;
        }
    }

    *ppNicknameList = pNicknameList;

cleanup:

    if (fp)
    {
        fclose(fp);
    }

    if (pNickname)
    {
        LsaNISFreeNickname(pNickname);
    }

    return dwError;

error:

    *ppNicknameList = NULL;

    if (pNicknameList)
    {
        LsaNISFreeNicknameList(pNicknameList);
    }

    goto cleanup;
}

PCSTR
LsaNISLookupAlias(
    PLW_DLINKED_LIST pNicknameList,
    PCSTR pszAlias
    )
{
    PCSTR pszName = NULL;
    PLW_DLINKED_LIST pIter = pNicknameList;

    for (; !pszName && pIter; pIter = pIter->pNext)
    {
        PLSA_NIS_NICKNAME pNickname = (PLSA_NIS_NICKNAME)pIter->pItem;

        if (!strcasecmp(pNickname->pszMapAlias, pszAlias))
        {
            pszName = pNickname->pszMapName;
        }
    }

    return pszName;
}

VOID
LsaNISFreeNicknameList(
    PLW_DLINKED_LIST pNicknameList
    )
{
    LwDLinkedListForEach(
                pNicknameList,
                &LsaNISFreeNicknameInList,
                NULL);
    LwDLinkedListFree(pNicknameList);
}

static
VOID
LsaNISFreeNicknameInList(
    PVOID pItem,
    PVOID pUserData
    )
{
    if (pItem)
    {
        LsaNISFreeNickname((PLSA_NIS_NICKNAME)pItem);
    }
}

static
VOID
LsaNISFreeNickname(
    PLSA_NIS_NICKNAME pNickname
    )
{
    LW_SAFE_FREE_STRING(pNickname->pszMapAlias);
    LW_SAFE_FREE_STRING(pNickname->pszMapName);
    LwFreeMemory(pNickname);
}

