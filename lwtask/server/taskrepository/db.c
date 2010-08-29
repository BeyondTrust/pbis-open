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

static
DWORD
LwTaskDbGetSchemaCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    LW_TASK_TYPE        taskType,
    PDWORD              pdwNumArgInfos
    );

static
DWORD
LwTaskDbGetTaskId_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    PCSTR               pszTaskname,
    PDWORD              pdwTaskId
    );

static
DWORD
LwTaskDbGetTaskCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    PDWORD              pdwNumTasks
    );

static
DWORD
LwTaskDbGetTaskArgs_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId,
    PLW_TASK_ARG*       ppArgArray,
    PDWORD              pdwNumArgs
    );

static
DWORD
LwTaskDbGetTaskArgCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId,
    PDWORD              pdwNumArgs
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
LwTaskDbBeginTransaction(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    "BEGIN",
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
LwTaskDbRollbackTransaction(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    "ROLLBACK",
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
LwTaskDbCommitTransaction(
    PLW_TASK_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    "COMMIT",
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
        {   "\"" LW_TASK_MIGRATE_ARG_REMOTE_SERVER_NAME "\"",
            LW_TASK_ARG_TYPE_STRING,
            LW_TASK_ARG_FLAG_MANDATORY | LW_TASK_ARG_FLAG_PERSIST
        },
        {   "\"" LW_TASK_MIGRATE_ARG_REMOTE_SHARE_NAME "\"",
            LW_TASK_ARG_TYPE_STRING_MULTI_CSV,
            LW_TASK_ARG_FLAG_PERSIST
        },
        {   "\"" LW_TASK_MIGRATE_ARG_REMOTE_USER_PRINCIPAL "\"",
            LW_TASK_ARG_TYPE_STRING,
            LW_TASK_ARG_FLAG_MANDATORY | LW_TASK_ARG_FLAG_PERSIST
        },
        {   "\"" LW_TASK_MIGRATE_ARG_REMOTE_PASSWORD "\"",
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

    if (!pDbContext->pQueryTaskTypes)
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

DWORD
LwTaskDbGetSchema(
    PLW_TASK_DB_CONTEXT pDbContext,
    LW_TASK_TYPE        taskType,
    PLW_TASK_ARG_INFO*  ppArgInfoArray,
    PDWORD              pdwNumArgInfos
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLW_TASK_ARG_INFO  pArgInfoArray = NULL;
    DWORD   dwNumArgInfos = 0;
    DWORD   iArgInfo = 0;
    sqlite3_stmt* pSqlStatement = NULL;

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskDbGlobals.mutex);

    dwError = LwTaskDbGetSchemaCount_inlock(
                    pDbContext,
                    taskType,
                    &dwNumArgInfos);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!dwNumArgInfos)
    {
        goto done;
    }

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_ARG_INFO) * dwNumArgInfos,
                    (PVOID*)&pArgInfoArray);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!pDbContext->pQueryTaskSchemaArgs)
    {
        PCSTR pszQueryTemplate = "SELECT " LW_TASK_DB_COL_ARG_TYPE "," \
                                           LW_TASK_DB_COL_ARG_NAME "," \
                                           LW_TASK_DB_COL_ARG_FLAGS    \
                                 " FROM "  LW_TASK_SCHEMA_TABLE        \
                                 " WHERE " LW_TASK_DB_COL_TASK_TYPE " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskSchemaArgs,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskSchemaArgs;

    dwError = sqlite3_bind_int(
                    pSqlStatement,
                    1,
                    taskType);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    while ((dwError = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
        const unsigned char* pszStringVal = NULL;
        PLW_TASK_ARG_INFO pArgInfo = &pArgInfoArray[iArgInfo];

        DWORD dwNumAttrs = sqlite3_column_count(pSqlStatement);
        if (dwNumAttrs != 3)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        pArgInfo->argType = sqlite3_column_int(pSqlStatement, 0);

        pszStringVal = sqlite3_column_text(pSqlStatement, 1);
        if (pszStringVal)
        {
            dwError = LwAllocateString(
                            (PCSTR)pszStringVal,
                            &pArgInfo->pszArgName);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        pArgInfo->dwFlags = sqlite3_column_int(pSqlStatement, 2);

        iArgInfo++;
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

done:

    *ppArgInfoArray = pArgInfoArray;
    *pdwNumArgInfos = dwNumArgInfos;

cleanup:

    if (pDbContext->pQueryTaskSchemaArgs)
    {
        sqlite3_reset(pDbContext->pQueryTaskSchemaArgs);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskDbGlobals.mutex);

    return dwError;

error:

    *ppArgInfoArray = NULL;
    *pdwNumArgInfos   = 0;

    if (pArgInfoArray)
    {
        LwTaskFreeArgInfoArray(pArgInfoArray, dwNumArgInfos);
    }

    goto cleanup;
}

static
DWORD
LwTaskDbGetSchemaCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    LW_TASK_TYPE        taskType,
    PDWORD              pdwNumArgInfos
    )
{
    DWORD   dwError = 0;
    DWORD   dwNumArgInfos = 0;
    sqlite3_stmt* pSqlStatement = NULL;

    if (!pDbContext->pQueryTaskSchemaArgCountStmt)
    {
        PCSTR pszQueryTemplate = "SELECT count(*) FROM " LW_TASK_SCHEMA_TABLE \
                                 " WHERE " LW_TASK_DB_COL_TASK_TYPE " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskSchemaArgCountStmt,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskSchemaArgCountStmt;

    dwError = sqlite3_bind_int(
                    pSqlStatement,
                    1,
                    taskType);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwNumArgInfos = sqlite3_column_int(pSqlStatement, 0);

        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pdwNumArgInfos   = dwNumArgInfos;

cleanup:

    if (pDbContext->pQueryTaskSchemaArgCountStmt)
    {
        sqlite3_reset(pDbContext->pQueryTaskSchemaArgCountStmt);
    }

    return dwError;

error:

    *pdwNumArgInfos   = 0;

    goto cleanup;
}

DWORD
LwTaskDbCreateTask(
    PLW_TASK_DB_CONTEXT pDbContext,
    PCSTR               pszTaskname,
    LW_TASK_TYPE        taskType,
    PDWORD              pdwTaskId
    )
{
    DWORD   dwError  = 0;
    BOOLEAN bInLock  = FALSE;
    DWORD   dwTaskId = 0;

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskDbGlobals.mutex);

    if (!pDbContext->pInsertTaskStmt)
    {
        PCSTR pszQueryTemplate =
                        "INSERT INTO " LW_TASK_TABLE "("            \
                                       LW_TASK_DB_COL_TASK_NAME "," \
                                       LW_TASK_DB_COL_TASK_TYPE ")" \
                        " VALUES (?1,?2)";

        dwError = sqlite3_prepare_v2(
                    pDbContext->pDbHandle,
                    pszQueryTemplate,
                    -1,
                    &pDbContext->pInsertTaskStmt,
                    NULL);
        BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(dwError, pDbContext->pDbHandle);
    }

    dwError = sqlite3_bind_text(
                    pDbContext->pInsertTaskStmt,
                    1,
                    pszTaskname,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskStmt);

    dwError = sqlite3_bind_int(
                    pDbContext->pInsertTaskStmt,
                    2,
                    taskType);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskStmt);

    dwError = sqlite3_step(pDbContext->pInsertTaskStmt);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskStmt);

    dwError = LwTaskDbGetTaskId_inlock(pDbContext, pszTaskname, &dwTaskId);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *pdwTaskId = dwTaskId;

cleanup:

    if (pDbContext->pInsertTaskStmt)
    {
        sqlite3_reset(pDbContext->pInsertTaskStmt);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskDbGlobals.mutex);

    return dwError;

error:

    *pdwTaskId = 0;

    goto cleanup;
}

