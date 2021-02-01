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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        structs.h
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
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

    LW_PIO_CREDS pKrb5Creds;

} LW_TASK_CREDS, *PLW_TASK_CREDS;

typedef struct _LW_TASK_FILE
{
    LONG           refCount;

    IO_FILE_HANDLE hFile;

} LW_TASK_FILE, *PLW_TASK_FILE;

typedef struct _LW_TASK_DIRECTORY
{
    PLW_TASK_FILE pParentRemote;
    PLW_TASK_FILE pParentLocal;

    PWSTR         pwszName;

    struct _LW_TASK_DIRECTORY* pNext;
    struct _LW_TASK_DIRECTORY* pPrev;

} LW_TASK_DIRECTORY, *PLW_TASK_DIRECTORY;

typedef struct _LW_SHARE_MIGRATION_COUNTERS
{
    ULONG64 ullNumFolders;
    ULONG64 ullNumFiles;

} LW_SHARE_MIGRATION_COUNTERS, *PLW_SHARE_MIGRATION_COUNTERS;

typedef struct _LW_SHARE_MIGRATION_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PWSTR          pwszServer;

    PLW_TASK_CREDS pRemoteCreds;
    LW_PIO_CREDS   pLocalCreds;

    LW_SHARE_MIGRATION_COUNTERS expected;
    LW_SHARE_MIGRATION_COUNTERS visited;

    PLW_TASK_DIRECTORY pHead;
    PLW_TASK_DIRECTORY pTail;

} LW_SHARE_MIGRATION_CONTEXT;

typedef struct _LW_SHARE_MIGRATION_GLOBALS
{
    BOOLEAN   bNetApiInitialized;
    PWSTR     pwszDefaultSharePath;
    wchar16_t wszRemoteDriverPrefix[32];
    wchar16_t wszDiskDriverPrefix[32];

} LW_SHARE_MIGRATION_GLOBALS, *PLW_SHARE_MIGRATION_GLOBALS;

