#ifndef __LWIDIRNODEQUERY_H__
#define __LWIDIRNODEQUERY_H__

#include "LWIQuery.h"
#include "LWICRC.h"
#include <map>

class LWIDirNodeQuery
{
private:
    LWIDirNodeQuery();
    ~LWIDirNodeQuery();
    LWIDirNodeQuery(const LWIDirNodeQuery& other);
    LWIDirNodeQuery& operator=(const LWIDirNodeQuery& other);

public:
    static long Initialize();
    static void Cleanup();

    static long Open(sOpenDirNode * pOpenDirNode);
    static long Close(sCloseDirNode * pCloseDirNode);
    static long GetInfo(sGetDirNodeInfo * pGetDirNodeInfo);
    static long GetAttributeEntry(sGetAttributeEntry * pData);
    static long GetAttributeValue(sGetAttributeValue * pData);
    static long CloseValueList(sCloseAttributeValueList * pData);
    static long CloseAttributeList(sCloseAttributeList * pData);

protected:

    static long FindAttribute(PDSMESSAGE pMessage, unsigned long attrIndex, PDSATTRIBUTE* ppAttribute);
    static long GetAttributeInfo(PDSMESSAGE pMessage, unsigned long attrIndex, tAttributeEntryPtr* ppAttributeEntryPtr);

private:
    static std::map<long, long> * _attributeRefMap;
};

#endif /* __LWIDIRNODEQUERY_H__ */

