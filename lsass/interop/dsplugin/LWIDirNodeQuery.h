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

