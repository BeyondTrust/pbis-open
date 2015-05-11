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

#ifndef __LWIQUERY_H__
#define __LWIQUERY_H__

#include "LWIPlugIn.h"
#include "LWIUser.h"
#include "LWIGroup.h"
#include "LWIComputer.h"
#include "LWIComputerList.h"
#include "LWIComputerGroup.h"

long CopyMCXValueList(PMCXVALUE pValueList, PMCXVALUE* pValueListCopy);

	
class LWIQuery
{
    friend class LWIAttrValDataQuery;
    friend class LWIRecordListQuery;
    friend class LWIDirNodeQuery;
    friend class LWIRecordQuery;

protected:
    LWIQuery(bool bGetValues, bool bAllowIOContinue, long dirNodeRef, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList, PDSDIRNODE pDirNode);
    virtual ~LWIQuery();

private:
    LWIQuery();
    LWIQuery(const LWIQuery& other);
    LWIQuery& operator=(const LWIQuery& other);

public:

    static long Create(bool bGetValues, bool bAllowIOContinue, long dirNodeRef, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList, LWIQuery** pQuery);
    void Release();

    PDSRECORD GetRecordList(bool bRemove);
    long GetDirNodeRef();
    static void SetCacheLifeTime(DWORD dwCacheLifeTime);

protected:

    bool ShouldQueryUserInformation();
    bool ShouldQueryGroupInformation();
    bool ShouldQueryComputerListInformation();
    bool ShouldQueryComputerGroupInformation();
    bool ShouldQueryComputerInformation();

    static bool IsTestEnabled();
    static bool IsAllDigit(const char* pszBuf);

    static bool IsLWIGeneratedUIDForUser(const char* pszBuf);
    static bool IsLWIGeneratedUIDForGroup(const char* pszBuf);
    static long ExtractUIDFromGeneratedUID(const char* pszBuf, uid_t& uid);
    static long ExtractGIDFromGeneratedUID(const char* pszBuf, gid_t& gid);
    static long BuildGeneratedUID(uid_t uid, char** ppszUID);
    static long BuildGeneratedGID(gid_t gid, char** ppszGID);