static
DWORD
LwTaskDbGetTaskId_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    PCSTR               pszTaskname,
    PDWORD              pdwTaskId
    )
{
    DWORD   dwError = 0;
    DWORD   dwTaskId = 0;
    sqlite3_stmt* pSqlStatement = NULL;

    if (!pDbContext->pQueryTaskId)
    {
        PCSTR pszQueryTemplate = "SELECT " LW_TASK_DB_COL_TASK_ID \
                                 " FROM  " LW_TASK_TABLE \
                                 " WHERE " LW_TASK_DB_COL_TASK_NAME " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskId,
                        NULL);
        BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(dwError, pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskId;

    dwError = sqlite3_bind_text(
                    pDbContext->pQueryTaskId,
                    1,
                    pszTaskname,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pQueryTaskId);

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwTaskId = sqlite3_column_int(pSqlStatement, 0);

        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pdwTaskId = dwTaskId;

cleanup:

    if (pDbContext->pQueryTaskId)
    {
        sqlite3_reset(pDbContext->pQueryTaskId);
    }

    return dwError;

error:

    *pdwTaskId = 0;

    goto cleanup;
}

DWORD
LwTaskDbCreateTaskArg(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId,
    PCSTR               pszArgName,
    PCSTR               pszArgValue,
    DWORD               dwArgType
    )
{
    DWORD   dwError  = 0;
    BOOLEAN bInLock  = FALSE;

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskDbGlobals.mutex);

    if (!pDbContext->pInsertTaskArgStmt)
    {
        PCSTR pszQueryTemplate =
                        "INSERT INTO " LW_TASK_ARGS_TABLE "(" \
                        LW_TASK_DB_COL_TASK_ID   "," \
                        LW_TASK_DB_COL_ARG_TYPE  "," \
                        LW_TASK_DB_COL_ARG_NAME  "," \
                        LW_TASK_DB_COL_ARG_VALUE ")" \
                        " VALUES (?1,?2,?3,?4)";

        dwError = sqlite3_prepare_v2(
                    pDbContext->pDbHandle,
                    pszQueryTemplate,
                    -1,
                    &pDbContext->pInsertTaskArgStmt,
                    NULL);
        BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(dwError, pDbContext->pDbHandle);
    }

    dwError = sqlite3_bind_int(
                    pDbContext->pInsertTaskArgStmt,
                    1,
                    dwTaskId);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskArgStmt);

    dwError = sqlite3_bind_int(
                    pDbContext->pInsertTaskArgStmt,
                    2,
                    dwArgType);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskArgStmt);

    dwError = sqlite3_bind_text(
                    pDbContext->pInsertTaskArgStmt,
                    3,
                    pszArgName,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskArgStmt);

    if (pszArgValue)
    {
        dwError = sqlite3_bind_text(
                        pDbContext->pInsertTaskArgStmt,
                        4,
                        pszArgValue,
                        -1,
                        SQLITE_TRANSIENT);
    }
    else
    {
        dwError = sqlite3_bind_null(pDbContext->pInsertTaskArgStmt, 4);
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskArgStmt);

    dwError = sqlite3_step(pDbContext->pInsertTaskArgStmt);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pInsertTaskArgStmt);

