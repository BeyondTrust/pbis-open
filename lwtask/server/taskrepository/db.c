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
 *        db.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Task Repository
 *
 *        Database support
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwTaskDbInitConfig(
    PLW_TASK_DB_CONTEXT pDbContext
    );

static
DWORD
LwTaskDbInitTaskTypes(
    PLW_TASK_DB_CONTEXT pDbContext
    );

static
DWORD
LwTaskDbInitTaskArgs(
    PLW_TASK_DB_CONTEXT pDbContext
    );

static
DWORD
LwTaskDbAddArgsForTaskType(
    PLW_TASK_DB_CONTEXT pDbContext,
    LW_TASK_TYPE        taskType,
    PCSTR               pszArgName,
    LW_TASK_ARG_TYPE    argType,
    LW_TASK_ARG_FLAG    dwArgFlags
    );

static
DWORD
LwTaskDbGetTaskTypeCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    PDWORD              pdwNumTaskTypes
    );

DWORD
LwTaskDbOpen(
    PLW_TASK_DB_CONTEXT* ppDbContext
    )
{
    DWORD dwError = 0;
    PCSTR pszDbPath = LW_TASK_DB;
    PLW_TASK_DB_CONTEXT pDbContext = NULL;

    dwError = LwAllocateMemory(sizeof(LW_TASK_DB_CONTEXT), (PVOID*)&pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = sqlite3_open(pszDbPath, &pDbContext->pDbHandle);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppDbContext = pDbContext;

cleanup:

    return dwError;

error:

    *ppDbContext = NULL;

    if (pDbContext)
    {
        LwTaskDbClose(pDbContext);
    }

    goto cleanup;
}

DWORD
LwTaskDbCreateTables(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR  pszError = NULL;
    PCSTR pszQuery = LW_TASK_DB_QUERY_CREATE_TABLES;

    LW_TASK_LOG_DEBUG("Query used to create tables [%s]", pszQuery);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    return dwError;

error:

    LW_TASK_LOG_DEBUG(  "Sqlite3 Error (code: %u): %s",
                        dwError,
                        LW_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
LwTaskDbAddDefaultEntries(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;

    dwError = LwTaskDbInitConfig(pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbInitTaskTypes(pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDbInitTaskArgs(pDbContext);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskDbInitConfig(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PCSTR pszQueryTemplate =
            "INSERT INTO " LW_TASK_DB_CONFIG_TABLE \
            " (Version"         \
            " )"                \
            " VALUES ("         \
            "  %u"
            " )";
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;

    pszQuery = sqlite3_mprintf(pszQueryTemplate, LW_TASK_DB_SCHEMA_VERSION);

    if (pszQuery == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    return dwError;

error:

    LW_TASK_LOG_DEBUG(  "Error (code: %u): %s",
                        dwError,
                        LW_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

static
DWORD
LwTaskDbInitTaskTypes(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PCSTR pszQueryTemplate =
            "INSERT INTO " LW_TASK_TYPES_TABLE \
            " (" LW_TASK_DB_COL_TASK_TYPE ","  \
                 LW_TASK_DB_COL_DESCRIPTION    \
            " )"        \
            " VALUES (" \
            "  %u,"     \
            "  %s"      \
            " )";
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    struct
    {
        DWORD dwTaskType;
        PCSTR pszDescription;
    }
    taskTypes[] =
    {
        { LW_TASK_TYPE_MIGRATE, "\"Likewise share migration task\"" }
    };
    DWORD dwNumTaskTypes = sizeof(taskTypes)/sizeof(taskTypes[0]);
    DWORD iTaskType = 0;

    for (iTaskType = 0; iTaskType < dwNumTaskTypes; iTaskType++)
    {
        if (pszQuery)
        {
            sqlite3_free(pszQuery);
            pszQuery = NULL;
        }

        pszQuery = sqlite3_mprintf(
                        pszQueryTemplate,
                        taskTypes[iTaskType].dwTaskType,
                        taskTypes[iTaskType].pszDescription);

        if (pszQuery == NULL)
        {
            dwError = LW_ERROR_OUT_OF_MEMORY;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwError = sqlite3_exec(
                        pDbContext->pDbHandle,
                        pszQuery,
                        NULL,
                        NULL,
                        &pszError);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    return dwError;

error:

    LW_TASK_LOG_DEBUG(  "Error (code: %u): %s",
                        dwError,
                        LW_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

static
DWORD
LwTaskDbInitTaskArgs(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    struct
    {
        PCSTR            pszArgName;
        LW_TASK_ARG_TYPE dwArgType;
        LW_TASK_ARG_FLAG dwFlags;
    }
    migrateTaskArgs[] =
    {
        {   "\"remote-server-name\"",
            LW_TASK_ARG_TYPE_STRING,
            LW_TASK_ARG_FLAG_MANDATORY | LW_TASK_ARG_FLAG_PERSIST
        },
        {   "\"remote-share-name\"",
            LW_TASK_ARG_TYPE_STRING_MULTI_CSV,
            LW_TASK_ARG_FLAG_PERSIST
        },
        {   "\"remote-user-principal\"",
            LW_TASK_ARG_TYPE_STRING,
            LW_TASK_ARG_FLAG_MANDATORY | LW_TASK_ARG_FLAG_PERSIST
        },
        {   "\"remote-password\"",
            LW_TASK_ARG_TYPE_STRING,
            LW_TASK_ARG_FLAG_MANDATORY
        }
    };
    DWORD dwNumTaskArgs = sizeof(migrateTaskArgs)/sizeof(migrateTaskArgs[0]);
    DWORD iTaskArg = 0;

    for (; iTaskArg < dwNumTaskArgs; iTaskArg++)
    {
        dwError = LwTaskDbAddArgsForTaskType(
                        pDbContext,
                        LW_TASK_TYPE_MIGRATE,
                        migrateTaskArgs[iTaskArg].pszArgName,
                        migrateTaskArgs[iTaskArg].dwArgType,
                        migrateTaskArgs[iTaskArg].dwFlags
                        );
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskDbAddArgsForTaskType(
    PLW_TASK_DB_CONTEXT pDbContext,
    LW_TASK_TYPE        taskType,
    PCSTR               pszArgName,
    LW_TASK_ARG_TYPE    argType,
    LW_TASK_ARG_FLAG    dwArgFlags
    )
{
    DWORD dwError = 0;
    PCSTR pszQueryTemplate =
            "INSERT INTO " LW_TASK_SCHEMA_TABLE \
            " (" LW_TASK_DB_COL_TASK_TYPE ","   \
                 LW_TASK_DB_COL_ARG_TYPE  ","   \
                 LW_TASK_DB_COL_ARG_NAME  ","   \
                 LW_TASK_DB_COL_ARG_FLAGS       \
            " )"                \
            " VALUES ("         \
            "  %u,"
            "  %u,"
            "  %s,"
            "  %u"
            " )";
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;

    pszQuery = sqlite3_mprintf(
                    pszQueryTemplate,
                    taskType,
                    argType,
                    pszArgName,
                    dwArgFlags);

    if (pszQuery == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    return dwError;

error:

    LW_TASK_LOG_DEBUG(  "Error (code: %u): %s",
                        dwError,
                        LW_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
LwTaskDbGetTypes(
    PLW_TASK_DB_CONTEXT pDbContext,
    PDWORD*             ppdwTaskTypeArray,
    PDWORD              pdwNumTaskTypes
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;
    PDWORD  pdwTaskTypeArray = NULL;
    PDWORD  pCursor = NULL;
    DWORD   dwNumTaskTypes = 0;
    sqlite3_stmt* pSqlStatement = NULL;

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskDbGlobals.mutex);

    dwError = LwTaskDbGetTaskTypeCount_inlock(pDbContext, &dwNumTaskTypes);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!dwNumTaskTypes)
    {
        goto done;
    }

    dwError = LwAllocateMemory(
                    sizeof(DWORD) * dwNumTaskTypes,
                    (PVOID*)&pdwTaskTypeArray);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pCursor = pdwTaskTypeArray;

    if (!pDbContext->pQueryTaskTypeCountStmt)
    {
        PCSTR pszQueryTemplate = "SELECT " LW_TASK_DB_COL_TASK_TYPE \
                                 " FROM "  LW_TASK_TYPES_TABLE;

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskTypes,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskTypes;

    while ((dwError = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
        DWORD dwNumAttrs = sqlite3_column_count(pSqlStatement);
        if (dwNumAttrs != 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        *pCursor++ = sqlite3_column_int(pSqlStatement, 0);
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

done:

    *ppdwTaskTypeArray = pdwTaskTypeArray;
    *pdwNumTaskTypes   = dwNumTaskTypes;

cleanup:

    if (pDbContext->pQueryTaskTypes)
    {
        sqlite3_reset(pDbContext->pQueryTaskTypes);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskDbGlobals.mutex);

    return dwError;

error:

    *ppdwTaskTypeArray = NULL;
    *pdwNumTaskTypes   = 0;

    LW_SAFE_FREE_MEMORY(pdwTaskTypeArray);

    goto cleanup;
}

static
DWORD
LwTaskDbGetTaskTypeCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    PDWORD              pdwNumTaskTypes
    )
{
    DWORD   dwError = 0;
    DWORD   dwNumTaskTypes = 0;
    sqlite3_stmt* pSqlStatement = NULL;

    if (!pDbContext->pQueryTaskTypeCountStmt)
    {
        PCSTR pszQueryTemplate = "SELECT count(*) FROM " LW_TASK_TYPES_TABLE;

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskTypeCountStmt,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskTypeCountStmt;

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwNumTaskTypes = sqlite3_column_int(pSqlStatement, 0);

        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pdwNumTaskTypes   = dwNumTaskTypes;

cleanup:

    if (pDbContext->pQueryTaskTypeCountStmt)
    {
        sqlite3_reset(pDbContext->pQueryTaskTypeCountStmt);
    }

    return dwError;

error:

    *pdwNumTaskTypes   = 0;

    goto cleanup;
}

VOID
LwTaskDbClose(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    if (pDbContext->pQueryTaskTypes)
    {
        sqlite3_finalize(pDbContext->pQueryTaskTypes);
    }

    if (pDbContext->pQueryTaskTypeCountStmt)
    {
        sqlite3_finalize(pDbContext->pQueryTaskTypeCountStmt);
    }

    if (pDbContext->pDbHandle)
    {
        sqlite3_close(pDbContext->pDbHandle);
    }

    LwFreeMemory(pDbContext);
}
