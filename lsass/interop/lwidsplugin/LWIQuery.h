/*
 *  LWIQuery.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */
#ifndef __LWIQUERY_H__
#define __LWIQUERY_H__

#include "LWIPlugIn.h"
#include "LWIUser.h"
#include "LWIGroup.h"


class LWIQuery
{
    friend class LWIAttrValDataQuery;
    friend class LWIRecordListQuery;
    friend class LWIDirNodeQuery;
    friend class LWIRecordQuery;

protected:
    LWIQuery(bool bGetValues);
    virtual ~LWIQuery();

private:
    LWIQuery();
    LWIQuery(const LWIQuery& other);
    LWIQuery& operator=(const LWIQuery& other);

public:

    static long Create(bool bGetValues, LWIQuery** pQuery);
    void Release();

    PDSRECORD GetRecordList(bool bRemove);

protected:

    bool ShouldQueryUserInformation();
    bool ShouldQueryGroupInformation();

    static bool IsTestEnabled();

    static bool IsAllDigit(const char* pszBuf);

    static bool IsLWIGeneratedUIDForUser(const char* pszBuf);
    static bool IsLWIGeneratedUIDForGroup(const char* pszBuf);
    static long ExtractUIDFromGeneratedUID(const char* pszBuf, uid_t& uid);
    static long ExtractGIDFromGeneratedUID(const char* pszBuf, gid_t& gid);
    static long BuildGeneratedUID(uid_t uid, char** ppszUID);
    static long BuildGeneratedGID(gid_t gid, char** ppszGID);
    static long GetAuthString(IN const PLWIUSER pUser, OUT char** AuthString);
    long ProcessUserAttributes(IN OUT PDSRECORD pRecord, IN OPTIONAL const char* pszName, IN const PLWIUSER pUser);
    long ProcessGroupAttributes(IN OUT PDSRECORD pRecord, IN OPTIONAL const char* pszName, IN const PLWIGROUP pGroup);
    long QueryAllUserInformation(const char* pszName);
    long QueryAllGroupInformation(const char* pszName);
    long QueryUserInformationByName(const char* pszName);
    long QueryUserInformationById(uid_t uid);
    long QueryUserInformationByGeneratedUID(const char* pszGUID);
    long QueryGroupInformationByName(const char* pszName);
    long QueryGroupInformationById(gid_t gid);
    long QueryGroupInformationByGeneratedUID(const char* pszGUID);
    long QueryComputerListInformationByName(const char* pszName);
    long QueryComputerGroupInformationByName(const char* pszName);
    long QueryComputerInformationByName(const char* pszName);

    long QueryGroupsForUser(gid_t gid, PCSTR pszUserSid);
    long QueryGroupsForUserByName(const char* pszName);
    long QueryGroupsForUserById(uid_t uid);
    long GetUserInformationById(uid_t uid);
    long GetUserInformationByName(const char* pszName);
    long GetGroupInformationById(gid_t gid);
    long GetGroupInformationByName(const char* pszName);

