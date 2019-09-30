/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samdbmisc.h
 *
 * Abstract:
 *
 *        BeyondTrust SAM DB
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
