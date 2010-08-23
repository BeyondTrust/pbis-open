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
 *        taskrepository.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Task Repository
 *
 *        Definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define LW_TASK_DB_SCHEMA_VERSION     1

#define LW_TASK_DB_DIR CACHEDIR       "/db"
#define LW_TASK_DB     LW_TASK_DB_DIR "/lwtask.db"

#define LW_TASK_DB_CONFIG_TABLE  "taskdbconfig"
#define LW_TASK_TYPES_TABLE      "tasktypes"
#define LW_TASK_SCHEMA_TABLE     "taskschema"
#define LW_TASK_TABLE            "tasks"
#define LW_TASK_ARGS_TABLE       "taskargs"
#define LW_TASK_STATUS_TABLE     "taskstatus"

#define LW_TASK_DB_COL_TASK_ID      "id"
#define LW_TASK_DB_COL_TASK_NAME    "taskname"
#define LW_TASK_DB_COL_TASK_TYPE    "tasktype"
#define LW_TASK_DB_COL_CREATED_TIME "created"
#define LW_TASK_DB_COL_START_TIME   "started"
#define LW_TASK_DB_COL_END_TIME     "ended"
#define LW_TASK_DB_COL_ARG_TYPE     "argtype"
#define LW_TASK_DB_COL_ARG_NAME     "argname"
#define LW_TASK_DB_COL_ARG_VALUE    "argvalue"
#define LW_TASK_DB_COL_ARG_FLAGS    "flags"
#define LW_TASK_DB_COL_DESCRIPTION  "description"

#define LW_TASK_DB_QUERY_CREATE_TABLES  \
    "CREATE TABLE " LW_TASK_DB_CONFIG_TABLE " (\n"                             \
                 "Version     INTEGER\n"                                       \
                 ");\n"                                                        \
    "CREATE TABLE " LW_TASK_TYPES_TABLE " (\n"                                 \
            LW_TASK_DB_COL_TASK_TYPE   " INTEGER PRIMARY KEY,\n"               \
            LW_TASK_DB_COL_DESCRIPTION " TEXT COLLATE NOCASE,\n"               \
            LW_TASK_DB_COL_CREATED_TIME " DATE DEFAULT (DATETIME('now')));\n"  \
    "CREATE TABLE " LW_TASK_SCHEMA_TABLE " (\n"                                \
            LW_TASK_DB_COL_TASK_TYPE     " INTEGER,\n"                         \
            LW_TASK_DB_COL_ARG_TYPE      " INTEGER,\n"                         \
            LW_TASK_DB_COL_ARG_NAME      " TEXT COLLATE NOCASE,\n"             \
            LW_TASK_DB_COL_ARG_FLAGS     " INTEGER,\n"                         \
            "UNIQUE(" LW_TASK_DB_COL_TASK_TYPE "," LW_TASK_DB_COL_ARG_NAME "),\n" \
            "FOREIGN KEY (" LW_TASK_DB_COL_TASK_TYPE ") \n"                    \
            "    REFERENCES " LW_TASK_TYPES_TABLE " (" LW_TASK_DB_COL_TASK_TYPE "));\n" \
    "CREATE TABLE " LW_TASK_TABLE "(\n"                                        \
             LW_TASK_DB_COL_TASK_ID   " INTEGER PRIMARY KEY AUTOINCREMENT,\n"  \
             LW_TASK_DB_COL_TASK_NAME " TEXT COLLATE NOCASE,\n"                \
             LW_TASK_DB_COL_TASK_TYPE " INTEGER, \n"                           \
             LW_TASK_DB_COL_CREATED_TIME " DATE DEFAULT (DATETIME('now')),\n"  \
             "UNIQUE(" LW_TASK_DB_COL_TASK_NAME "),\n"                         \
             "FOREIGN KEY (" LW_TASK_DB_COL_TASK_TYPE ") \n"                   \
             "    REFERENCES " LW_TASK_TYPES_TABLE " (" LW_TASK_DB_COL_TASK_TYPE "));\n" \
    "CREATE TABLE " LW_TASK_ARGS_TABLE "(\n"                                   \
            LW_TASK_DB_COL_TASK_ID   " INTEGER,\n"                             \
            LW_TASK_DB_COL_ARG_TYPE  " INTEGER,\n"                             \
            LW_TASK_DB_COL_ARG_NAME  " TEXT COLLATE NOCASE,\n"                 \
            LW_TASK_DB_COL_ARG_VALUE " TEXT COLLATE NOCASE,\n"                 \
            "UNIQUE(" LW_TASK_DB_COL_TASK_ID "," LW_TASK_DB_COL_ARG_NAME "),\n"\
            "FOREIGN KEY (" LW_TASK_DB_COL_TASK_ID ") \n"                      \
            "    REFERENCES " LW_TASK_TABLE " (" LW_TASK_DB_COL_TASK_ID "));\n"\
    "CREATE TABLE " LW_TASK_STATUS_TABLE "(\n"                                 \
            LW_TASK_DB_COL_TASK_ID      " INTEGER,\n"                          \
            LW_TASK_DB_COL_START_TIME   " DATE,\n"                             \
            LW_TASK_DB_COL_END_TIME     " DATE,\n"                             \
            LW_TASK_DB_COL_CREATED_TIME " DATE DEFAULT (DATETIME('now')));\n"  \
    "CREATE TRIGGER tasks_delete \n"                                           \
    "AFTER  DELETE on " LW_TASK_TABLE "\n"                                     \
    "BEGIN\n"                                                                  \
    "  DELETE FROM " LW_TASK_ARGS_TABLE "\n"                                   \
    "  WHERE " LW_TASK_DB_COL_TASK_ID " = old." LW_TASK_DB_COL_TASK_ID ";\n"   \
    "  DELETE FROM " LW_TASK_STATUS_TABLE "\n"                                 \
    "  WHERE " LW_TASK_DB_COL_TASK_ID " = old." LW_TASK_DB_COL_TASK_ID ";\n"   \
    "END;\n"

#define BAIL_ON_LW_TASK_DB_SQLITE_ERROR(dwError, pszError)  \
    if (dwError) {                                          \
        LW_TASK_LOG_DEBUG("Sqlite3 Error (code: %u): %s",   \
                            dwError,                        \
                            LW_SAFE_LOG_STRING(pszError));  \
        dwError = LW_ERROR_SAM_DATABASE_ERROR;              \
        goto error;                                         \
    }

#define BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(dwError, pDb) \
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR(dwError, sqlite3_errmsg(pDb))

#define BAIL_ON_LW_TASK_DB_SQLITE_ERROR_STMT(dwError, pStatement) \
    BAIL_ON_LW_TASK_DB_SQLITE_ERROR_DB(dwError, sqlite3_db_handle(pStatement))
