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

