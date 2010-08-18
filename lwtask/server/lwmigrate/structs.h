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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        Structure Definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

typedef struct _LW_TASK_CREDS
{
    krb5_context ctx;
    krb5_ccache  cc;
    PSTR         pszRestoreCache;

    LW_PIO_CREDS pRestoreCreds;

} LW_TASK_CREDS, *PLW_TASK_CREDS;

typedef struct _LW_FILE_ITEM
{
    BOOLEAN bIsDir;

    PWSTR   pwszRemotePath;
    PWSTR   pwszLocalPath;

    struct _LW_FILE_ITEM* pNext;
    struct _LW_FILE_ITEM* pPrev;

} LW_FILE_ITEM, *PLW_FILE_ITEM;

typedef struct _LW_SHARE_MIGRATION_CONTEXT
{

    PLW_FILE_ITEM pHead;
    PLW_FILE_ITEM pTail;

} LW_SHARE_MIGRATION_CONTEXT, *PLW_SHARE_MIGRATION_CONTEXT;

