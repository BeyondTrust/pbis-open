/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
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