cleanup:

    if (pDbContext->pInsertTaskArgStmt)
    {
        sqlite3_reset(pDbContext->pInsertTaskArgStmt);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskDbGlobals.mutex);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskDbGetTasks(
    PLW_TASK_DB_CONTEXT pDbContext,
    PLW_SRV_DB_TASK*    ppTaskArray,
    PDWORD              pdwNumTasks
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    sqlite3_stmt* pSqlStatement = NULL;
    PLW_SRV_DB_TASK pTaskArray = NULL;
    DWORD           dwNumTasks = 0;
    DWORD           iTask = 0;

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskDbGlobals.mutex);

    dwError = LwTaskDbGetTaskCount_inlock(pDbContext, &dwNumTasks);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!dwNumTasks)
    {
        goto done;
    }

    dwError = LwAllocateMemory(
                    sizeof(LW_SRV_DB_TASK) * dwNumTasks,
                    (PVOID*)&pTaskArray);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!pDbContext->pQueryTasks)
    {
        PCSTR pszQueryTemplate = "SELECT " LW_TASK_DB_COL_TASK_NAME "," \
                                           LW_TASK_DB_COL_TASK_ID ","   \
                                           LW_TASK_DB_COL_TASK_TYPE     \
                                 " FROM "  LW_TASK_TABLE;

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTasks,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTasks;

    while ((dwError = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
        const unsigned char* pszStringVal = NULL;
        PLW_SRV_DB_TASK pTask = &pTaskArray[iTask];

        DWORD dwNumAttrs = sqlite3_column_count(pSqlStatement);
        if (dwNumAttrs != 3)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        pszStringVal = sqlite3_column_text(pSqlStatement, 0);
        if (pszStringVal)
        {
            dwError = LwAllocateString(
                            (PCSTR)pszStringVal,
                            &pTask->pszTaskName);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        pTask->dwTaskId = sqlite3_column_int(pSqlStatement, 1);

        pTask->taskType = sqlite3_column_int(pSqlStatement, 2);

        iTask++;
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    for (iTask = 0; iTask < dwNumTasks; iTask++)
    {
        PLW_SRV_DB_TASK pTask = &pTaskArray[iTask];

        dwError = LwTaskDbGetTaskArgs_inlock(
                        pDbContext,
                        pTask->dwTaskId,
                        &pTask->pArgArray,
                        &pTask->dwNumArgs);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

done:

    *ppTaskArray = pTaskArray;
    *pdwNumTasks = dwNumTasks;

cleanup:

    if (pDbContext->pQueryTasks)
    {
        sqlite3_reset(pDbContext->pQueryTasks);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskDbGlobals.mutex);

    return dwError;

error:

    *ppTaskArray = NULL;
    *pdwNumTasks = 0;

    if (pTaskArray)
    {
        LwTaskDbFreeTaskArray(pTaskArray, dwNumTasks);
    }

    goto cleanup;
}

static
DWORD
LwTaskDbGetTaskCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    PDWORD              pdwNumTasks
    )
{
    DWORD   dwError = 0;
    DWORD   dwNumTasks = 0;
    sqlite3_stmt* pSqlStatement = NULL;

    if (!pDbContext->pQueryTaskArgCountStmt)
    {
        PCSTR pszQueryTemplate = "SELECT count(*) FROM " LW_TASK_TABLE;

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskCountStmt,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskCountStmt;

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwNumTasks = sqlite3_column_int(pSqlStatement, 0);

        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pdwNumTasks = dwNumTasks;

cleanup:

    if (pDbContext->pQueryTaskCountStmt)
    {
        sqlite3_reset(pDbContext->pQueryTaskCountStmt);
    }

    return dwError;

error:

    *pdwNumTasks = 0;

    goto cleanup;
}

static
DWORD
LwTaskDbGetTaskArgs_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId,
    PLW_TASK_ARG*       ppArgArray,
    PDWORD              pdwNumArgs
    )
{
    DWORD dwError = 0;
    sqlite3_stmt* pSqlStatement = NULL;
    PLW_TASK_ARG pArgArray = NULL;
    DWORD        dwNumArgs = 0;
    DWORD        iArg = 0;

    dwError = LwTaskDbGetTaskArgCount_inlock(pDbContext, dwTaskId, &dwNumArgs);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!dwNumArgs)
    {
        goto done;
    }

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_ARG) * dwNumArgs,
                    (PVOID*)&pArgArray);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!pDbContext->pQueryTaskArgs)
    {
        PCSTR pszQueryTemplate = "SELECT " LW_TASK_DB_COL_ARG_NAME "," \
                                           LW_TASK_DB_COL_ARG_TYPE "," \
                                           LW_TASK_DB_COL_ARG_VALUE    \
                                 " FROM  " LW_TASK_ARGS_TABLE          \
                                 " WHERE " LW_TASK_DB_COL_TASK_ID " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskArgs,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskArgs;

    dwError = sqlite3_bind_int(
                    pSqlStatement,
                    1,
                    dwTaskId);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    while ((dwError = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
        const unsigned char* pszStringVal = NULL;
        PLW_TASK_ARG pArg = &pArgArray[iArg];

        DWORD dwNumAttrs = sqlite3_column_count(pSqlStatement);
        if (dwNumAttrs != 3)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        pszStringVal = sqlite3_column_text(pSqlStatement, 0);
        if (pszStringVal)
        {
            dwError = LwAllocateString(
                            (PCSTR)pszStringVal,
                            &pArg->pszArgName);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        pArg->dwArgType = sqlite3_column_int(pSqlStatement, 1);

        pszStringVal = sqlite3_column_text(pSqlStatement, 2);
        if (pszStringVal)
        {
            dwError = LwAllocateString(
                            (PCSTR)pszStringVal,
                            &pArg->pszArgValue);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        iArg++;
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

done:

    *ppArgArray = pArgArray;
    *pdwNumArgs = dwNumArgs;

cleanup:

    if (pDbContext->pQueryTaskArgs)
    {
        sqlite3_reset(pDbContext->pQueryTaskArgs);
    }

    return dwError;

error:

    *ppArgArray = NULL;
    *pdwNumArgs = 0;

    if (pArgArray)
    {
        LwTaskFreeArgArray(pArgArray, dwNumArgs);
    }

    goto cleanup;
}

static
DWORD
LwTaskDbGetTaskArgCount_inlock(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId,
    PDWORD              pdwNumArgs
    )
{
    DWORD   dwError = 0;
    DWORD   dwNumArgs = 0;
    sqlite3_stmt* pSqlStatement = NULL;

    if (!pDbContext->pQueryTaskArgCountStmt)
    {
        PCSTR pszQueryTemplate = "SELECT count(*) FROM " LW_TASK_ARGS_TABLE \
                                 " WHERE " LW_TASK_DB_COL_TASK_ID " = ?1";

        dwError = sqlite3_prepare_v2(
                        pDbContext->pDbHandle,
                        pszQueryTemplate,
                        -1,
                        &pDbContext->pQueryTaskArgCountStmt,
                        NULL);
            BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(
                            dwError,
                            pDbContext->pDbHandle);
    }

    pSqlStatement = pDbContext->pQueryTaskArgCountStmt;

    dwError = sqlite3_bind_int(
                    pSqlStatement,
                    1,
                    dwTaskId);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if ((dwError = sqlite3_step(pSqlStatement) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pSqlStatement) != 1)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        dwNumArgs = sqlite3_column_int(pSqlStatement, 0);

        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    *pdwNumArgs = dwNumArgs;

cleanup:

    if (pDbContext->pQueryTaskArgCountStmt)
    {
        sqlite3_reset(pDbContext->pQueryTaskArgCountStmt);
    }

    return dwError;

error:

    *pdwNumArgs = 0;

    goto cleanup;
}

DWORD
LwTaskDbDeleteTask(
    PLW_TASK_DB_CONTEXT pDbContext,
    DWORD               dwTaskId
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskDbGlobals.mutex);

    if (!pDbContext->pDelTaskStmt)
    {
        PCSTR pszDelQueryTemplate =
                        "DELETE FROM " LW_TASK_TABLE \
                        " WHERE " LW_TASK_DB_COL_TASK_ID " = ?1";

        dwError = sqlite3_prepare_v2(
                    pDbContext->pDbHandle,
                    pszDelQueryTemplate,
                    -1,
                    &pDbContext->pDelTaskStmt,
                    NULL);
        BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(dwError, pDbContext->pDbHandle);
    }

    dwError = sqlite3_bind_int(
                    pDbContext->pDelTaskStmt,
                    1,
                    dwTaskId);
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pDelTaskStmt);

    dwError = sqlite3_step(pDbContext->pDelTaskStmt);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pDbContext->pDelTaskStmt);

cleanup:

    if (pDbContext->pDelTaskStmt)
    {
        sqlite3_reset(pDbContext->pDelTaskStmt);
    }

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskDbGlobals.mutex);

    return dwError;

error:

    goto cleanup;
}