    static long SetDistinguishedName(PDSRECORD pRecord, const char* pszUsername, bool bSetValue);
    static long SetGeneratedUID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetGeneratedUID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetNFSHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPassword(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPassword(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetPasswordPlus(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPasswordPolicyOptions(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPrimaryGroupID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPrimaryGroupID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetUniqueID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetUserShell(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetAuthenticationAuthority(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetGroupMembership(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetGroupMembership(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPasswordAgingPolicy(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPasswordChange(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPasswordExpire(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetMetaNodeLocation(PDSRECORD pRecord, bool bSetValue);
    static long SetTimeToLive(PDSRECORD pRecord, bool bSetValue);
    static void FreeAttributeValue(PDSATTRIBUTEVALUE pAttributeValue);
    static void FreeAttributeValueList(PDSATTRIBUTEVALUE pAttributeValueList);
    static void FreeAttribute(PDSATTRIBUTE pAttribute);
    static void FreeAttributeList(PDSATTRIBUTE pAttribute);
    static void FreeRecord(PDSRECORD pRecord);
    static void FreeRecordList(PDSRECORD pRecord);
    static void FreeMessage(PDSMESSAGE pMessage);
    static void FreeMessageHeader(PDSMESSAGEHEADER pHeader);
    static long CreateMemberList(DWORD dwMemberCount, PLSA_SECURITY_OBJECT* ppMembers, PLWIMEMBERLIST* ppMemberList);
    static void FreeMemberList(PLWIMEMBERLIST pMemberList);
    long AddUserRecordHelper(PLSA_SECURITY_OBJECT pUserObject);
    long AddUserRecord(PLWIUSER pUser, OPTIONAL const char* AltName);
    long AddGroupRecordHelper(PLSA_SECURITY_OBJECT pGroupObject);
    long AddGroupRecord(PLWIGROUP pGroup, OPTIONAL const char* AltName);
    static long BuildRecord(const char* pszType, const char* pszName, PDSRECORD* ppRecord);
    long CommitRecord(PDSRECORD pRecord);
    static long AddAttributeEx(OUT PDSATTRIBUTE* ppAttribute, IN PDSRECORD pRecord, IN const char* pszAttributeName, IN OPTIONAL const char* pszValue);
    static long AddAttribute(const char* pszAttributeName, PDSRECORD pRecord, PDSATTRIBUTE * ppAttribute);
    static long SetAttributeValue(PDSATTRIBUTE pAttribute, const char* pszValue);
    static long SetAttributeValue(PDSATTRIBUTE pAttribute, const char* pszValue, int valLen);
    static long AddAttributeAndValue(const char* pszAttributeName, const char* pszValue, PDSRECORD pRecord, PDSATTRIBUTE* ppAttribute);
    static long AddAttributeAndValue(const char* pszAttributeName, const char* pszValue, int valLen, PDSRECORD pRecord, PDSATTRIBUTE* ppAttribute);
    long WriteResponse(char* buffer, unsigned long maxBufferSize, unsigned long& bytesWritten, unsigned long& nRecordsWritten, uint32_t headerType = 'StdA');
    long WriteGDNIResponse(char* buffer, unsigned long maxBufferSize, unsigned long& bytesWritten, unsigned long& nRecordsWritten);
    static long ReadResponseAttributeValue(char* buffer, unsigned long  bufferSize, unsigned long offset, PDSATTRIBUTEVALUE* ppAttributeValue);
    static long ReadResponseAttribute(char* buffer, unsigned long  bufferSize, unsigned long  offset, PDSATTRIBUTE* ppAttribute);
    static long ReadResponseRecord(char* buffer, unsigned long  bufferSize, unsigned long  offset, PDSRECORD* ppRecord);
    static long ReadResponseRecords(char* buffer, unsigned long  bufferSize, PDSMESSAGEHEADER pHeader, PDSRECORD* ppRecordList);
    static long ReadResponseHeader(char* buffer, unsigned long  bufferSize, PDSMESSAGEHEADER * ppHeader);
    static long ReadResponse(char* buffer, unsigned long  bufferSize, PDSMESSAGE* ppMessage);
    static long FindAttributeByType(PDSRECORD pRecord, const char* pszAttrType, PDSATTRIBUTE* ppAttribute);
    static long FindAttributeValueByID(PDSATTRIBUTE pAttribute, unsigned long attrValueID, PDSATTRIBUTEVALUE* ppAttributeValue);
    static long FindAttributeValueByIndex(PDSATTRIBUTE pAttribute, unsigned long attrValIndex, PDSATTRIBUTEVALUE* ppAttributeValue);
    static long CreateAttributeEntry(OUT tAttributeEntryPtr* ppAttributeEntry, IN PDSATTRIBUTE pAttribute);
    static long CreateAttributeValueEntry(OUT tAttributeValueEntryPtr* ppAttributeValueEntry, IN PDSATTRIBUTEVALUE pAttributeValue);
    static long GetNumberOfAttributes(PDSRECORD pRecord, uint16_t& nAttributes);
    static long GetNumberOfAttributeValues(PDSATTRIBUTE pAttribute, uint16_t& nValues);
    long DetermineRecordsToFitInBuffer(unsigned long maxBufferSize, int& nRecords);
    static unsigned int GetHeaderSize(int nRecords);
    static long GetAttributeSize(PDSATTRIBUTE pAttribute, uint32_t& attrSize);
    static long GetRecordSize(PDSRECORD pRecord, uint32_t& recordSize);
    static long WriteHeader(char* buffer, int nRecords, unsigned long& offset, uint32_t headerType);
    static long WriteRecord(char* buffer, int iRecord, PDSRECORD pRecord, unsigned long& offset);
    static long WriteAttribute(char* buffer, PDSATTRIBUTE pAttribute, unsigned long& offset);
    static long WriteAttributeValue(char* buffer, PDSATTRIBUTEVALUE pAttributeValue, unsigned long& offset);

protected:
    bool _bGetValues;

    PDSRECORD _pRecordListHead;
    PDSRECORD _pRecordListTail;
    PDSRECORD _pCurrentRecord;

    PLWIBITVECTOR _recTypeSet;
    PLWIBITVECTOR _attributeSet;

    static uint32_t DEFAULT_ATTRIBUTE_TTL_SECONDS;
};

#endif /* __LWIQUERY_H__ */

