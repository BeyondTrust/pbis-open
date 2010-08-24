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
 *        Structure Definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

typedef struct _LW_TASK_DB_CONTEXT
{

    sqlite3* pDbHandle;

    sqlite3_stmt* pQueryTaskTypeCountStmt;
    sqlite3_stmt* pQueryTaskTypes;

    sqlite3_stmt* pQueryTaskSchemaArgCountStmt;
    sqlite3_stmt* pQueryTaskSchemaArgs;

    sqlite3_stmt* pQueryTaskCountStmt;
    sqlite3_stmt* pQueryTasks;

    sqlite3_stmt* pQueryTaskArgCountStmt;
    sqlite3_stmt* pQueryTaskArgs;

    sqlite3_stmt* pDelTaskStmt;

    sqlite3_stmt* pInsertTaskStmt;
    sqlite3_stmt* pInsertTaskArgStmt;

    sqlite3_stmt* pQueryTaskId;

} LW_TASK_DB_CONTEXT;

typedef struct _LW_TASK_DB_GLOBALS
{
    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

} LW_TASK_DB_GLOBALS, *PLW_TASK_DB_GLOBALS;