    long ProcessUserAttributes(PDSRECORD pRecord, OPTIONAL const char* pszName, const PLWIUSER pUser);
    long ProcessGroupAttributes(PDSRECORD pRecord, OPTIONAL const char* pszName, const PLWIGROUP pGroup);
    long ProcessComputerAttributes(PDSRECORD pRecord, const PLWICOMPUTER pComputer);
    long ProcessComputerListAttributes(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList);
    long ProcessComputerGroupAttributes(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup);
    long QueryAllUserInformation(const char* pszName);
    long QueryAllGroupInformation(const char* pszName);
    long GetGPOComputerList(void);
    long QueryAllComputerListInformation(const char* pszName);
    long GetGPOComputerGroups(void);
    long QueryAllComputerGroupInformation(const char* pszName);
    long QueryAllComputerInformation(const char* pszName);
    long QueryUserInformationByName(const char* pszName);
    long QueryUserInformationById(uid_t uid);
    long QueryUserInformationByGeneratedUID(const char* pszGUID);
    long QueryUserInformationByPrimaryGroupID(const char* pszPrimaryGID);
    long QueryGroupInformationByName(const char* pszName);
    long QueryGroupInformationById(gid_t gid);
    long QueryGroupInformationByGeneratedUID(const char* pszGUID);
    long QueryComputerListInformationByName(const char* pszName);
    long QueryComputerGroupInformationByName(const char* pszName);
    long QueryComputerInformationByName(const char* pszName);
    long QueryComputerInformationByENetAddress(const char* pszENetAddress);
    long QueryComputerInformationByIPAddress(const char* pszIPAddress);
    long QueryComputerInformationByGeneratedUID(const char* pszGeneratedUID);
#if 0 /* Design change - See bugzilla 7388 */
    long GetGPOGroupsForUser(PLWIUSER UserInfo);
#endif
    long GetGPOGroupMCXSettingsForUser(const char* pszName, uid_t uid, PMCXVALUE * ppMCXValueList, LWE_DS_FLAGS Flags);
    long GetHomeDirectoryProtocolXmlAndMountPath(uid_t uid, PSTR pszHomeDirectory, LWE_DS_FLAGS Flags, PSTR * ppszHomeDirectoryXML, PSTR * ppszHomeDirectoryMount);
    long GetUserHomeFolderSettings(uid_t uid, PSTR pszHomeDirectory, LWE_DS_FLAGS Flags, PSTR * ppszNFSHomeDirectory, PSTR * ppszHomeDirectory, PSTR * ppszOriginalNFSHomeDirectory, PSTR * ppszOriginalHomeDirectory);
    long QueryGroupsForUser(gid_t gid, PCSTR pszUserSid);
    long QueryGroupsForUserByName(const char* pszName);
    long QueryGroupsForUserById(uid_t uid);
    long GetUserInformationById(uid_t uid);
    long GetUserInformationByName(const char* pszName);
    long GetGroupInformationById(gid_t gid);
    long GetGroupInformationByName(const char* pszName);
    long GetComputerListByName(const char* pszName, PLWICOMPUTERLIST* ppComputerList);
    long GetComputerGroupByName(const char* pszName, PLWICOMPUTERGROUP* ppComputerGroup);
    long GetComputerByName(const char* pszName, PLWICOMPUTER* ppComputer);
    long GetComputerByENetAddress(const char* pszENetAddress, PLWICOMPUTER* ppComputer);
    long GetComputerByIPAddress(const char* pszIPAddress, PLWICOMPUTER* ppComputer);
    long GetComputerByGeneratedUID(const char* pszGeneratedUID, PLWICOMPUTER* ppComputer);
    static long SetDistinguishedName(PDSRECORD pRecord, const char* pszName, const char* pszNameAsQueried, bool bSetValue);
    static long SetGeneratedUID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetGeneratedUID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetGeneratedUID(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue);
    static long SetGeneratedUID(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetGeneratedUID(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetNFSHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetOriginalNFSHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetOriginalHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPassword(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPassword(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetPasswordPlus(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPasswordPolicyOptions(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPrimaryGroupID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPrimaryGroupID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetPrimaryGroupID(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetPrimaryGroupID(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetUniqueID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetUniqueID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetUserShell(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetGroupMembership(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetGroupMembership(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetGroupMembership(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetGroupMembers(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue);
    static long SetGroupMembers(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetPasswordAgingPolicy(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPasswordChange(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetPasswordExpire(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetMetaNodeLocation(PDSRECORD pRecord, PSTR pszPath, bool bSetValue);
    static long SetTimeToLive(PDSRECORD pRecord, bool bSetValue);
    static long SetMCXFlags(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetMCXFlags(PDSRECORD pRecord, PLWIGROUP pGroup, bool bSetValue);
    static long SetMCXFlags(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetMCXFlags(PDSRECORD pRecord, PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetMCXFlags(PDSRECORD pRecord, PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetMCXSettings(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetMCXSettings(PDSRECORD pRecord, PLWIGROUP pGroup, bool bSetValue);
    static long SetMCXSettings(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetMCXSettings(PDSRECORD pRecord, PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetMCXSettings(PDSRECORD pRecord, PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetComment(PDSRECORD pRecord, PLWIGROUP pGroup, bool bSetValue);
    static long SetComment(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetComment(PDSRECORD pRecord, PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetComment(PDSRECORD pRecord, PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetComputers(PDSRECORD pRecord, PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetComputers(PDSRECORD pRecord, PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetShortName(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetShortName(PDSRECORD pRecord, PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetShortName(PDSRECORD pRecord, PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetRealName(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetRealName(PDSRECORD pRecord, PLWICOMPUTERLIST pComputerList, bool bSetValue);
    static long SetRealName(PDSRECORD pRecord, PLWICOMPUTERGROUP pComputerGroup, bool bSetValue);
    static long SetENetAddress(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetIPAddress(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetKeywords(PDSRECORD pRecord, PLWICOMPUTER pComputer, bool bSetValue);
    static long SetFirstName(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetLastName(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetDomain(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetKerberosPrincipal(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue);
    static long SetEMail(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetPhone(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetAddress(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetWork(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetProfile(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetLogon(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
    static long SetPolicy(PDSRECORD pRecord, PLWIUSER pUser, bool bSetValue);
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
    long AddUserRecordHelper(PLSA_SECURITY_OBJECT pUserObject, OPTIONAL const char* pszNameAsQueried);
    long AddUserRecord(PLWIUSER pUser, OPTIONAL const char* AltName);
    long AddGroupRecordHelper(PLSA_SECURITY_OBJECT pGroupObject, bool bExpandMembers, OPTIONAL const char* pszNameAsQueried);
    long AddGroupRecord(PLWIGROUP pGroup, OPTIONAL const char* AltName);
    long AddComputerRecord(PLWICOMPUTER pComputer);
    long AddComputerListRecord(PLWICOMPUTERLIST pComputerList);
    long AddComputerGroupRecord(PLWICOMPUTERGROUP pComputerGroup);
    static long BuildRecord(const char* pszType, const char* pszName, long dirNodeRef, PDSRECORD* ppRecord);
    long CommitRecord(PDSRECORD pRecord);
    static long AddAttributeEx(PDSATTRIBUTE* ppAttribute, PDSRECORD pRecord, const char* pszAttributeName, OPTIONAL const char* pszValue);
    static long AddAttribute(const char* pszAttributeName, PDSRECORD pRecord, PDSATTRIBUTE * ppAttribute);
    static long SetAttributeValue(PDSATTRIBUTE pAttribute, const char* pszValue);
    static long SetAttributeValue(PDSATTRIBUTE pAttribute, const char* pszValue, int valLen);
    static long AddAttributeAndValue(const char* pszAttributeName, const char* pszValue, PDSRECORD pRecord, PDSATTRIBUTE* ppAttribute);
    static long AddAttributeAndValue(const char* pszAttributeName, const char* pszValue, int valLen, PDSRECORD pRecord, PDSATTRIBUTE* ppAttribute);
    static void RemoveAttributeAndValues(const char* pszAttributeName, PDSRECORD pRecord);
    long WriteResponse(char* buffer, unsigned long maxBufferSize, unsigned long& bytesWritten, unsigned long& nRecordsWritten, unsigned long& TotalRecords, uint32_t headerType = 'StdA');
    long WriteGDNIResponse(char* buffer, unsigned long maxBufferSize, unsigned long& bytesWritten, unsigned long& nRecordsWritten, unsigned long& TotalRecords);
    static long ReadResponseAttributeValue(char* buffer, unsigned long  bufferSize, unsigned long offset, PDSATTRIBUTEVALUE* ppAttributeValue);
    static long ReadResponseAttribute(char* buffer, unsigned long  bufferSize, unsigned long  offset, PDSATTRIBUTE* ppAttribute);
    static long ReadResponseRecord(char* buffer, unsigned long  bufferSize, unsigned long  offset, PDSRECORD* ppRecord);
    static long ReadResponseRecords(char* buffer, unsigned long  bufferSize, PDSMESSAGEHEADER pHeader, PDSRECORD* ppRecordList);
    static long ReadResponseHeader(char* buffer, unsigned long  bufferSize, PDSMESSAGEHEADER * ppHeader);
    static long ReadResponse(char* buffer, unsigned long  bufferSize, PDSMESSAGE* ppMessage);
    static long FindAttributeByType(PDSRECORD pRecord, const char* pszAttrType, PDSATTRIBUTE* ppAttribute);
    static long FindAttributeValueByID(PDSATTRIBUTE pAttribute, unsigned long attrValueID, PDSATTRIBUTEVALUE* ppAttributeValue);
    static long FindAttributeValueByIndex(PDSATTRIBUTE pAttribute, unsigned long attrValIndex, PDSATTRIBUTEVALUE* ppAttributeValue);
    static long CreateAttributeEntry(tAttributeEntryPtr* ppAttributeEntry, PDSATTRIBUTE pAttribute);
    static long CreateAttributeValueEntry(tAttributeValueEntryPtr* ppAttributeValueEntry, PDSATTRIBUTEVALUE pAttributeValue);
    static long GetNumberOfAttributes(PDSRECORD pRecord, uint16_t& nAttributes);
    static long GetNumberOfAttributeValues(PDSATTRIBUTE pAttribute, uint16_t& nValues);
    long DetermineRecordsToFitInBuffer(unsigned long maxBufferSize, int& nRecords, int& TotalRecords);
    static unsigned int GetHeaderSize(int nRecords);
    static long GetAttributeSize(PDSATTRIBUTE pAttribute, uint32_t& attrSize);
    static long GetRecordSize(PDSRECORD pRecord, uint32_t& recordSize);
    static long WriteHeader(char* buffer, int nRecords, unsigned long& offset, uint32_t headerType);
    static long WriteRecord(char* buffer, int iRecord, PDSRECORD pRecord, unsigned long& offset);
    static long WriteAttribute(char* buffer, PDSATTRIBUTE pAttribute, unsigned long& offset);
    static long WriteAttributeValue(char* buffer, PDSATTRIBUTEVALUE pAttributeValue, unsigned long& offset);

protected:
    bool _bGetValues;
    bool _bAllowIOContinue;
    long _dirNodeRef;
    LWE_DS_FLAGS _dwFlags;
    PNETADAPTERINFO _pNetAdapterList;
    PDSDIRNODE _pDirNode;

    PDSRECORD _pRecordListHead;
    PDSRECORD _pRecordListTail;
    PDSRECORD _pCurrentRecord;

    bool _bRespondedWithTooSmallError;
    unsigned long _lastResponseBufferSize;

    PLWIBITVECTOR _recTypeSet;
    PLWIBITVECTOR _attributeSet;

    static uint32_t DEFAULT_ATTRIBUTE_TTL_SECONDS;
};

//
// IO response buffer continuation cache
//
typedef struct __QUERYCONTEXT
{
    tContextData           HandleId;
    LWIQuery*              pQuery;
    struct __QUERYCONTEXT* pNext;
} QUERYCONTEXT, *PQUERYCONTEXT;

long
InitializeContextList(
    );

void
UninitializeContextList(
    );

long
AddQueryToContextList(
    LWIQuery*     pQuery,
    tContextData* pHandleId
    );

long
GetQueryFromContextList(
    tContextData HandleId,
    LWIQuery**   ppQuery
    );

//
// Global cache context variables
//
extern PQUERYCONTEXT   Global_ContextList;
extern tContextData    Global_LastHandleId;
extern pthread_mutex_t Global_ContextListMutexLock;
extern BOOLEAN         Global_ContextListMutexLockInitialized;

#endif /* __LWIQUERY_H__ */

