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

#ifndef __LWIRECORDLISTQUERY_H__
#define __LWIRECORDLISTQUERY_H__

#include "LWIQuery.h"

class LWIRecordListQuery : public LWIQuery
{
private:
    LWIRecordListQuery();
    ~LWIRecordListQuery();
    LWIRecordListQuery(const LWIRecordListQuery& other);
    LWIRecordListQuery& operator=(const LWIRecordListQuery& other);

public:
    static long Run(IN OUT sGetRecordList* pGetRecordList, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList);
    static long ReleaseContinueData(IN OUT sReleaseContinueData* pReleaseContinueData);
    static long GetRecordEntry(sGetRecordEntry* pGetRecordEntry);
    static long Test(IN const char* DsPath, IN sGetRecordList* pGetRecordList);

protected:
    static long Test(IN const char* DsPath, IN tDataListPtr RecNameList, IN tDirPatternMatch PatternMatch,
                     IN tDataListPtr RecTypeList, IN tDataListPtr AttribTypeList, IN dsBool AttribInfoOnly,
                     IN unsigned long Size);
};

#endif /* __LWIRECORDLISTQUERY_H__ */


