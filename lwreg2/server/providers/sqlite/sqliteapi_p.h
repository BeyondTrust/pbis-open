/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        sqliteapi_p.h
 *
 * Abstract:
 *
 *        Registry sqlite provider (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef SQLITEAPI_P_H_
#define SQLITEAPI_P_H_

NTSTATUS
SqliteGetKeyToken(
    PCWSTR pwszInputString,
    wchar16_t c,
    PWSTR *ppwszOutputString
    );

NTSTATUS
SqliteGetParentKeyName(
    PCWSTR pwszInputString,
    wchar16_t c,
    PWSTR *ppwszOutputString
    );

NTSTATUS
SqliteCreateKeyHandle(
    IN ACCESS_MASK samGranted,
    IN PREG_KEY_CONTEXT pKey,
    OUT PREG_KEY_HANDLE* ppKeyHandle
    );

NTSTATUS
SqliteCreateKeyContext(
    IN ACCESS_MASK samGranted,
    IN PREG_DB_KEY pRegEntry,
    OUT PREG_KEY_CONTEXT* ppKeyResult
    );

NTSTATUS
SqliteCreateKeyInternal(
    IN OPTIONAL HANDLE handle,
    IN PREG_KEY_CONTEXT pParentKeyCtx,
    IN PWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle,
    OUT OPTIONAL PDWORD pdwDisposition
    );

NTSTATUS
SqliteOpenKeyInternal(
	IN HANDLE handle,
    IN PCWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
    );

NTSTATUS
SqliteOpenKeyInternal_inlock(
	IN OPTIONAL HANDLE handle,
    IN PCWSTR pwszFullKeyName, // Full Key Path
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
    );

NTSTATUS
SqliteOpenKeyInternal_inlock_inDblock(
	IN OPTIONAL HANDLE handle,
	IN PCWSTR pwszFullKeyName, // Full Key Path
	IN ACCESS_MASK AccessDesired,
	OUT OPTIONAL PREG_KEY_HANDLE* ppKeyHandle
	);

VOID
SqliteCloseKey_inlock(
    IN HKEY hKey
    );

NTSTATUS
SqliteDeleteKeyInternal(
	IN HANDLE handle,
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteDeleteKeyInternal_inlock_inDblock(
	IN HANDLE handle,
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteDeleteActiveKey(
    IN PCWSTR pwszKeyName
    );

NTSTATUS
SqliteDeleteActiveKey_inlock(
    IN PCWSTR pwszKeyName
    );

#endif /* SQLITEAPI_P_H_ */
