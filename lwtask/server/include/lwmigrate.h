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
 *        lwmigrate.h
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#ifndef __LWMIGRATE_H__
#define __LWMIGRATE_H__

typedef DWORD LW_MIGRATE_FLAGS;

#define LW_MIGRATE_FLAGS_OVERWRITE 0x00000001

typedef struct _LW_SHARE_MIGRATION_CONTEXT *PLW_SHARE_MIGRATION_CONTEXT;

DWORD
LwTaskMigrateInit(
    VOID
    );

DWORD
LwTaskMigrateCreateContext(
    PCSTR                        pszUsername,
    PCSTR                        pszPassword,
    PLW_SHARE_MIGRATION_CONTEXT* ppContext
    );

DWORD
LwTaskMigrateMultipleSharesA(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PCSTR                       pszServer,
    PCSTR                       pszRemoteShares,
    LW_MIGRATE_FLAGS            dwFlags
    );

DWORD
LwTaskMigrateShareA(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PCSTR                       pszServer,
    PCSTR                       pszShare,
    LW_MIGRATE_FLAGS            dwFlags
    );

DWORD
LwTaskMigrateShareW(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PWSTR                       pwszServer,
    PWSTR                       pwszShare,
    LW_MIGRATE_FLAGS            dwFlags
    );

VOID
LwTaskMigrateCloseContext(
    PLW_SHARE_MIGRATION_CONTEXT pContext
    );

VOID
LwTaskMigrateShutdown(
    VOID
    );

#endif /* __LWMIGRATE_H__ */

