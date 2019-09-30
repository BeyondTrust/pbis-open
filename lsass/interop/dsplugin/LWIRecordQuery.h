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

#ifndef __LWIRECORDQUERY_H__
#define __LWIRECORDQUERY_H__

#include "LWIPlugIn.h"
#include <map>

class LWIRecordQuery
{

    typedef std::map<long, PDSRECORD>           RecordRefMap;
    typedef std::map<long, PDSRECORD>::iterator RecordRefMapIter;

    typedef std::map<long, long>           AttributeRefMap;
    typedef std::map<long, long>::iterator AttributeRefMapIter;

private:
    LWIRecordQuery();
    ~LWIRecordQuery();
    LWIRecordQuery(const LWIRecordQuery& other);
    LWIRecordQuery& operator=(const LWIRecordQuery& other);

public:
    static long Initialize();
    static void Cleanup();

    static long Open(sOpenRecord* pOpenRecord, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList);
    static long Close(sCloseRecord* pCloseRecord);

    static long GetReferenceInfo(sGetRecRefInfo* pGetRecRefInfo);
    static long GetAttributeInfo(sGetRecAttribInfo* pGetRecAttribInfo);
    static long GetAttributeValueByID(sGetRecordAttributeValueByID* pGetRecAttribValueByID);
    static long GetAttributeValueByIndex(sGetRecordAttributeValueByIndex* pGetRecAttribValueByIndex);
    static long GetDataNodeString(tDataNodePtr pDataNode, char** ppszString);
    static long GetDataNodeValue(tDataNodePtr pDataNode, char** ppData, long * pLength);


    /* Update methods */
    static long SetAttributeValues(sSetAttributeValues* pSetAttributeValues);
    static long SetAttributeValue(sSetAttributeValue* pSetAttributeValue);
    static long AddAttribute(sAddAttribute* pAddAttribute);
    static long AddAttributeValue(sAddAttributeValue* pAddAttributeValue);
    static long RemoveAttribute(sRemoveAttribute* pRemoveAttribute);
    static long FlushRecord(sFlushRecord* pFlushRecord);
    
private:

    static RecordRefMap* _recordRefMap;
    static AttributeRefMap* _attributeRefMap;
};

#endif /* __LWIRECORDQUERY_H__ */

