/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Driver for program to migrate locally defined users and groups
 *
 * Authors:
 *
 *        Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

#ifdef NOT_YET

#define MAXLINELEN 1024

#define DB_QUERY_INSERT_GROUP_MEMBERSHIP                     \
    "INSERT INTO lwigroupmembers                             \
                 (Gid, Uid) values (%d, %d)"

static
IsComment(
    PSTR pszLine
    )
{
    if (LW_IS_NULL_OR_EMPTY_STR(pszLine))
        return TRUE;

    while (*pszLine != '\0' && isspace((int)*pszLine))
        pszLine++;

    return *pszLine == '#' || *pszLine == '\0';
}

static
DWORD
LsaSrvPopulateGroups(
    sqlite3* pDbHandle
    )
{
    DWORD dwError = 0;
    FILE* fp = NULL;
    CHAR szLine[MAXLINELEN+1]; // Maybe, this is not enough?
    PSTR pszBuf = NULL;
    PSTR pszToken = NULL;
    PSTR pszGroupname = NULL;
    PSTR pszPassword = NULL;
    gid_t gid = 0;
    BOOLEAN bReleaseLock = FALSE;

    if ((fp = fopen("/etc/group", "r")) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_RW_WRITER_LOCK;

    bReleaseLock = TRUE;

    while (1) {
        char* pszSavePtr = NULL;
        DWORD iToken = 0;
        BOOLEAN bKeepParsing = TRUE;

        if (fgets(szLine, MAXLINELEN, fp) == NULL) {
            if (feof(fp)) {
                break;
            } else {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        LwStripWhitespace(szLine, TRUE, TRUE);

        if (IsComment(szLine))
            continue;

        dwError = LwAllocateString(szLine, &pszBuf);
        BAIL_ON_LSA_ERROR(dwError);

        iToken = 0;
        gid = 0;
        pszToken = strtok_r(pszBuf, ":", &pszSavePtr);
        while (bKeepParsing) {
            switch (iToken++) {
                case 0:
                {
                    dwError = LwAllocateString(pszToken, &pszGroupname);
                    BAIL_ON_LSA_ERROR(dwError);
                    break;
                }
                case 1:
                {
                    if (!LW_IS_NULL_OR_EMPTY_STR(pszToken)) {
                       dwError = LwAllocateString(pszToken, &pszPassword);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 2:
                {
                    gid = atoi(pszToken);
                    bKeepParsing = FALSE;
                    break;
                }
            }
            pszToken = strtok_r(NULL, ":", &pszSavePtr);
        }

        dwError = LsaSrvAddGroup_Unsafe(
                           pDbHandle,
                           pszGroupname,
                           pszPassword,
                           gid);
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_STRING(pszBuf);
        LW_SAFE_FREE_STRING(pszGroupname);
        LW_SAFE_FREE_STRING(pszPassword);
    }

cleanup:

    if (fp) {
        fclose(fp);
    }

    if (bReleaseLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    LW_SAFE_FREE_STRING(pszBuf);
    LW_SAFE_FREE_STRING(pszGroupname);
    LW_SAFE_FREE_STRING(pszPassword);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LsaSrvPopulateUsers(
    sqlite3* pDbHandle
    )
{
    DWORD dwError = 0;
    FILE* fp = NULL;
    CHAR szLine[MAXLINELEN+1]; // Maybe, this is not enough?
    PSTR pszBuf = NULL;
    PSTR pszToken = NULL;
    PSTR pszUsername = NULL;
    PSTR pszPassword = NULL;
    uid_t uid = 0;
    gid_t gid = 0;
    PSTR pszGecos = NULL;
    PSTR pszHomedir = NULL;
    PSTR pszShell = NULL;

    BOOLEAN bReleaseLock = FALSE;

    if ((fp = fopen("/etc/passwd", "r")) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_RW_WRITER_LOCK;

    bReleaseLock = TRUE;

    while (1) {
        char* pszSavePtr = NULL;
        DWORD iToken = 0;
        BOOLEAN bKeepParsing = TRUE;

        if (fgets(szLine, MAXLINELEN, fp) == NULL) {
            if (feof(fp)) {
                break;
            } else {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        LwStripWhitespace(szLine, TRUE, TRUE);

        if (IsComment(szLine))
            continue;

        dwError = LwAllocateString(szLine, &pszBuf);
        BAIL_ON_LSA_ERROR(dwError);

        iToken = 0;
        uid = 0;
        gid = 0;
        pszToken = strtok_r(pszBuf, ":", &pszSavePtr);
        while (bKeepParsing) {
            switch (iToken++) {
                case 0:
                {
                    dwError = LwAllocateString(pszToken, &pszUsername);
                    BAIL_ON_LSA_ERROR(dwError);
                    break;
                }
                case 1:
                {
                    if (!LW_IS_NULL_OR_EMPTY_STR(pszToken)) {
                       dwError = LwAllocateString(pszToken, &pszPassword);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 2:
                {
                    uid = atoi(pszToken);
                    break;
                }
                case 3:
                {
                    gid = atoi(pszToken);
                    break;
                }
                case 4:
                {
                    if (!LW_IS_NULL_OR_EMPTY_STR(pszToken)) {
                        dwError = LwAllocateString(pszToken, &pszGecos);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 5:
                {
                    if (!LW_IS_NULL_OR_EMPTY_STR(pszToken)) {
                        dwError = LwAllocateString(pszToken, &pszHomedir);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 6:
                {
                    if (!LW_IS_NULL_OR_EMPTY_STR(pszToken)) {
                        dwError = LwAllocateString(pszToken, &pszShell);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                    bKeepParsing = FALSE;
                    break;
                }
            }
            pszToken = strtok_r(NULL, ":", &pszSavePtr);
        }

        dwError = LsaSrvAddUser_Unsafe(
                        pDbHandle,
                        pszUsername,
                        pszPassword,
                        uid,
                        gid,
                        pszGecos,
                        pszShell,
                        pszHomedir
                        );
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_STRING(pszBuf);
        LW_SAFE_FREE_STRING(pszUsername);
        LW_SAFE_FREE_STRING(pszPassword);
        LW_SAFE_FREE_STRING(pszGecos);
        LW_SAFE_FREE_STRING(pszHomedir);
        LW_SAFE_FREE_STRING(pszShell);
    }

cleanup:

    if (fp) {
        fclose(fp);
    }

    if (bReleaseLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    LW_SAFE_FREE_STRING(pszBuf);
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszGecos);
    LW_SAFE_FREE_STRING(pszHomedir);
    LW_SAFE_FREE_STRING(pszShell);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LsaSrvPopulateGroupMembers(
    sqlite3* pDbHandle
    )
{
    DWORD dwError = 0;
    FILE* fp = NULL;
    CHAR szLine[MAXLINELEN+1]; // Maybe, this is not enough?
    PSTR pszBuf = NULL;
    PSTR pszToken = NULL;
    PSTR pszGroupname = NULL;
    PSTR pszPassword = NULL;
    gid_t gid = 0;
    PSTR pszGroupMembers = NULL;
    PSTR pszError = NULL;
    PSTR pszQuery = NULL;
    BOOLEAN bReleaseLock = FALSE;

    if ((fp = fopen("/etc/group", "r")) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_RW_WRITER_LOCK;

    bReleaseLock = TRUE;

    while (1) {
        char* pszSavePtr = NULL;
        DWORD iToken = 0;
        BOOLEAN bKeepParsing = TRUE;

        if (fgets(szLine, MAXLINELEN, fp) == NULL) {
            if (feof(fp)) {
                break;
            } else {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        LwStripWhitespace(szLine, TRUE, TRUE);

        if (IsComment(szLine))
            continue;

        dwError = LwAllocateString(szLine, &pszBuf);
        BAIL_ON_LSA_ERROR(dwError);

        iToken = 0;
        gid = 0;
        pszToken = strtok_r(pszBuf, ":", &pszSavePtr);
        while (bKeepParsing) {
            switch (iToken++) {
                case 0:
                {
                    dwError = LwAllocateString(pszToken, &pszGroupname);
                    BAIL_ON_LSA_ERROR(dwError);
                    break;
                }
                case 1:
                {
                    if (!LW_IS_NULL_OR_EMPTY_STR(pszToken)) {
                       dwError = LwAllocateString(pszToken, &pszPassword);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 2:
                {
                    gid = atoi(pszToken);
                    break;
                }
                case 3:
                {
                    if (!LW_IS_NULL_OR_EMPTY_STR(pszToken)) {
                        dwError = LwAllocateString(pszToken, &pszGroupMembers);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                    bKeepParsing = FALSE;
                    break;
                }
            }
            pszToken = strtok_r(NULL, ":", &pszSavePtr);
        }

        LwStripWhitespace(pszGroupMembers, TRUE, TRUE);

        if (!LW_IS_NULL_OR_EMPTY_STR(pszGroupMembers)) {
            uid_t candidateUid = 0;
            BOOLEAN bExists = FALSE;

            pszToken = strtok_r(pszGroupMembers, ",", &pszSavePtr);
            while (pszToken) {

                dwError = LsaProviderLocal_GetUid_Unsafe(
                                pDbHandle,
                                pszToken,
                                &candidateUid);
                BAIL_ON_LSA_ERROR(dwError);

                bExists = FALSE;
                dwError = LsaProviderLocal_CheckGroupMembershipRecord_Unsafe(
                                pDbHandle,
                                candidateUid,
                                gid,
                                &bExists);
                BAIL_ON_LSA_ERROR(dwError);

                if (!bExists) {

                    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_GROUP_MEMBERSHIP,
                                               gid,
                                               candidateUid
                                               );

                    if (pszQuery == NULL)
                    {
                        dwError = LW_ERROR_OUT_OF_MEMORY;
                        BAIL_ON_LSA_ERROR(dwError);
                    }

                    dwError = sqlite3_exec(pDbHandle,
                                           pszQuery,
                                           NULL,
                                           NULL,
                                           &pszError);
                    BAIL_ON_LSA_ERROR(dwError);

                    if (pszQuery) {
                       sqlite3_free(pszQuery);
                       pszQuery = NULL;
                    }
                }
                pszToken = strtok_r(NULL, ",", &pszSavePtr);
            }
        }

        LW_SAFE_FREE_STRING(pszBuf);
        LW_SAFE_FREE_STRING(pszGroupname);
        LW_SAFE_FREE_STRING(pszPassword);
        LW_SAFE_FREE_STRING(pszGroupMembers);
    }

cleanup:

    if (fp) {
        fclose(fp);
    }

    if (bReleaseLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    LW_SAFE_FREE_STRING(pszBuf);
    LW_SAFE_FREE_STRING(pszGroupname);
    LW_SAFE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszGroupMembers);

    return dwError;

error:

    if (pszError) {
        LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

static
DWORD
LsaSrvPopulateDB(
    sqlite3* pDbHandle
    )
{
    DWORD dwError = 0;

    dwError = LsaSrvPopulateGroups(pDbHandle);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvPopulateUsers(pDbHandle);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvPopulateGroupMembers(pDbHandle);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

#endif /* NOT_YET */

int
migrator_main(
    int argc,
    char* argv[]
    )
{
    printf("lw-migrator is not implemented yet.\n");
    return 0;
}