VOID
LwTaskDbFreeTaskArray(
    PLW_SRV_DB_TASK pTaskArray,
    DWORD           dwNumTasks
    )
{
    DWORD iTask = 0;

    for (; iTask < dwNumTasks; iTask++)
    {
        PLW_SRV_DB_TASK pTask = &pTaskArray[iTask];

        if (pTask->pszTaskName)
        {
            LwFreeMemory(pTask->pszTaskName);
        }

        if (pTask->pArgArray)
        {
            LwTaskFreeArgArray(pTask->pArgArray, pTask->dwNumArgs);
        }
    }

    LwFreeMemory(pTaskArray);
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

    if (pDbContext->pQueryTaskSchemaArgCountStmt)
    {
        sqlite3_finalize(pDbContext->pQueryTaskSchemaArgCountStmt);
    }

    if (pDbContext->pQueryTaskSchemaArgs)
    {
        sqlite3_finalize(pDbContext->pQueryTaskSchemaArgs);
    }

    if (pDbContext->pQueryTaskCountStmt)
    {
        sqlite3_finalize(pDbContext->pQueryTaskCountStmt);
    }

    if (pDbContext->pQueryTasks)
    {
        sqlite3_finalize(pDbContext->pQueryTasks);
    }

    if (pDbContext->pQueryTaskArgCountStmt)
    {
        sqlite3_finalize(pDbContext->pQueryTaskArgCountStmt);
    }

    if (pDbContext->pQueryTaskArgs)
    {
        sqlite3_finalize(pDbContext->pQueryTaskArgs);
    }

    if (pDbContext->pDelTaskStmt)
    {
        sqlite3_finalize(pDbContext->pDelTaskStmt);
    }

    if (pDbContext->pInsertTaskStmt)
    {
        sqlite3_finalize(pDbContext->pInsertTaskStmt);
    }

    if (pDbContext->pInsertTaskArgStmt)
    {
        sqlite3_finalize(pDbContext->pInsertTaskArgStmt);
    }

    if (pDbContext->pQueryTaskId)
    {
        sqlite3_finalize(pDbContext->pQueryTaskId);
    }

    if (pDbContext->pDbHandle)
    {
        sqlite3_close(pDbContext->pDbHandle);
    }

    LwFreeMemory(pDbContext);
}
