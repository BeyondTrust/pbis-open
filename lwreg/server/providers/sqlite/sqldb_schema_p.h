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
 *        sqldb_schema_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Registry
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
