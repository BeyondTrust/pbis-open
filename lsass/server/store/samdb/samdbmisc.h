/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        samdbmisc.h
 *
 * Abstract:
 *
 *        Likewise SAM DB
 *
 *        Misc Functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __SAMDB_MISC_H__
#define __SAMDB_MISC_H__

DWORD
SamDbComputeLMHash(
    PCSTR pszPassword,
    PBYTE pHash,
    DWORD dwHashByteLen
    );

DWORD
SamDbComputeNTHash(
    PCWSTR pwszPassword,
    PBYTE pHash,
    DWORD dwHashByteLen
    );

DWORD
SamDbGetObjectClass(
    DIRECTORY_MOD       Modifications[],
    SAMDB_OBJECT_CLASS* pObjectClass
    );

DWORD
SamDbFindObjectClassMapInfo(
    SAMDB_OBJECT_CLASS                   objectClass,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO  pMapInfos,
    DWORD                                dwNumMapInfos,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO* ppMapInfo
    );

PSAM_DB_COLUMN_VALUE
SamDbReverseColumnValueList(
    PSAM_DB_COLUMN_VALUE pColumnValueList
    );

VOID
SamDbFreeColumnValueList(
    PSAM_DB_COLUMN_VALUE pColValueList
    );

DWORD
SamDbGetNumberOfDependents_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PDWORD                 pdwNumDependents
    );

DWORD
SamDbGetObjectCount(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    SAMDB_OBJECT_CLASS     objectClass,
    PDWORD                 pdwNumObjects
    );

DWORD
SamDbGetObjectRecordInfo(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    );

DWORD
SamDbGetObjectRecordInfo_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    );

DWORD
SamDbGetObjectRecordInfoBySID(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectSID,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    );

DWORD
SamDbGetObjectRecordInfoBySID_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectSID,
    PLONG64                pllObjectRecordId,
    SAMDB_OBJECT_CLASS*    pObjectClass
    );

LONG64
SamDbGetNTTime(
    time_t timeVal
    );

DWORD
SamDbIncrementSequenceNumber(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext
    );

DWORD
SamDbIncrementSequenceNumber_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext
    );


#endif /* __SAMDB_MISC_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
