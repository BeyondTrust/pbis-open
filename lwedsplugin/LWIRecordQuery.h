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

