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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        Function Prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// config.c

DWORD
LwTaskGetDefaultSharePathW(
    PWSTR* ppwszFileSystemRoot
    );

// fileitem.c

DWORD
LwTaskCreateFile(
    PLW_TASK_FILE* ppFile
    );

PLW_TASK_FILE
LwTaskAcquireFile(
    PLW_TASK_FILE pFile
    );

VOID
LwTaskReleaseFile(
    PLW_TASK_FILE pFile
    );

DWORD
LwTaskCreateDirectory(
    PWSTR               pwszDirname,
    PLW_TASK_FILE       pParentRemote,
    PLW_TASK_FILE       pParentLocal,
    PLW_TASK_DIRECTORY* ppFileItem
    );

VOID
LwTaskFreeDirectoryList(
    PLW_TASK_DIRECTORY  pFileItem
    );

// krb5.c

DWORD
LwTaskAcquireCredsW(
    PWSTR           pwszUsername,  /* IN     */
    PWSTR           pwszPassword,  /* IN     */
    PLW_TASK_CREDS* ppCreds        /* IN OUT */
    );

DWORD
LwTaskAcquireCredsA(
    PCSTR           pszUsername,  /* IN     */
    PCSTR           pszPassword,  /* IN     */
    PLW_TASK_CREDS* ppCreds       /* IN OUT */
    );

VOID
LwTaskFreeCreds(
    PLW_TASK_CREDS pCreds         /* IN OUT */
    );

// migrate.c

DWORD
LwTaskMigrateOpenRemoteShare(
    PWSTR           pwszServer,
    PWSTR           pwszShare,
    PIO_FILE_HANDLE phFileRemote
    );

DWORD
LwTaskMigrateCreateShare(
    PSHARE_INFO_502 pShareInfoRemote,
    BOOLEAN         bAddShare,
    PIO_FILE_HANDLE phShare
    );

DWORD
LwTaskMigrateShareEx(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_FILE               pRemoteFile,
    PLW_TASK_FILE               pLocalFile,
    LW_MIGRATE_FLAGS            dwFlags
    );

// path.c

DWORD
LwTaskGetLocalSharePathW(
	PWSTR  pwszSharePathWindows,
	PWSTR* ppwszSharePathLocal
	);

DWORD
LwTaskGetMappedSharePathW(
    PWSTR  pwszDriverPrefix,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    );
