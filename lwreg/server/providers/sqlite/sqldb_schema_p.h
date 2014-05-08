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
 *        sqldb_schema_p.h
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Private functions in sqlite3 Caching backend
 *        With Value Attributes Schema process
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#ifndef SQLDB_SCHEMA_P_H_
#define SQLDB_SCHEMA_P_H_


NTSTATUS
RegDbUnpackRegValueAttributesInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_DB_VALUE_ATTRIBUTES pResult
    );

void
RegDbSafeFreeEntryValueAttributes(
    PREG_DB_VALUE_ATTRIBUTES* ppEntry
    );

void
RegDbSafeFreeEntryValueAttributesList(
    size_t sCount,
    PREG_DB_VALUE_ATTRIBUTES** pppEntries
    );

NTSTATUS
RegDbStoreRegValueAttributes(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppValueAttributes
    );

NTSTATUS
RegDbUpdateRegValueAttributes(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE_ATTRIBUTES* ppValueAttributes
    );

NTSTATUS
RegDbCreateValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );

NTSTATUS
RegDbGetValueAttributes_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES* ppRegEntry
    );

NTSTATUS
RegDbQueryDefaultValuesCount_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwKeyId,
    OUT size_t* psCount
    );

NTSTATUS
RegDbQueryDefaultValuesCount(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwKeyId,
    OUT size_t* psCount
    );

NTSTATUS
RegDbQueryDefaultValues(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES** pppRegEntries
    );

NTSTATUS
RegDbQueryDefaultValues_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES** pppRegEntries
    );

NTSTATUS
RegDbGetValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE_ATTRIBUTES* ppRegEntry
    );

NTSTATUS
RegDbSetValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );

NTSTATUS
RegDbDeleteValueAttributes(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName
    );



#endif /* SQLDB_SCHEMA_P_H_ */
