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

#ifndef __LWIDIRNODEQUERY_H__
#define __LWIDIRNODEQUERY_H__

#include "LWIQuery.h"
#include "LWICRC.h"
#include <map>

void
FreeDirNode(
    PDSDIRNODE pDirNode
    );

class LWIDirNodeQuery
{
    typedef std::map<long, PDSDIRNODE>           DirNodeRefMap;
    typedef std::map<long, PDSDIRNODE>::iterator DirNodeRefMapIter;

    typedef std::map<long, long>           AttributeRefMap;
    typedef std::map<long, long>::iterator AttributeRefMapIter;

private:
    LWIDirNodeQuery();
    ~LWIDirNodeQuery();
    LWIDirNodeQuery(const LWIDirNodeQuery& other);
    LWIDirNodeQuery& operator=(const LWIDirNodeQuery& other);

public:
    static long Initialize();
    static void Cleanup();

    static long Open(sOpenDirNode * pOpenDirNode);
    static long DoDirNodeAuth(sDoDirNodeAuth* pDirNodeAuth, bool fIsJoined, PVOID pAllowAdminCheckData, LWE_DS_FLAGS Flags);
    static long Close(sCloseDirNode * pCloseDirNode);
    static long GetInfo(sGetDirNodeInfo * pGetDirNodeInfo, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList);
    static long GetAttributeEntry(sGetAttributeEntry * pData);
    static long GetAttributeValue(sGetAttributeValue * pData);
    static long CloseValueList(sCloseAttributeValueList * pData);
    static long CloseAttributeList(sCloseAttributeList * pData);
    
    /* Helper function for LWIRecordListQuery and others to retrieve directory node information */
    static long GetDsDirNodeRef(long dirNodeRef, PDSDIRNODE* ppDirNode);

protected:

    static long FindAttribute(PDSMESSAGE pMessage, unsigned long attrIndex, PDSATTRIBUTE* ppAttribute);
    static long GetAttributeInfo(PDSMESSAGE pMessage, unsigned long attrIndex, tAttributeEntryPtr* ppAttributeEntryPtr);

private:
    static DirNodeRefMap*   _dirNodeRefMap;
    static AttributeRefMap* _attributeRefMap;
};

#endif /* __LWIDIRNODEQUERY_H__ */

