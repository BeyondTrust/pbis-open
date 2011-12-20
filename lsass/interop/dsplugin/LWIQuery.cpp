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

#include "includes.h"
#include "LWIDirNodeQuery.h"
#include "LWIQuery.h"
#include "LWICRC.h"
#include <sys/stat.h>


#define ENABLE_COMPUTER_LIST_SUPPORT 1

#ifndef kDSStdRecordTypeComputerGroups /* For when building on Tiger based build machines */
/*!
 * @defined kDSStdRecordTypeComputerGroups
 * @discussion Identifies computer group records.
 */
#define kDSStdRecordTypeComputerGroups "dsRecTypeStandard:ComputerGroups"
#endif


// ISSUE-2007/06/07 -- The '16' is a magic padding value.  It is not clear why it is used.
// We used it because the Apple plug-ins do it too.  Further investigation would be good.
#define MAGIC_PADDING 16

#define CHECK_INVALID_OFFSET(offset, bufsize)    \
        if (offset > bufsize) {                  \
           macError = eDSInvalidBuffFormat;      \
           GOTO_CLEANUP_ON_MACERROR(macError);   \
        }

uint32_t LWIQuery::DEFAULT_ATTRIBUTE_TTL_SECONDS = 10; // Cache values for 10 seconds

//
// Global cache context variables
//
PQUERYCONTEXT   Global_ContextList = NULL;
tContextData    Global_LastHandleId = 0;
pthread_mutex_t Global_ContextListMutexLock;
BOOLEAN         Global_ContextListMutexLockInitialized = FALSE;

LWIQuery::LWIQuery(bool bGetValues, bool bAllowIOContinue, long dirNodeRef, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList, PDSDIRNODE pDirNode)
    : _bGetValues(bGetValues),
      _bAllowIOContinue(bAllowIOContinue),
      _dirNodeRef(dirNodeRef),
      _dwFlags(Flags),
      _pNetAdapterList(pNetAdapterList),
      _pDirNode(pDirNode),
      _pRecordListHead(NULL),
      _pRecordListTail(NULL),
      _pCurrentRecord(NULL),
      _bRespondedWithTooSmallError(false),
      _lastResponseBufferSize(0),
      _recTypeSet(NULL),
      _attributeSet(NULL)
{
}

LWIQuery::~LWIQuery()
{
    FreeRecordList(_pRecordListHead);

    if (_recTypeSet)
    {
        LWIFreeBitVector(_recTypeSet);
    }

    if (_attributeSet)
    {
        LWIFreeBitVector(_attributeSet);
    }
}

long
CopyMCXValueList(
    PMCXVALUE pValueList,
    PMCXVALUE* ppValueListCopy
    )
{
    long macError = eDSNoErr;
    PMCXVALUE pValueListNew = NULL;
    PMCXVALUE pPrev = NULL;

    while (pValueList)
    {
        PMCXVALUE pNew = NULL;

        macError = LwAllocateMemory(sizeof(MCXVALUE), (PVOID*) &pNew);
        GOTO_CLEANUP_ON_MACERROR(macError);

        if (pValueList->iValLen)
        {
            macError = LwAllocateMemory(pValueList->iValLen, (PVOID*)&pNew->pValueData);
            GOTO_CLEANUP_ON_MACERROR(macError);

            memcpy(pNew->pValueData, pValueList->pValueData, pValueList->iValLen);
            pNew->iValLen = pValueList->iValLen;
        }

        if (pPrev)
        {
            pPrev->pNext = pNew;
        }
        else
        {
            pValueListNew = pNew;
        }

        pPrev = pNew;
        pNew = NULL;

        pValueList = pValueList->pNext;
    }

    *ppValueListCopy = pValueListNew;
    pValueListNew = NULL;

cleanup:

    FreeMCXValueList(pValueListNew);

    return macError;
}

long
LWIQuery::Create(
    bool bGetValues,
    bool bAllowIOContinue,
    long dirNodeRef,
    LWE_DS_FLAGS Flags,
    PNETADAPTERINFO pNetAdapterList,
    OUT LWIQuery** ppQuery)
{
    long macError = eDSNoErr;
    PDSDIRNODE pDirNode = NULL;
    LWIQuery* pQuery = NULL;

    macError = LWIDirNodeQuery::GetDsDirNodeRef(dirNodeRef, &pDirNode);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!pDirNode)
    {
        LOG_ERROR("Invalid directory node reference: %u", dirNodeRef);
        macError = eDSInvalidNodeRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pQuery = new LWIQuery(bGetValues, bAllowIOContinue, dirNodeRef, Flags, pNetAdapterList, pDirNode);
    if (!pQuery)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    LOG("LWIQuery::Create() DirNode: %s(%s), User:%s",
        pDirNode->fPlugInRootConnection ? "[Root]" : "",
        pDirNode->pszDirNodePath ? pDirNode->pszDirNodePath : "<Unknown>",
        pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "<Not authenticated>");
    
    pDirNode = NULL;
    
    *ppQuery = pQuery;
    pQuery = NULL;

cleanup:
    
    if (pQuery)
    {
        delete pQuery;
    }

    return macError;
}

void
LWIQuery::Release()
{
    delete this;
}

PDSRECORD
LWIQuery::GetRecordList(bool bRemove)
{
    PDSRECORD pResult = _pRecordListHead;

    if (bRemove)
    {
        _pRecordListHead = NULL;
        _pRecordListTail = NULL;
    }

    return pResult;
}

long
LWIQuery::GetDirNodeRef(void)
{
    return _dirNodeRef;
}

bool
LWIQuery::ShouldQueryUserInformation()
{
    /* We support user queries for all our DS nodes */
    return (_recTypeSet &&
            LWI_BITVECTOR_ISSET(_recTypeSet, LWIRecTypeLookup::idx_kDSStdRecordTypeUsers));
}

bool
LWIQuery::ShouldQueryGroupInformation()
{
    /* We support user group queries for all our DS nodes */
    return (_recTypeSet &&
            LWI_BITVECTOR_ISSET(_recTypeSet, LWIRecTypeLookup::idx_kDSStdRecordTypeGroups));
}

bool
LWIQuery::ShouldQueryComputerListInformation()
{
#if ENABLE_COMPUTER_LIST_SUPPORT
    /* We support computer list queries for all our DS nodes */
    return (_recTypeSet &&
            (_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == false &&
            (_dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == false &&
            LWI_BITVECTOR_ISSET(_recTypeSet, LWIRecTypeLookup::idx_kDSStdRecordTypeComputerLists));
#else
    return false;
#endif
}

bool
LWIQuery::ShouldQueryComputerGroupInformation()
{
    /* We support computer group queries for all our DS nodes */
    return (_recTypeSet &&
            ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) || _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD)) == true &&
            LWI_BITVECTOR_ISSET(_recTypeSet, LWIRecTypeLookup::idx_kDSStdRecordTypeComputerGroups));
}

bool
LWIQuery::ShouldQueryComputerInformation()
{
    /* We support computer queries for the root DS node only */
    return (_recTypeSet &&
            LWI_BITVECTOR_ISSET(_recTypeSet, LWIRecTypeLookup::idx_kDSStdRecordTypeComputers) &&
            _pDirNode->fPlugInRootConnection);
}

#define TEST_ENABLED_PATH "/lwidstest.enabled"

bool
LWIQuery::IsTestEnabled()
{
    struct stat statBuffer;

    return (stat(TEST_ENABLED_PATH, &statBuffer) == 0) ? true : false;
}


bool
LWIQuery::IsAllDigit(const char* pszBuf)
{
    while (pszBuf && *pszBuf && (*pszBuf >= '0' && *pszBuf <= '9'))
    {
        pszBuf++;
    }
    return (pszBuf && !*pszBuf);
}

bool
LWIQuery::IsLWIGeneratedUIDForUser(const char* pszBuf)
{
    return (pszBuf && *pszBuf && !strncasecmp(pszBuf, LWI_UUID_UID_PREFIX, sizeof(LWI_UUID_UID_PREFIX)-1));
}

bool
LWIQuery::IsLWIGeneratedUIDForGroup(const char* pszBuf)
{
    return (pszBuf && *pszBuf && !strncasecmp(pszBuf, LWI_UUID_GID_PREFIX, sizeof(LWI_UUID_GID_PREFIX)-1));
}

long
LWIQuery::ExtractUIDFromGeneratedUID(const char* pszBuf, uid_t& uid)
{
    long macError = eDSNoErr;

    if (!IsLWIGeneratedUIDForUser(pszBuf))
    {
        macError = eDSAttributeNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    uid = strtoul(pszBuf+sizeof(LWI_UUID_UID_PREFIX)-1, NULL, 16);

cleanup:

    return macError;
}

long
LWIQuery::ExtractGIDFromGeneratedUID(const char* pszBuf, gid_t& gid)
{
    long macError = eDSNoErr;

    if (!IsLWIGeneratedUIDForGroup(pszBuf))
    {
        macError = eDSAttributeNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    gid = strtoul(pszBuf+sizeof(LWI_UUID_GID_PREFIX)-1, NULL, 16);

cleanup:

    return macError;
}

long
LWIQuery::BuildGeneratedUID(uid_t uid, char** ppszUID)
{
    char szBuf[LWI_GUID_LENGTH+1];

    sprintf(szBuf, LWI_UUID_UID_PREFIX "%.8X", uid);

    return LwAllocateString(szBuf, ppszUID);
}

long
LWIQuery::BuildGeneratedGID(gid_t gid, char** ppszGID)
{
    char szBuf[LWI_GUID_LENGTH+1];

    sprintf(szBuf, LWI_UUID_GID_PREFIX "%.8X", gid);

    return LwAllocateString(szBuf, ppszGID);
}

long
LWIQuery::ProcessUserAttributes(
    IN OUT PDSRECORD pRecord,
    IN OPTIONAL const char* pszName,
    IN const PLWIUSER pUser
    )
{
    long macError = eDSNoErr;
    int  iAttr = 0;
    bool bSetValue = false;
    PDSATTRIBUTE pAttribute = NULL;
    char* authString = NULL;

    bSetValue = _bGetValues;

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll);
    }

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrDistinguishedName);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_DISPLAY_NAME);              - same as above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_CN);                        - same as above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_NAME);                      - same as above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrGeneratedUID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrNFSHomeDirectory);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrHomeDirectory);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrOriginalHomeDirectory);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrOriginalNFSHomeDirectory);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPassword);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPasswordPlus);                - skipped
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPasswordPolicyOptions);       - skipped
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPwdAgingPolicy);              - skipped
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrChange);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrExpire);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPrimaryGroupID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrUniqueID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrUserShell);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrGroupMembership);             - skipped
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordType);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMetaNodeLocation);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXFlags);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXSettings);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMCXSettings);

        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrFirstName);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_GIVEN_NAME);               - same as above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrLastName);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_SN);                       - same as above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_AD_DOMAIN);
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_KERBEROS_PRINCIPAL);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_USER_PRINCIPAL_NAME);      - same as above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrEMailAddress);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_EMAIL_ADDRESS);            - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MS_EXCH_HOME_SERVER_NAME); - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MS_EXCH_HOME_MDB);         - included with above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrPhoneNumber);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrFaxNumber);                     - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMobileNumber);                  - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_TELEPHONE_NUMBER);            - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_FACSIMILIE_TELEPHONE_NUMBER); - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MOBILE);                      - included with above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrStreet);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrCity);                       - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrState);                      - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrPostalCode);                 - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrCountry);                    - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_STREET_ADDRESS);           - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_POST_OFFICE_BOX);          - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_CITY);                     - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_STATE);                    - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_POSTAL_CODE);              - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_COUNTRY);                  - included with above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrJobTitle);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_COMPANY_MAC);                    - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrDepartment);                 - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_TITLE);                    - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_COMPANY_AD);                  - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_DEPARTMENT);               - included with above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_HOME_DIRECTORY);
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_HOME_DRIVE);               - included with above
        
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_PWD_LAST_SET);                // SetLogon()
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_USER_ACCOUNT_CONTROL);     - included with above

        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MAX_MINUTES_UNTIL_CHANGE_PASSWORD); // SetPolicy
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MIN_MINUTES_UNTIL_CHANGE_PASSWORD); - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_K_DS_ATTR_MAX_PWD_AGE);    - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_K_DS_ATTR_MIN_PWD_AGE);    - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_K_DS_ATTR_MAX_PWD_AGE_DAYS); - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_K_DS_ATTR_MIN_PWD_AGE_DAYS); - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MAX_FAILED_LOGIN_ATTEMPTS);- included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_LOCKOUT_THRESHHOLD);       - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_USING_HISTORY);            - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_PWD_HISTORY_LENGTH);       - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MIN_CHARS);                - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_MIN_PWD_LENGTH);           - included with above
        // LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_K_DS_ATTR_DAYS_TILL_PWD_EXPIRES);    - included with above
    }

    for (iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(_attributeSet, iAttr))
        {
            switch (iAttr)
            {
                case LWIAttrLookup::idx_K_DS_ATTR_DISPLAY_NAME:
                case LWIAttrLookup::idx_K_DS_ATTR_CN:
                case LWIAttrLookup::idx_K_DS_ATTR_NAME:
                case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
                    macError = SetDistinguishedName(pRecord, pUser->pw_display_name, pUser->pw_name_as_queried, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
                    macError = SetGeneratedUID(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrNFSHomeDirectory:
                    macError = SetNFSHomeDirectory(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrHomeDirectory:
                    macError = SetHomeDirectory(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrOriginalHomeDirectory:
                    macError = SetOriginalHomeDirectory(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrOriginalNFSHomeDirectory:
                    macError = SetOriginalNFSHomeDirectory(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrPassword:
                    macError = SetPassword(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrPasswordPlus:
                    macError = SetPasswordPlus(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrPasswordPolicyOptions:
                    macError = SetPasswordPolicyOptions(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrPrimaryGroupID:
                    macError = SetPrimaryGroupID(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrUniqueID:
                    macError = SetUniqueID(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrUserShell:
                    macError = SetUserShell(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrGroupMembership:
                    macError = SetGroupMembership(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrRecordName:
                    macError = AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordName, NULL);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (bSetValue)
                    {
                        macError = SetAttributeValue(pAttribute, pUser->pw_name);
                        GOTO_CLEANUP_ON_MACERROR(macError);

                        if (pszName && strcmp(pszName, pUser->pw_name))
                        {
                            macError = SetAttributeValue(pAttribute, pszName);
                            GOTO_CLEANUP_ON_MACERROR(macError);
                        }
                    }
                    break;
                case LWIAttrLookup::idx_kDSNAttrRecordType:
                    macError = AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordType, NULL);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (bSetValue)
                    {
                        macError = SetAttributeValue(pAttribute, kDSStdRecordTypeUsers);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }
                    break;
                case LWIAttrLookup::idx_kDS1AttrPwdAgingPolicy:
                    macError = SetPasswordAgingPolicy(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrChange:
                    macError = SetPasswordChange(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrExpire:
                    macError = SetPasswordExpire(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrMetaNodeLocation:
                    macError = SetMetaNodeLocation(pRecord, _pDirNode->pszDirNodePath, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSAttributesAll:
                case LWIAttrLookup::idx_kDSAttributesStandardAll:
                    // We assume that the other bits are already set.
                    break;
                case LWIAttrLookup::idx_kDS1AttrTimeToLive:
                    macError = SetTimeToLive(pRecord, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrMCXFlags:
                    macError = SetMCXFlags(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrMCXSettings:
                case LWIAttrLookup::idx_kDSNAttrMCXSettings:
                    macError = SetMCXSettings(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrFirstName:
                case LWIAttrLookup::idx_K_DS_ATTR_GIVEN_NAME:
                    macError = SetFirstName(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDS1AttrLastName:
                case LWIAttrLookup::idx_K_DS_ATTR_SN:
                    macError = SetLastName(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_K_DS_ATTR_AD_DOMAIN:
                    macError = SetDomain(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_K_DS_ATTR_KERBEROS_PRINCIPAL:
                case LWIAttrLookup::idx_K_DS_ATTR_USER_PRINCIPAL_NAME:
                    macError = SetKerberosPrincipal(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrEMailAddress:
                case LWIAttrLookup::idx_K_DS_ATTR_EMAIL_ADDRESS:
                case LWIAttrLookup::idx_K_DS_ATTR_MS_EXCH_HOME_SERVER_NAME:
                case LWIAttrLookup::idx_K_DS_ATTR_MS_EXCH_HOME_MDB:
                    macError = SetEMail(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrPhoneNumber:
                case LWIAttrLookup::idx_K_DS_ATTR_TELEPHONE_NUMBER:
                case LWIAttrLookup::idx_kDSNAttrFaxNumber:
                case LWIAttrLookup::idx_K_DS_ATTR_FACSIMILIE_TELEPHONE_NUMBER:
                case LWIAttrLookup::idx_kDSNAttrMobileNumber:
                case LWIAttrLookup::idx_K_DS_ATTR_MOBILE:
                    macError = SetPhone(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrStreet:
                case LWIAttrLookup::idx_K_DS_ATTR_STREET_ADDRESS:
                case LWIAttrLookup::idx_K_DS_ATTR_POST_OFFICE_BOX:
                case LWIAttrLookup::idx_kDSNAttrCity:
                case LWIAttrLookup::idx_K_DS_ATTR_CITY:
                case LWIAttrLookup::idx_kDSNAttrState:
                case LWIAttrLookup::idx_K_DS_ATTR_STATE:
                case LWIAttrLookup::idx_kDSNAttrPostalCode:
                case LWIAttrLookup::idx_K_DS_ATTR_POSTAL_CODE:
                case LWIAttrLookup::idx_kDSNAttrCountry:
                case LWIAttrLookup::idx_K_DS_ATTR_COUNTRY:
                    macError = SetAddress(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_kDSNAttrJobTitle:
                case LWIAttrLookup::idx_K_DS_ATTR_TITLE:
                case LWIAttrLookup::idx_K_DS_ATTR_COMPANY_MAC:
                case LWIAttrLookup::idx_K_DS_ATTR_COMPANY_AD:
                case LWIAttrLookup::idx_kDSNAttrDepartment:
                case LWIAttrLookup::idx_K_DS_ATTR_DEPARTMENT:
                    macError = SetWork(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_K_DS_ATTR_HOME_DIRECTORY:
                case LWIAttrLookup::idx_K_DS_ATTR_HOME_DRIVE:
                    macError = SetProfile(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_K_DS_ATTR_PWD_LAST_SET:
                case LWIAttrLookup::idx_K_DS_ATTR_USER_ACCOUNT_CONTROL:
                    macError = SetLogon(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                case LWIAttrLookup::idx_K_DS_ATTR_MAX_MINUTES_UNTIL_CHANGE_PASSWORD:
                case LWIAttrLookup::idx_K_DS_ATTR_MIN_MINUTES_UNTIL_CHANGE_PASSWORD:
                case LWIAttrLookup::idx_K_DS_ATTR_MAX_PWD_AGE:
                case LWIAttrLookup::idx_K_DS_ATTR_MIN_PWD_AGE:
                case LWIAttrLookup::idx_K_DS_ATTR_MAX_PWD_AGE_DAYS:
                case LWIAttrLookup::idx_K_DS_ATTR_MIN_PWD_AGE_DAYS:
                case LWIAttrLookup::idx_K_DS_ATTR_MAX_FAILED_LOGIN_ATTEMPTS:
                case LWIAttrLookup::idx_K_DS_ATTR_LOCKOUT_THRESHHOLD:
                case LWIAttrLookup::idx_K_DS_ATTR_USING_HISTORY:
                case LWIAttrLookup::idx_K_DS_ATTR_PWD_HISTORY_LENGTH:
                case LWIAttrLookup::idx_K_DS_ATTR_MIN_CHARS:
                case LWIAttrLookup::idx_K_DS_ATTR_MIN_PWD_LENGTH:
                case LWIAttrLookup::idx_K_DS_ATTR_DAYS_TILL_PWD_EXPIRES:
                    macError = SetPolicy(pRecord, pUser, bSetValue);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                default:
                    break;
            }
        }
    }

cleanup:

    if (authString)
    {
        free(authString);
    }

    return macError;
}

long
LWIQuery::ProcessGroupAttributes(
    IN OUT PDSRECORD pRecord,
    IN OPTIONAL const char* pszName,
    IN const PLWIGROUP pGroup
    )
{
    long macError = eDSNoErr;
    int  iAttr = 0;
    bool bSetValue = false;
    PDSATTRIBUTE pAttribute = NULL;

    bSetValue = _bGetValues;

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll);
    }

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrDistinguishedName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrUniqueID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrGeneratedUID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPassword);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPrimaryGroupID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrGroupMembership);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrGroupMembers);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrComment);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMetaNodeLocation);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXFlags);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXSettings);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMCXSettings);
    }

    for (iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(_attributeSet, iAttr))
        {
            switch (iAttr)
            {
            case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
                macError = SetDistinguishedName(pRecord, pGroup->gr_name, pGroup->gr_name_as_queried, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrUniqueID:
                macError = SetUniqueID(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
                macError = SetGeneratedUID(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrPassword:
                macError = SetPassword(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrPrimaryGroupID:
                macError = SetPrimaryGroupID(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrGroupMembership:
                macError = SetGroupMembership(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrGroupMembers:
                macError = SetGroupMembers(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrRecordName:
                macError = AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordName, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    macError = SetAttributeValue(pAttribute, pGroup->gr_name);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (pszName && strcmp(pszName, pGroup->gr_name))
                    {
                        macError = SetAttributeValue(pAttribute, pszName);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }
                }
                break;
            case LWIAttrLookup::idx_kDS1AttrComment:
                macError = SetComment(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrMetaNodeLocation:
                macError = SetMetaNodeLocation(pRecord, _pDirNode->pszDirNodePath, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXFlags:
                macError = SetMCXFlags(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXSettings:
            case LWIAttrLookup::idx_kDSNAttrMCXSettings:
                macError = SetMCXSettings(pRecord, pGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSAttributesAll:
            case LWIAttrLookup::idx_kDSAttributesStandardAll:
                // We assume that the other bits are already set.
                break;
            case LWIAttrLookup::idx_kDS1AttrTimeToLive:
            {
                macError = SetTimeToLive(pRecord, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
            break;
            default:
                break;
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::ProcessComputerAttributes(
    IN OUT PDSRECORD pRecord,
    IN const PLWICOMPUTER pComputer
    )
{
    long macError = eDSNoErr;
    int  iAttr = 0;
    bool bSetValue = false;
    PDSATTRIBUTE pAttribute = NULL;

    bSetValue = _bGetValues;

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll);
    }

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordType);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrDistinguishedName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMetaNodeLocation);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrGeneratedUID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrENetAddress);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrIPAddress);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrComment);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrKeywords);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXFlags);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXSettings);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMCXSettings);
    }

    for (iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(_attributeSet, iAttr))
        {
            switch (iAttr)
            {
            case LWIAttrLookup::idx_kDSNAttrRecordType:
                macError = AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordType, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    macError = SetAttributeValue(pAttribute, kDSStdRecordTypeComputers);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                }
                break;
            case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
                macError = SetRealName(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrRecordName:
                macError = SetShortName(pRecord, pComputer, bSetValue);
                break;
            case LWIAttrLookup::idx_kDSNAttrMetaNodeLocation:
                macError = SetMetaNodeLocation(pRecord, _pDirNode->pszDirNodePath, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
                macError = SetGeneratedUID(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrENetAddress:
                macError = SetENetAddress(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrIPAddress:
                macError = SetIPAddress(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrComment:
                macError = SetComment(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrKeywords:
                macError = SetKeywords(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXFlags:
                macError = SetMCXFlags(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXSettings:
            case LWIAttrLookup::idx_kDSNAttrMCXSettings:
                macError = SetMCXSettings(pRecord, pComputer, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSAttributesAll:
            case LWIAttrLookup::idx_kDSAttributesStandardAll:
                // We assume that the other bits are already set.
                break;
            case LWIAttrLookup::idx_kDS1AttrTimeToLive:
            {
                macError = SetTimeToLive(pRecord, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
            break;
            default:
                break;
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::ProcessComputerListAttributes(
    IN OUT PDSRECORD pRecord,
    IN const PLWICOMPUTERLIST pComputerList
    )
{
    long macError = eDSNoErr;
    int  iAttr = 0;
    bool bSetValue = false;

    bSetValue = _bGetValues;

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll);
    }

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrDistinguishedName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMetaNodeLocation);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrGeneratedUID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPrimaryGroupID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrComment);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrComputers);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXFlags);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXSettings);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMCXSettings);
    }

    for (iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(_attributeSet, iAttr))
        {
            switch (iAttr)
            {
            case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
                macError = SetRealName(pRecord, pComputerList, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrRecordName:
                macError = SetShortName(pRecord, pComputerList, bSetValue);
                break;
            case LWIAttrLookup::idx_kDSNAttrMetaNodeLocation:
                macError = SetMetaNodeLocation(pRecord, _pDirNode->pszDirNodePath, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
                macError = SetGeneratedUID(pRecord, pComputerList, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrPrimaryGroupID:
                macError = SetPrimaryGroupID(pRecord, pComputerList, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrComment:
                macError = SetComment(pRecord, pComputerList, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrComputers:
                macError = SetComputers(pRecord, pComputerList, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXFlags:
                macError = SetMCXFlags(pRecord, pComputerList, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXSettings:
            case LWIAttrLookup::idx_kDSNAttrMCXSettings:
                macError = SetMCXSettings(pRecord, pComputerList, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSAttributesAll:
            case LWIAttrLookup::idx_kDSAttributesStandardAll:
                // We assume that the other bits are already set.
                break;
            case LWIAttrLookup::idx_kDS1AttrTimeToLive:
            {
                macError = SetTimeToLive(pRecord, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
            break;
            default:
                break;
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::ProcessComputerGroupAttributes(
    IN OUT PDSRECORD pRecord,
    IN const PLWICOMPUTERGROUP pComputerGroup
    )
{
    long macError = eDSNoErr;
    int  iAttr = 0;
    bool bSetValue = false;

    bSetValue = _bGetValues;

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll);
    }

    if (LWI_BITVECTOR_ISSET(_attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll))
    {
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrDistinguishedName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordName);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMetaNodeLocation);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrGeneratedUID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPrimaryGroupID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrComment);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrComputers);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrGroupMembers);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrGroupMembership);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXFlags);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXSettings);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMCXSettings);
    }

    for (iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(_attributeSet, iAttr))
        {
            switch (iAttr)
            {
            case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
                macError = SetRealName(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrRecordName:
                macError = SetShortName(pRecord, pComputerGroup, bSetValue);
                break;
            case LWIAttrLookup::idx_kDSNAttrMetaNodeLocation:
                macError = SetMetaNodeLocation(pRecord, _pDirNode->pszDirNodePath, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrGeneratedUID:
                macError = SetGeneratedUID(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrPrimaryGroupID:
                macError = SetPrimaryGroupID(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrComment:
                macError = SetComment(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrComputers:
                macError = SetComputers(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrGroupMembers:
                macError = SetGroupMembers(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrGroupMembership:
                macError = SetGroupMembership(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXFlags:
                macError = SetMCXFlags(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXSettings:
            case LWIAttrLookup::idx_kDSNAttrMCXSettings:
                macError = SetMCXSettings(pRecord, pComputerGroup, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSAttributesAll:
            case LWIAttrLookup::idx_kDSAttributesStandardAll:
                // We assume that the other bits are already set.
                break;
            case LWIAttrLookup::idx_kDS1AttrTimeToLive:
            {
                macError = SetTimeToLive(pRecord, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
            break;
            default:
                break;
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::QueryAllUserInformation(const char* pszName)
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;
    DWORD dwNumUsersFound = 0;
    DWORD iUser = 0;

    if (_pDirNode->fPlugInRootConnection)
    {
        macError = GetUserObjects(&ppUserObjects, &dwNumUsersFound);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (iUser = 0; iUser < dwNumUsersFound; iUser++)
        {
            if (ppUserObjects[iUser]->enabled)
            {
                macError = AddUserRecordHelper(ppUserObjects[iUser], NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }
    else
    {
        macError = CreateLWIUser(USER_NAME_GPO, /* Record Name */
                                 USER_NAME_GPO, /* Display name */
                                 NULL, /* Name as queried */
                                 NULL, /* Password */
                                 NULL, /* Class */
                                 NULL, /* GECOS */
                                 "/Users/gpouser", /* NFSHomeDirectory */
                                 NULL, /* HomeDirectory */
                                 NULL, /* OrigNFSHomeDirectory */
                                 NULL, /* OrigHomeDirectory */
                                 "/bin/bash", /* Shell */
                                 USER_UID_GPO,
                                 GROUP_GID_GPO,
                                 NULL, /* MCXValues */
                                 NULL, /* UserADInfo */
                                 &pUser);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddUserRecord(pUser, USER_NAME_GPO);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    FreeLWIUser(pUser);

    if (ppUserObjects)
    {
        FreeObjectList(dwNumUsersFound, ppUserObjects);
    }

    return macError;
}

long
LWIQuery::QueryAllGroupInformation(const char* pszName)
{
    long macError = eDSNoErr;
    PLWIGROUP pGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD iGroup = 0;

    if (_pDirNode->fPlugInRootConnection)
    {
        macError = GetGroupObjects(&ppGroupObjects, &dwNumGroupsFound);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            if (ppGroupObjects[iGroup]->enabled)
            {
                macError = AddGroupRecordHelper(ppGroupObjects[iGroup], false, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }
    else
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, USER_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, USER_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIGroup("Group of Users managed by GPO",
                                      NULL, /* Name as queried */
                                      NULL, /* Password */
                                      "GPOUserGroup",
                                      USER_GROUP_COMMENT,
                                      NULL, /* Member user name - not set for this group */
                                      GROUP_GID_GPO_ID,
                                      GROUP_GID_GPO,
                                      pMCXValueList,
                                      &pGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = AddGroupRecord(pGroup, "GPOUserGroup");
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

cleanup:

    FreeLWIGroup(pGroup);

    if (ppGroupObjects)
    {
        FreeObjectList(dwNumGroupsFound, ppGroupObjects);
    }

    FreeMCXValueList(pMCXValueList);
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    return macError;
}

long
LWIQuery::GetGPOComputerList(
    void
    )
{
    MACERROR  macError = eDSNoErr;
    PLWICOMPUTERLIST pComputerList = NULL;
    PMCXVALUE pMCXValueList = NULL;
    char      szGPOName[256] = { 0 };
    char      szGPOGUID[256] = { 0 };
    FILE *    fp = NULL;
    int       iStage = 0;
    PSTR pszHostname = NULL;

    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == true ||
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == true)
    {
       /* Computer Lists are only used on Tiger OS X, so we can return here */
        goto cleanup;
    }
    
    /* Get list of GPOs that apply to computer by parsing .lwe-computer-mcx */
    fp = fopen("/var/lib/likewise/grouppolicy/mcx/computer/.lwe-computer-mcx", "r");
    
    if (!fp)
    {
        LOG("FYI: No computer GPOs found, returning without error");
        goto cleanup;
    }
    
        
    /* Notes: GlennC - On Tiger systems, a computer can only be a member of one ComputerList, therefore we
       must limit the list of GPOs that describe MCX settings to just one, the last one we encounter in our
       list is the one with number 1 precedence in Active Directory for the computer policies. */
    while (1)
    {
        if ( NULL == fgets( szGPOName,
                            sizeof(szGPOName),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOName, TRUE, TRUE);
        iStage = 1;
        
        if ( NULL == fgets( szGPOGUID,
                            sizeof(szGPOGUID),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOGUID, TRUE, TRUE);
        iStage = 2;
    }
        
    if (iStage == 2)
    {
        char szPolicyPath[PATH_MAX] = { 0 };
        
        LOG("Adding GPO computer list for computer with (Name: %s GUID: %s)", szGPOName, szGPOGUID);
         
        sprintf(szPolicyPath, "%s/%s/%s", LWDS_GPO_CACHE_DIR, szGPOGUID, LWDS_COMPUTER_MCX_CSE_GUID);

        macError = ConvertMCXSettingsToMCXValues(szPolicyPath, MACHINE_GROUP_POLICY, &pMCXValueList);
        GOTO_CLEANUP_ON_MACERROR(macError);
            
        macError = CreateLWIComputerList(szGPOName,
                                         szGPOName,
                                         COMPUTER_LIST_COMMENT,
                                         szGPOGUID,
                                         UNSET_GID_UID_ID,
                                         pszHostname,
                                         pMCXValueList,
                                         &pComputerList);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddComputerListRecord(pComputerList);
        GOTO_CLEANUP_ON_MACERROR(macError);

        FreeLWIComputerList(pComputerList);
        pComputerList = NULL;
    }

cleanup:

    /* All these free functions test for null parameter */
    FreeLWIComputerList(pComputerList);
    FreeMCXValueList(pMCXValueList);
        
    LW_SAFE_FREE_STRING(pszHostname);
    
    if (fp)
        fclose(fp);

    return macError;
}

long
LWIQuery::QueryAllComputerListInformation(const char* pszName)
{
    long macError = eDSNoErr;
    PLWICOMPUTERLIST pComputerList = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == true ||
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == true)
    {
       /* Computer Lists are only used on Tiger OS X, so we can return here */
        goto cleanup;
    }

    if (_pDirNode->fPlugInRootConnection)
    {
        /* Look for list of computer list GPOs that apply to the local computer. This list will
           specify the name and GUID of the GPO (computer list) policy with MCX settings. */
        macError = GetGPOComputerList();
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, MACHINE_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, MACHINE_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIComputerList("List of Computers managed by GPO",
                                             "GPOComputerList",
                                             COMPUTER_LIST_COMMENT,
                                             COMPUTER_LIST_UID_ID,
                                             COMPUTER_LIST_UID,
                                             "GPOComputer",
                                             pMCXValueList,
                                             &pComputerList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = AddComputerListRecord(pComputerList);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

cleanup:

    FreeMCXValueList(pMCXValueList);
    FreeLWIComputerList(pComputerList);
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    return macError;
}

long
LWIQuery::GetGPOComputerGroups(
    void
    )
{
    MACERROR  macError = eDSNoErr;
    PLWICOMPUTERGROUP pComputerGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    char      szGPOName[256] = { 0 };
    char      szGPOGUID[256] = { 0 };
    PNETADAPTERINFO pTempNetInfo = _pNetAdapterList;
    FILE *    fp = NULL;
    PSTR pszHostname = NULL;

    while (pTempNetInfo)
    {
        if (pTempNetInfo->pszName)
        {
            if (!strcmp(pTempNetInfo->pszName, "en0"))
            {
                LOG("GetGPOComputerGroups() found primary en0 adapter...");
                LOG("Name: %s", pTempNetInfo->pszName);
                LOG("ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
                LOG("IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
                LOG("Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
                LOG("Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
                break;
            }
        }
        pTempNetInfo = pTempNetInfo->pNext;
    }

    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == false &&
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == false)
    {
       /* Computer Groups are only used on Leopard OS X, so we can return here */
        goto cleanup;
    }
    
    /* Get list of GPOs that apply to computer by parsing .lwe-computer-mcx */
    fp = fopen("/var/lib/likewise/grouppolicy/mcx/computer/.lwe-computer-mcx", "r");
    
    if (!fp)
    {
        LOG("FYI: No computer GPOs found, returning without error");
        goto cleanup;
    }
    
    while (1)
    {
        char szPolicyPath[PATH_MAX] = { 0 };
        
        if ( NULL == fgets( szGPOName,
                            sizeof(szGPOName),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOName, TRUE, TRUE);
        
        if ( NULL == fgets( szGPOGUID,
                            sizeof(szGPOGUID),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOGUID, TRUE, TRUE);
        
        LOG("Adding GPO computer group for computer with (Name: %s GUID: %s)", szGPOName, szGPOGUID);
         
        sprintf(szPolicyPath, "%s/%s/%s", LWDS_GPO_CACHE_DIR, szGPOGUID, LWDS_COMPUTER_MCX_CSE_GUID);
            
        macError = ConvertMCXSettingsToMCXValues(szPolicyPath, MACHINE_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);
            
        macError = CreateLWIComputerGroup(szGPOName,
                                          szGPOName,
                                          COMPUTER_GROUP_COMMENT,
                                          szGPOGUID,
                                          UNSET_GID_UID_ID,
                                          pszHostname,
                                          pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                          pMCXValueList,
                                          &pComputerGroup);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddComputerGroupRecord(pComputerGroup);
        GOTO_CLEANUP_ON_MACERROR(macError);

        FreeLWIComputerGroup(pComputerGroup);
        pComputerGroup = NULL;
    }

cleanup:

    /* All these free functions test for null parameter */
    FreeLWIComputerGroup(pComputerGroup);
    FreeMCXValueList(pMCXValueList);
    
    if (fp)
    {
        fclose(fp);
    }
        
    LW_SAFE_FREE_STRING(pszHostname);

    return macError;
}

long
LWIQuery::QueryAllComputerGroupInformation(const char* pszName)
{
    long macError = eDSNoErr;
    PLWICOMPUTERGROUP pComputerGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == false &&
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == false)
    {
       /* Computer Groups are only used on Leopard OS X, so we can return here */
        goto cleanup;
    }

    if (_pDirNode->fPlugInRootConnection)
    {
        /* Look for list of computer group GPOs that apply to the local computer. This list will
           specify the name and GUID of the GPO (computer group) policy with MCX settings. */
        macError = GetGPOComputerGroups();
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, MACHINE_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, MACHINE_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIComputerGroup("Group of Computers managed by GPO",
                                              "GPOComputerGroup",
                                              COMPUTER_GROUP_COMMENT,
                                              COMPUTER_GROUP_UID_ID,
                                              COMPUTER_GROUP_UID,
                                              "GPOComputer",
                                              COMPUTER_MAC_GPO,
                                              pMCXValueList,
                                              &pComputerGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = AddComputerGroupRecord(pComputerGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

cleanup:

    FreeMCXValueList(pMCXValueList);
    FreeLWIComputerGroup(pComputerGroup);
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    return macError;
}

long
LWIQuery::QueryAllComputerInformation(const char* pszName)
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;

    /* This Likewise DSPlugin is designed to not return all computers that exist in AD. Rather than
       enumerating all computers in a loop of GetComputersNext() calls, we will simply return the one
       computer that represents the default or localhost computer.                                     */

    /* NULL name below will indicate that we want the localhost or default computer */
    macError = GetComputerByName(NULL, &pComputer);
    GOTO_CLEANUP_ON_MACERROR(macError);

    AddComputerRecord(pComputer);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (pComputer)
    {
        FreeLWIComputer(pComputer);
    }

    return macError;
}

long
LWIQuery::QueryUserInformationByName(const char* pszName)
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    if ( !strcmp(pszName, kDSRecordsAll) )
    {
        macError = QueryAllUserInformation(pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (!strcmp(pszName, USER_NAME_GPO))
    {
        macError = CreateLWIUser(USER_NAME_GPO, /* Record Name */
                                 USER_NAME_GPO, /* Display name */
                                 NULL, /* Name as queried */
                                 NULL, /* Password */
                                 NULL, /* Class */
                                 NULL, /* GECOS */
                                 "/Users/gpouser", /* NFSHomeDirectory */
                                 NULL, /* HomeDirectory */
                                 NULL, /* OrigNFSHomeDirectory */
                                 NULL, /* OrigHomeDirectory */
                                 "/bin/bash", /* Shell */
                                 USER_UID_GPO,
                                 GROUP_GID_GPO,
                                 NULL, /* MCXValues */
                                 NULL, /* UserADInfo */
                                 &pUser);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddUserRecord(pUser, USER_NAME_GPO);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetUserObjectFromName(pszName, &ppUserObjects);
        GOTO_CLEANUP_ON_MACERROR(macError);

        if (ppUserObjects[0]->enabled)
        {
            macError = AddUserRecordHelper(ppUserObjects[0], pszName);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

cleanup:

    if (ppUserObjects) {
        FreeObjectList(1, ppUserObjects);
    }

    FreeLWIUser(pUser);

    return macError;
}

long
LWIQuery::QueryUserInformationById(uid_t uid)
{
    long macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    macError = GetUserObjectFromId(uid, &ppUserObjects);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (ppUserObjects[0]->enabled)
    {
        macError = AddUserRecordHelper(ppUserObjects[0], NULL);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppUserObjects) {
        FreeObjectList(1, ppUserObjects);
    }

    return macError;
}

long
LWIQuery::QueryUserInformationByGeneratedUID(const char* pszGUID)
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;
    uid_t uid;

    if (!strcmp(pszGUID, USER_NAME_GPO))
    {
        macError = CreateLWIUser(USER_NAME_GPO, /* Record Name */
                                 USER_NAME_GPO, /* Display name */
                                 NULL, /* Name as queried */
                                 NULL, /* Password */
                                 NULL, /* Class */
                                 NULL, /* GECOS */
                                 "/Users/gpouser", /* NFSHomeDirectory */
                                 NULL, /* HomeDirectory */
                                 NULL, /* OrigNFSHomeDirectory */
                                 NULL, /* OrigHomeDirectory */
                                 "/bin/bash", /* Shell */
                                 USER_UID_GPO,
                                 GROUP_GID_GPO,
                                 NULL, /* MCXValues */
                                 NULL, /* UserADInfo */
                                 &pUser);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddUserRecord(pUser, USER_NAME_GPO);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = ExtractUIDFromGeneratedUID(pszGUID, uid);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = QueryUserInformationById(uid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    FreeLWIUser(pUser);

    return macError;
}

long
LWIQuery::QueryUserInformationByPrimaryGroupID(const char* pszPrimaryGID)
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;

    if (!strcmp(pszPrimaryGID, GROUP_GID_GPO_ID)) {
        macError = CreateLWIUser(USER_NAME_GPO, /* Record Name */
                                 USER_NAME_GPO, /* Display name */
                                 NULL, /* Name as queried */
                                 NULL, /* Password */
                                 NULL, /* Class */
                                 NULL, /* GECOS */
                                 "/Users/gpouser", /* NFSHomeDirectory */
                                 NULL, /* HomeDirectory */
                                 NULL, /* OrigNFSHomeDirectory */
                                 NULL, /* OrigHomeDirectory */
                                 "/bin/bash", /* Shell */
                                 USER_UID_GPO,
                                 GROUP_GID_GPO,
                                 NULL, /* MCXValues */
                                 NULL, /* UserADInfo */
                                 &pUser);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddUserRecord(pUser, USER_NAME_GPO);
        GOTO_CLEANUP_ON_MACERROR(macError);
    } else {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    FreeLWIUser(pUser);

    return macError;
}

long
LWIQuery::QueryGroupInformationByName(const char* pszName)
{
    long macError = eDSNoErr;

    if ( !strcmp(pszName, kDSRecordsAll) )
    {
        macError = QueryAllGroupInformation(pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetGroupInformationByName(pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::QueryGroupInformationById(gid_t gid)
{
    long macError = eDSNoErr;
    PLWIGROUP pGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    if (gid == GROUP_GID_GPO)
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, USER_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, USER_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIGroup("Group of Users managed by GPO",
                                      NULL, /* Name as queried */
                                      NULL, /* Password */
                                      "GPOUserGroup",
                                      USER_GROUP_COMMENT,
                                      NULL, /* Member user name - not set for this group */
                                      GROUP_GID_GPO_ID,
                                      GROUP_GID_GPO,
                                      pMCXValueList,
                                      &pGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = AddGroupRecord(pGroup, "GPOUserGroup");
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else
    {
        macError = GetGroupInformationById(gid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    FreeMCXValueList(pMCXValueList);
    FreeLWIGroup(pGroup);
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    return macError;
}

long
LWIQuery::QueryGroupInformationByGeneratedUID(const char* pszGUID)
{
    long macError = eDSNoErr;
    gid_t gid;

    macError = ExtractGIDFromGeneratedUID(pszGUID, gid);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetGroupInformationById(gid);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    return macError;
}

long
LWIQuery::QueryComputerListInformationByName(const char* pszName)
{
    long macError = eDSNoErr;
    PLWICOMPUTERLIST pComputerList = NULL;

    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == true ||
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == true)
    {
       /* Computer Lists are only used on Tiger OS X, so we can return here */
        goto cleanup;
    }

    if ( !strcmp(pszName, kDSRecordsAll) )
    {
        macError = QueryAllComputerListInformation(pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetComputerListByName(pszName, &pComputerList);
        GOTO_CLEANUP_ON_MACERROR(macError);

        AddComputerListRecord(pComputerList);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    FreeLWIComputerList(pComputerList);

    return macError;
}

long
LWIQuery::QueryComputerGroupInformationByName(const char* pszName)
{
    long macError = eDSNoErr;
    PLWICOMPUTERGROUP pComputerGroup = NULL;

    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == false &&
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == false)
    {
       /* Computer Groups are only used on Leopard OS X, so we can return here */
        goto cleanup;
    }

    if ( !strcmp(pszName, kDSRecordsAll) )
    {
        macError = QueryAllComputerGroupInformation(pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetComputerGroupByName(pszName, &pComputerGroup);
        GOTO_CLEANUP_ON_MACERROR(macError);

        AddComputerGroupRecord(pComputerGroup);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    FreeLWIComputerGroup(pComputerGroup);

    return macError;
}

long
LWIQuery::QueryComputerInformationByName(const char* pszName)
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;

    if ( !strcmp(pszName, kDSRecordsAll) )
    {
        macError = QueryAllComputerInformation(pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetComputerByName(pszName, &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);

        AddComputerRecord(pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (pComputer)
    {
        FreeLWIComputer(pComputer);
    }

    return macError;
}

long
LWIQuery::QueryComputerInformationByENetAddress(const char* pszENetAddress)
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;

    if ( !strcmp(pszENetAddress, kDSRecordsAll) )
    {
        LOG("ERROR: Unexpected Ethernet address value '%s'", pszENetAddress);
        macError = QueryAllComputerInformation(pszENetAddress);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetComputerByENetAddress(pszENetAddress, &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);

        AddComputerRecord(pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (pComputer)
    {
        FreeLWIComputer(pComputer);
    }

    return macError;
}

long
LWIQuery::QueryComputerInformationByIPAddress(const char* pszIPAddress)
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;

    if ( !strcmp(pszIPAddress, kDSRecordsAll) )
    {
        LOG("ERROR: Unexpected IP address value '%s'", pszIPAddress);
        macError = QueryAllComputerInformation(pszIPAddress);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetComputerByIPAddress(pszIPAddress, &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);

        AddComputerRecord(pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (pComputer)
    {
        FreeLWIComputer(pComputer);
    }

    return macError;
}

long
LWIQuery::QueryComputerInformationByGeneratedUID(const char* pszGeneratedUID)
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;

    if ( !strcmp(pszGeneratedUID, kDSRecordsAll) )
    {
        LOG("ERROR: Unexpected GeneratedUID value '%s'", pszGeneratedUID);
        macError = QueryAllComputerInformation(pszGeneratedUID);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetComputerByGeneratedUID(pszGeneratedUID, &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);

        AddComputerRecord(pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (pComputer)
    {
        FreeLWIComputer(pComputer);
    }

    return macError;
}

static
PMCXVALUE
CombineMCXValueLists(
    PMCXVALUE pPrependList,
    PMCXVALUE pExistingList
    )
{
    PMCXVALUE pTemp = pPrependList;
    PMCXVALUE pPrev = NULL;

    while (pTemp)
    {
        pPrev = pTemp;

        pTemp = pTemp->pNext;
    }

    if (pPrev)
    {
        pPrev->pNext = pExistingList;
        pTemp = pPrependList;
    }
    else
    {
        pTemp = pExistingList;
    }

    return pTemp;
}

static
MACERROR
GetMCXValueInfo(
    PMCXVALUE pItem,
    PSTR* ppszKeyName
    )
{
    MACERROR  macError = eDSNoErr;
    PSTR pszKeyName = NULL;
    PSTR pKey = NULL;
    PSTR pKeyEnd = NULL;
    PSTR pBuf = NULL;
    int len = 0;

    pBuf = pItem->pValueData;
    len = pItem->iValLen;

    pKey = strnstr(pBuf, "<key>mcx_application_data</key>", len);

    if (pKey)
    {
        pBuf = pKey + strlen("<key>mcx_application_data</key>");
        len = pItem->iValLen - (pBuf - pItem->pValueData);

        pKey = strnstr(pBuf, "<key>", len);

        if (pKey)
        {
            pBuf = pKey + strlen("<key>");
            len = pItem->iValLen - (pBuf - pItem->pValueData);

            pKeyEnd = strnstr(pBuf, "</key>", len);

            if (pKeyEnd)
            {
                macError = LwAllocateMemory((pKeyEnd - pBuf + 1) * sizeof(char), (PVOID*)&pszKeyName);           
                GOTO_CLEANUP_ON_MACERROR(macError);

                strncpy(pszKeyName, pBuf, pKeyEnd - pBuf);
                *ppszKeyName = pszKeyName;
            }
            else
            {
                macError = eDSOperationFailed;
            }
        }
        else
        {
            macError = eDSOperationFailed;
        }
    }
    else
    {
        macError = eDSOperationFailed;
    }

cleanup:

    return macError;
}

static
bool
IsDuplicateMCXValue(
    PMCXVALUE pItem1,
    PMCXVALUE pItem2
    )
{
    MACERROR  macError = eDSNoErr;
    bool bMatch = false;
    PSTR pszKeyNameItem1 = NULL;
    PSTR pszKeyNameItem2 = NULL;

    macError = GetMCXValueInfo(pItem1, &pszKeyNameItem1);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetMCXValueInfo(pItem2, &pszKeyNameItem2);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!strcasecmp(pszKeyNameItem1, pszKeyNameItem2))
    {
        bMatch = true;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszKeyNameItem1);
    LW_SAFE_FREE_STRING(pszKeyNameItem2);

    return bMatch;
}

static
PMCXVALUE
FindAndRemoveDuplicateFromMCXValuesList(
    PMCXVALUE pItem,
    PMCXVALUE pList
    )
{
    PMCXVALUE pCurrent = pList;
    PMCXVALUE pPrev = NULL;
    PMCXVALUE pTemp = NULL;
    PMCXVALUE pNewList = NULL;

    while (pCurrent)
    {
        if (IsDuplicateMCXValue(pItem, pCurrent))
        {
            pTemp = pCurrent;

            if (pPrev)
            {
                pPrev->pNext = pCurrent->pNext;
            }

            pCurrent = pCurrent->pNext;
            pTemp->pNext = NULL;
            FreeMCXValueList(pTemp);
        }
        else
        {
            pPrev = pCurrent;
            pCurrent = pCurrent->pNext;
        }

        if (!pNewList)
        {
            pNewList = pPrev;
        }
    }

    return pNewList;
}

static
PMCXVALUE
AddToMCXValuesList(
    PMCXVALUE pItem,
    PMCXVALUE pList
    )
{
    PMCXVALUE pCurrent = pList;
    PMCXVALUE pPrev = NULL;

    while (pCurrent)
    {
        pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (pPrev)
    {
        pPrev->pNext = pItem;
    }
    else
    {
        return pItem;
    }

    return pList;
}

static
PMCXVALUE
RemoveDuplicateMCXValuesFromList(
    PMCXVALUE pMCXValueList
    )
{
    PMCXVALUE pNewList = NULL;
    PMCXVALUE pCurrent = pMCXValueList;
    PMCXVALUE pTemp = NULL;

    while (pCurrent)
    {
        pTemp = pCurrent;
        pCurrent = FindAndRemoveDuplicateFromMCXValuesList(pTemp, pCurrent->pNext);
        pTemp->pNext = NULL;

        pNewList = AddToMCXValuesList(pTemp, pNewList);
    }

    return pNewList;
}

static
BOOLEAN
IsMCXValueForSection(
    PCSTR pszMCXSectionName,
    PMCXVALUE pItem
    )
{
    MACERROR  macError = eDSNoErr;
    BOOLEAN bMatch = FALSE;
    PSTR pszKeyNameItem = NULL;

    macError = GetMCXValueInfo(pItem, &pszKeyNameItem);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!strcasecmp(pszMCXSectionName, pszKeyNameItem))
    {
        bMatch = TRUE;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszKeyNameItem);

    return bMatch;
}

static
BOOLEAN
FindGPOGroupMCXSetting(
    PCSTR     pszMCXSectionName,
    PMCXVALUE pMCXValueList
    )
{
    PMCXVALUE pCurrent = pMCXValueList;
    BOOLEAN fFound = FALSE;

    while (pCurrent)
    {
        if (IsMCXValueForSection(pszMCXSectionName, pCurrent))
        {
            fFound = TRUE;
            break;
        }
        else
        {
            pCurrent = pCurrent->pNext;
        }
    }

    return fFound;
}

static
long
GetGPOGroupMCXSettingsForUser_HighPriorityOnly(
    const char* pszName,
    uid_t       uid,
    PMCXVALUE * ppMCXValueList,
    LWE_DS_FLAGS Flags
    )
{
    MACERROR  macError = eDSNoErr;
    PMCXVALUE pMCXValueList = NULL;
    PMCXVALUE pHomeDirDockValue = NULL;
    char      szUserGPOFile[PATH_MAX] = { 0 };
    char      szGPOName[256] = { 0 };
    char      szGPOGUID[256] = { 0 };
    FILE *    fp = NULL;
    int       iStage = 0;
    BOOLEAN   fContainsDockSettings = FALSE;
    BOOLEAN   fContainsLoginWindowSettings = FALSE;
    
    if (!pszName || !uid)
    {
        LOG("Called with invalid parameter");
        goto cleanup;
    }

    if (Flags & LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK)
    {
        macError = GetHomeDirectoryDockMCXValue(&pHomeDirDockValue);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    sprintf(szUserGPOFile, "/var/lib/likewise/grouppolicy/mcx/users/%ld/.lwe-user-mcx", (long) uid);

    /* Get list of GPOs that apply to user by parsing .lwe-user-mcx for specific user*/
    fp = fopen(szUserGPOFile, "r");
    
    while (fp)
    {
        if ( NULL == fgets( szGPOName,
                            sizeof(szGPOName),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOName, TRUE, TRUE);
        iStage = 1;
        
        if ( NULL == fgets( szGPOGUID,
                            sizeof(szGPOGUID),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOGUID, TRUE, TRUE);
        iStage = 2;
    }
 
    if (iStage == 2)
    {
        char szPolicyPath[PATH_MAX] = { 0 };
        
        LOG("Adding user group MCX settings to user (%s) from GPO (Name: %s GUID: %s)", pszName, szGPOName, szGPOGUID);
         
        sprintf(szPolicyPath, "/var/lib/likewise/grouppolicy/user-cache/%ld/%s/%s", (long)uid, szGPOGUID, LWDS_USER_MCX_CSE_GUID);

        macError = ConvertMCXSettingsToMCXValues(szPolicyPath, USER_GROUP_POLICY, &pMCXValueList);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    fContainsLoginWindowSettings = FindGPOGroupMCXSetting("com.apple.loginwindow", pMCXValueList);
    fContainsDockSettings = FindGPOGroupMCXSetting("com.apple.dock", pMCXValueList);

    if (!fContainsLoginWindowSettings && !fContainsDockSettings)
    {
        /* Add additional MCX settings plist data to create a dock folder for the home directory URL */
        if (pHomeDirDockValue)
        {
            pMCXValueList = CombineMCXValueLists(pMCXValueList, pHomeDirDockValue);
            pHomeDirDockValue = NULL;
        }
    }

    /* If there is more than one plist item for a given key (com.apple.dock for example),
       then the Mac will show assertions. We need to strip out any possible duplicates. */
    pMCXValueList = RemoveDuplicateMCXValuesFromList(pMCXValueList);

    *ppMCXValueList = pMCXValueList;
    pMCXValueList = NULL;

cleanup:

    FreeMCXValueList(pMCXValueList);
    FreeMCXValueList(pHomeDirDockValue);
    
    if (fp)
        fclose(fp);

    return macError;
}

static
long
GetGPOGroupMCXSettingsForUser_Combined(
    const char* pszName,
    uid_t       uid,
    PMCXVALUE * ppMCXValueList,
    LWE_DS_FLAGS Flags
    )
{
    MACERROR  macError = eDSNoErr;
    PMCXVALUE pMCXValueList = NULL;
    PMCXVALUE pHomeDirDockValue = NULL;
    char      szUserGPOFile[PATH_MAX] = { 0 };
    char      szGPOName[256] = { 0 };
    char      szGPOGUID[256] = { 0 };
    FILE *    fp = NULL;
    BOOLEAN   fContainsDockSettings = FALSE;
    BOOLEAN   fContainsLoginWindowSettings = FALSE;
    
    if (!pszName || !uid)
    {
        LOG("Called with invalid parameter");
        goto cleanup;
    }

    if (Flags & LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK)
    {
        macError = GetHomeDirectoryDockMCXValue(&pHomeDirDockValue);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    sprintf(szUserGPOFile, "/var/lib/likewise/grouppolicy/mcx/users/%ld/.lwe-user-mcx", (long) uid);

    /* Get list of GPOs that apply to user by parsing .lwe-user-mcx for specific user*/
    fp = fopen(szUserGPOFile, "r");
    
    while (fp)
    {
        char szPolicyPath[PATH_MAX] = { 0 };
        PMCXVALUE pNewList = NULL;

        if ( NULL == fgets( szGPOName,
                            sizeof(szGPOName),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOName, TRUE, TRUE);
        
        if ( NULL == fgets( szGPOGUID,
                            sizeof(szGPOGUID),
                            fp) ) {
            if (feof(fp)) {
                break;
            }

            macError = LwErrnoToWin32Error(errno);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        LwStripWhitespace(szGPOGUID, TRUE, TRUE);
        
        sprintf(szPolicyPath, "/var/lib/likewise/grouppolicy/user-cache/%ld/%s/%s", (long) uid, szGPOGUID, LWDS_USER_MCX_CSE_GUID);

        macError = ConvertMCXSettingsToMCXValues(szPolicyPath, USER_GROUP_POLICY, &pNewList);
        GOTO_CLEANUP_ON_MACERROR(macError);
         
        LOG("Adding user group MCX settings to user (%s) from GPO (Name: %s GUID: %s)", pszName, szGPOName, szGPOGUID);

        pMCXValueList = CombineMCXValueLists(pNewList, pMCXValueList);
    }

    fContainsLoginWindowSettings = FindGPOGroupMCXSetting("com.apple.loginwindow", pMCXValueList);
    fContainsDockSettings = FindGPOGroupMCXSetting("com.apple.dock", pMCXValueList);

    if (!fContainsLoginWindowSettings && !fContainsDockSettings)
    {
        /* Add additional MCX settings plist data to create a dock folder for the home directory URL */
        if (pHomeDirDockValue)
        {
            pMCXValueList = CombineMCXValueLists(pMCXValueList, pHomeDirDockValue);
            pHomeDirDockValue = NULL;
        }
    }

    /* If there is more than one plist item for a given key (com.apple.dock for example),
       then the Mac will show assertions. We need to strip out any possible duplicates. */
    pMCXValueList = RemoveDuplicateMCXValuesFromList(pMCXValueList);

    *ppMCXValueList = pMCXValueList;
    pMCXValueList = NULL;

cleanup:

    FreeMCXValueList(pMCXValueList);
    
    if (fp)
        fclose(fp);

    return macError;
}

long
LWIQuery::GetGPOGroupMCXSettingsForUser(
    const char* pszName,
    uid_t       uid,
    PMCXVALUE * ppMCXValueList,
    LWE_DS_FLAGS Flags
    )
{
    if (Flags & LWE_DS_FLAG_MERGE_MODE_MCX)
    {
        return GetGPOGroupMCXSettingsForUser_Combined(pszName, uid, ppMCXValueList, Flags);
    }
    else
    {
        return GetGPOGroupMCXSettingsForUser_HighPriorityOnly(pszName, uid, ppMCXValueList, Flags);
    }
}

long
LWIQuery::GetHomeDirectoryProtocolXmlAndMountPath(
    PSTR pszHomeDirectory,
    LWE_DS_FLAGS Flags,
    PSTR * ppszHomeDirectoryXML,
    PSTR * ppszHomeDirectoryMount
    )
{
    MACERROR macError = eDSNoErr;
    char szPath[PATH_MAX] = { 0 };
    char szNewPath[PATH_MAX] = { 0 };
    PSTR pszHomeDirectoryXML = NULL;
    PSTR pszHomeDirectoryMount = NULL;
    PSTR pszServer = NULL;
    PSTR pszShare = NULL;
    PSTR pszPath = NULL;
    PSTR pszToken = NULL;

    if (pszHomeDirectory &&
        strlen(pszHomeDirectory) < sizeof(szPath) &&
        pszHomeDirectory[0] == '\\' &&
        pszHomeDirectory[1] == '\\' &&
        pszHomeDirectory[2] != '\0')
    {
        strcpy(szPath, &pszHomeDirectory[2]);

        /* Get Server name from path */
        pszToken = strtok(szPath, "\\");
        if (pszToken)
        {
            macError = LwAllocateString(pszToken, &pszServer);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = eDSInvalidBuffFormat;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        /* Get Share name from path */
        pszToken = strtok(NULL, "\\");
        if (pszToken)
        {
            macError = LwAllocateString(pszToken, &pszShare);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = eDSInvalidBuffFormat;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        /* Get Path portion from any remaining path */
        pszToken = strtok(NULL, "\\");
        while (pszToken)
        {
            strcat(szNewPath, pszToken);
            pszToken = strtok(NULL, "\\");
            if (pszToken)
            {
                strcat(szNewPath, "/");
            }
        }

        if (strlen(szNewPath) > 0)
        {
            macError = LwAllocateString(szNewPath, &pszPath);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else
    {
        macError = eDSInvalidBuffFormat;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = LwAllocateStringPrintf(&pszHomeDirectoryMount,
                                      "/Network/Servers/%s/%s%s%s",
                                      pszServer,
                                      pszShare,
                                      pszPath ? "/" : "",
                                      pszPath ? pszPath : "");
    GOTO_CLEANUP_ON_MACERROR(macError);


    if (Flags & LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_SMB)
    {
        macError = LwAllocateStringPrintf(&pszHomeDirectoryXML,
                                          "<home_dir><url>smb://%s/%s</url><path>%s</path></home_dir>",
                                          pszServer,
                                          pszShare,
                                          pszPath ? pszPath : "");
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (Flags & LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_AFP)
    {
        macError = LwAllocateStringPrintf(&pszHomeDirectoryXML,
                                          "<home_dir><url>afp://%s/%s</url><path>%s</path></home_dir>",
                                          pszServer,
                                          pszShare,
                                          pszPath ? pszPath : "");
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = LwAllocateStringPrintf(&pszHomeDirectoryXML,
                                          "<home_dir><url>http://%s/%s%s%s</url></home_dir>",
                                          pszServer,
                                          pszShare,
                                          pszPath ? "/" : "",
                                          pszPath ? pszPath : "");
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppszHomeDirectoryMount = pszHomeDirectoryMount;
    pszHomeDirectoryMount = NULL;
    *ppszHomeDirectoryXML = pszHomeDirectoryXML;
    pszHomeDirectoryXML = NULL;

cleanup:

    if (macError == eDSInvalidBuffFormat)
    {
        LOG("User home directory path not formatted correctly, going to skip AD UNC path for user");
        *ppszHomeDirectoryMount = NULL;
        *ppszHomeDirectoryXML = NULL;
        macError = eDSNoErr;
    }

    LW_SAFE_FREE_STRING(pszHomeDirectoryMount);
    LW_SAFE_FREE_STRING(pszHomeDirectoryXML);
    LW_SAFE_FREE_STRING(pszServer);
    LW_SAFE_FREE_STRING(pszShare);
    LW_SAFE_FREE_STRING(pszPath);

    return macError;
}

long
LWIQuery::GetUserHomeFolderSettings(
    PSTR pszHomeDirectory,
    LWE_DS_FLAGS Flags,
    PSTR * ppszNFSHomeDirectory,
    PSTR * ppszHomeDirectory,
    PSTR * ppszOriginalNFSHomeDirectory,
    PSTR * ppszOriginalHomeDirectory
    )
{
    MACERROR macError = eDSNoErr;
    PSTR pszHomeDirectoryXML = NULL;
    PSTR pszHomeDirectoryMount = NULL;

    /* Preset out parameters */
    *ppszNFSHomeDirectory = NULL;
    *ppszHomeDirectory = NULL;
    *ppszOriginalHomeDirectory = NULL;
    *ppszOriginalNFSHomeDirectory = NULL;

    if (pszHomeDirectory &&
        (Flags & LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_SMB ||
         Flags & LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_AFP))
    {
        macError = GetHomeDirectoryProtocolXmlAndMountPath(pszHomeDirectory, Flags, &pszHomeDirectoryXML, &pszHomeDirectoryMount);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        if (Flags & LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK)
        {
            /* Scenario - User will get a folder added to dock for network file share resource
               Here we need to set the following:
                   NFS Home Directory:          Null - so that user default is still applied from LSASS
                   Home Directory :             Null - so that this attribute is blank on user object
                   Original Home Directory:     Ex: <home_dir><url>smb://<server>/<share></url><path><my path></path></home_dir>
                   Original NFS Home Directory: Ex: /Network/Servers/<server>/<share>/<my path>
            */
            *ppszOriginalHomeDirectory = pszHomeDirectoryXML;
            pszHomeDirectoryXML = NULL;
            *ppszOriginalNFSHomeDirectory = pszHomeDirectoryMount;
            pszHomeDirectoryMount = NULL;
        }
        else
        {
            /* Scenario - User will get home directory configured to use a network file share resource
               Here we need to set the following:
                   NFS Home Directory:          Ex: /Network/Servers/<server>/<share>/<my path>
                   Home Directory:              Ex: <home_dir><url>smb://<server>/<share></url><path><my path></path></home_dir>
                   Original Home Directory :    Null - so that this attribute is blank on user object
                   Original NFS Home Directory: Null - so that this attribute is blank on user object
            */
            *ppszNFSHomeDirectory = pszHomeDirectoryMount;
            pszHomeDirectoryMount = NULL;
            *ppszHomeDirectory = pszHomeDirectoryXML;
            pszHomeDirectoryXML = NULL;
        }
    }
    else
    {
        /* There is not a configuration to create a network file resource connection.
           Here we can return with no attributes to set for the user record. */
        goto cleanup;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszHomeDirectoryXML);
    LW_SAFE_FREE_STRING(pszHomeDirectoryMount);

    return macError;
}

long
LWIQuery::QueryGroupsForUser(
    IN gid_t gid,
    IN PCSTR pszUserSid
    )
{
    MACERROR macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppGroups = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD iGroup = 0;
    BOOLEAN fFoundPrimaryGroup = FALSE;

    LOG("QueryGroupsForUser(gid: %d, UserSid: %s",
        gid,
        pszUserSid ? pszUserSid : "<null>");

    macError = GetUserGroups(pszUserSid, &ppGroups, &dwNumGroupsFound);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LOG("QueryGroupsForUser found %d entries", dwNumGroupsFound);

    for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
    {
        if (ppGroups[iGroup] && ppGroups[iGroup]->enabled)
        {
            if (gid == ppGroups[iGroup]->groupInfo.gid)
            {
                fFoundPrimaryGroup = TRUE;
            }
 
            macError = AddGroupRecordHelper(ppGroups[iGroup], false, NULL);
            GOTO_CLEANUP_ON_MACERROR(macError);
            LOG("QueryGroupsForUser adding group id", ppGroups[iGroup]);
        }
    }

    if (!fFoundPrimaryGroup)
    {
        macError = GetGroupInformationById(gid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppGroups)
    {
        FreeObjectList(dwNumGroupsFound, ppGroups);
    }

    return macError;
}

long
LWIQuery::QueryGroupsForUserByName(
    IN const char* pszName
    )
{
    MACERROR macError = eDSNoErr;
    PLWIGROUP pGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    if (!strcmp(pszName, USER_NAME_GPO))
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, USER_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, USER_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIGroup("Group of Users managed by GPO",
                                      NULL, /* Name as queried */
                                      NULL, /* Password */
                                      "GPOUserGroup",
                                      USER_GROUP_COMMENT,
                                      NULL, /* Member user name - not set for this group */
                                      GROUP_GID_GPO_ID,
                                      GROUP_GID_GPO,
                                      pMCXValueList,
                                      &pGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = AddGroupRecord(pGroup, "GPOUserGroup");
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else
    {
        macError = GetUserObjectFromName(pszName, &ppUserObjects);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = QueryGroupsForUser(ppUserObjects[0]->userInfo.gid, ppUserObjects[0]->pszObjectSid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppUserObjects) {
        FreeObjectList(1, ppUserObjects);
    }

    FreeMCXValueList(pMCXValueList);
    FreeLWIGroup(pGroup);
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    return macError;
}

long
LWIQuery::QueryGroupsForUserById(
    IN uid_t uid
    )
{
    MACERROR macError = eDSNoErr;
    PLWIGROUP pGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    if (uid == USER_UID_GPO)
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, USER_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, USER_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIGroup("Group of Users managed by GPO",
                                      NULL, /* Name as queried */
                                      NULL, /* Password */
                                      "GPOUserGroup",
                                      USER_GROUP_COMMENT,
                                      NULL, /* Member user name - not set for this group */
                                      GROUP_GID_GPO_ID,
                                      GROUP_GID_GPO,
                                      pMCXValueList,
                                      &pGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = AddGroupRecord(pGroup, "GPOUserGroup");
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else
    {
        macError = GetUserObjectFromId(uid, &ppUserObjects);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = QueryGroupsForUser(ppUserObjects[0]->userInfo.gid, ppUserObjects[0]->pszObjectSid);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppUserObjects) {
        FreeObjectList(1, ppUserObjects);
    }

    FreeMCXValueList(pMCXValueList);
    FreeLWIGroup(pGroup);
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    return macError;
}

long
LWIQuery::GetGroupInformationById(
    IN gid_t gid
    )
{
    long macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;

    if (_pDirNode->fPlugInRootConnection)
    {
        macError = GetGroupObjectFromId(gid, &ppGroupObjects);
        GOTO_CLEANUP_ON_MACERROR(macError);

        if (ppGroupObjects[0]->enabled)
        {
            macError = AddGroupRecordHelper(ppGroupObjects[0], false, NULL);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else
    {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppGroupObjects)
    {
        FreeObjectList(1, ppGroupObjects);
    }

    return macError;
}

long
LWIQuery::GetGroupInformationByName(
    IN const char* pszName
    )
{
    long macError = eDSNoErr;
    PLWIGROUP pUserGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;

    if (!strcmp(pszName, "Group of Users managed by GPO"))
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, USER_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, USER_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIGroup("Group of Users managed by GPO",
                                      NULL, /* Name as queried */
                                      NULL, /* Password */
                                      "GPOUserGroup",
                                      USER_GROUP_COMMENT,
                                      NULL, /* Member user name - not set for this group */
                                      GROUP_GID_GPO_ID,
                                      GROUP_GID_GPO,
                                      pMCXValueList,
                                      &pUserGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = AddGroupRecord(pUserGroup, "GPOUserGroup");
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else if (_pDirNode->fPlugInRootConnection)
    {
        macError = GetGroupObjectFromName(pszName, &ppGroupObjects);
        GOTO_CLEANUP_ON_MACERROR(macError);

        if (ppGroupObjects[0]->enabled)
        {
            macError = AddGroupRecordHelper(ppGroupObjects[0], true, pszName);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else
    {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppGroupObjects)
    {
        FreeObjectList(1, ppGroupObjects);
    }

    FreeMCXValueList(pMCXValueList);
    GPA_SAFE_FREE_GPO_LIST(pGPO);
    FreeLWIGroup(pUserGroup);

    return macError;
}

long
LWIQuery::GetComputerListByName(
    IN const char* pszName,
    OUT PLWICOMPUTERLIST* ppComputerList
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTERLIST pComputerList = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PSTR pszGPOGUID = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PSTR pszHostname = NULL;

    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == true ||
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == true)
    {
       /* Computer Lists are only used on Tiger OS X, so we can return here */
        goto cleanup;
    }

    if (!strcmp(pszName, "GPOComputerList"))
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, MACHINE_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, MACHINE_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIComputerList("List of Computers managed by GPO",
                                             "GPOComputerList",
                                             COMPUTER_LIST_COMMENT,
                                             COMPUTER_LIST_UID_ID,
                                             COMPUTER_LIST_UID,
                                             "GPOComputer",
                                             pMCXValueList,
                                             &pComputerList);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else if (!strcmp(pszName, "List of Computers managed by GPO"))
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, MACHINE_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, MACHINE_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIComputerList("List of Computers managed by GPO",
                                              "GPOComputerList",
                                              COMPUTER_LIST_COMMENT,
                                              COMPUTER_LIST_UID_ID,
                                              COMPUTER_LIST_UID,
                                              "GPOComputer",
                                              pMCXValueList,
                                              &pComputerList);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else if (_pDirNode->fPlugInRootConnection)
    {
        char szPolicyPath[PATH_MAX] = { 0 };

        macError = LookupComputerListGPO(pszName, &pszGPOGUID);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        snprintf(szPolicyPath, sizeof(szPolicyPath), "%s/%s/%s", LWDS_GPO_CACHE_DIR, pszGPOGUID, LWDS_COMPUTER_MCX_CSE_GUID);
        
        macError = ConvertMCXSettingsToMCXValues(szPolicyPath, MACHINE_GROUP_POLICY, &pMCXValueList);
        GOTO_CLEANUP_ON_MACERROR(macError);
        
        macError = CreateLWIComputerList(pszName,
                                         pszName,
                                         COMPUTER_LIST_COMMENT,
                                         pszGPOGUID,
                                         UNSET_GID_UID_ID,
                                         pszHostname,
                                         pMCXValueList,
                                         &pComputerList);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppComputerList = pComputerList;
    pComputerList = NULL;
    
cleanup:

    /* All these free functions test for null parameter */
    FreeMCXValueList(pMCXValueList);
    GPA_SAFE_FREE_GPO_LIST(pGPO);
    LW_SAFE_FREE_STRING(pszGPOGUID);
    LW_SAFE_FREE_STRING(pszHostname);

    return macError;
}

long
LWIQuery::GetComputerGroupByName(
    IN const char* pszName,
    OUT PLWICOMPUTERGROUP* ppComputerGroup
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTERGROUP pComputerGroup = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PSTR pszGPOGUID = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PNETADAPTERINFO pTempNetInfo = _pNetAdapterList;
    PSTR pszHostname = NULL;

    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if ((_dwFlags & LWE_DS_FLAG_IS_LEOPARD) == false &&
         _dwFlags & LWE_DS_FLAG_IS_SNOW_LEOPARD) == false)
    {
       /* Computer Groups are only used on Leopard OS X, so we can return here */
        goto cleanup;
    }

    if (!strcmp(pszName, "GPOComputerGroup"))
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, MACHINE_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, MACHINE_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIComputerGroup("Group of Computers managed by GPO",
                                              "GPOComputerGroup",
                                              COMPUTER_GROUP_COMMENT,
                                              COMPUTER_GROUP_UID_ID,
                                              COMPUTER_GROUP_UID,
                                              "GPOComputer",
                                              COMPUTER_MAC_GPO,
                                              pMCXValueList,
                                              &pComputerGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else if (!strcmp(pszName, "Group of Computers managed by GPO"))
    {
        /* Fetch latest version of the GPO, so that we reflect current user or machine extensions that are enabled */
        if (_pDirNode && _pDirNode->pDirNodeGPO)
        {
            macError = GetSpecificGPO(NULL, _pDirNode->pDirNodeGPO->pszDisplayName, &pGPO);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (pGPO == NULL)
        {
            LOG("Failed to locate GPO in AD which represents the current DS node");
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        if (IsMCXSettingEnabledForGPO(pGPO, MACHINE_GROUP_POLICY))
        {
            macError = GetMCXValuesForGPOSettingType(pGPO, MACHINE_GROUP_POLICY, &pMCXValueList);
            GOTO_CLEANUP_ON_MACERROR(macError);

            macError = CreateLWIComputerGroup("Group of Computers managed by GPO",
                                              "GPOComputerGroup",
                                              COMPUTER_GROUP_COMMENT,
                                              COMPUTER_GROUP_UID_ID,
                                              COMPUTER_GROUP_UID,
                                              "GPOComputer",
                                              COMPUTER_MAC_GPO,
                                              pMCXValueList,
                                              &pComputerGroup);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }
    else if (_pDirNode->fPlugInRootConnection)
    {
        char szPolicyPath[PATH_MAX] = { 0 };

        macError = LookupComputerGroupGPO(pszName, &pszGPOGUID);
        GOTO_CLEANUP_ON_MACERROR(macError);
            
        snprintf(szPolicyPath, sizeof(szPolicyPath), "%s/%s/%s", LWDS_GPO_CACHE_DIR, pszGPOGUID, LWDS_COMPUTER_MCX_CSE_GUID);

        macError = ConvertMCXSettingsToMCXValues(szPolicyPath, MACHINE_GROUP_POLICY, &pMCXValueList);
        GOTO_CLEANUP_ON_MACERROR(macError);

        while (pTempNetInfo)
        {
            if (pTempNetInfo->pszName)
            {
                if (!strcmp(pTempNetInfo->pszName, "en0"))
                {
                    LOG("GetComputerGroupByName(%s) found primary en0 adapter...", pszName);
                    LOG("Name: %s", pTempNetInfo->pszName);
                    LOG("ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
                    LOG("IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
                    LOG("Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
                    LOG("Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
                    break;
                }
            }
            pTempNetInfo = pTempNetInfo->pNext;
        }
        
        macError = CreateLWIComputerGroup(pszName,
                                          pszName,
                                          COMPUTER_GROUP_COMMENT,
                                          pszGPOGUID,
                                          UNSET_GID_UID_ID,
                                          pszHostname,
                                          pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                          pMCXValueList,
                                          &pComputerGroup);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppComputerGroup = pComputerGroup;
    pComputerGroup = NULL;
    
cleanup:

    /* All these free functions test for null parameter */
    FreeMCXValueList(pMCXValueList);
    LW_SAFE_FREE_STRING(pszGPOGUID);
    LW_SAFE_FREE_STRING(pszHostname);
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    return macError;
}

long
LWIQuery::GetComputerByName(
    IN const char* pszName,
    OUT PLWICOMPUTER* ppComputer
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;
    PNETADAPTERINFO pTempNetInfo = _pNetAdapterList;
    PSTR pszHostname = NULL;

    while (pTempNetInfo)
    {
        if (pTempNetInfo->pszName)
        {
            if (!strcmp(pTempNetInfo->pszName, "en0"))
            {
                LOG("GetComputerByName(%s) found primary en0 adapter...", pszName);
                LOG("Name: %s", pTempNetInfo->pszName);
                LOG("ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
                LOG("IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
                LOG("Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
                LOG("Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
                break;
            }
        }    
        pTempNetInfo = pTempNetInfo->pNext;
    }
        
    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pszName)
    {
        if (!strcmp(pszName, pszHostname))
        {
            macError = CreateLWIComputer(pszHostname,
                                         pszHostname,
                                         COMPUTER_COMMENT_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                         pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                         pszHostname,
                                         NULL,
                                         &pComputer);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else if (!strcmp(pszName, "GPOComputer"))
        {
            macError = CreateLWIComputer("Computer managed by GPO",
                                         "GPOComputer",
                                         COMPUTER_COMMENT_GPO,
                                         COMPUTER_ID_GPO,
                                         COMPUTER_MAC_GPO,
                                         COMPUTER_LOOPBACK_IP,
                                         NULL, /* Not aliased, no keyword */
                                         NULL,
                                         &pComputer);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else if (!strcmp(pszName, "localhost"))
        {
            macError = CreateLWIComputer("Local Computer",
                                         "localhost",
                                         COMPUTER_COMMENT_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                         pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                         pszHostname,
                                         NULL,
                                         &pComputer);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else if (!strcmp(pszName, "Local Computer"))
        {
            macError = CreateLWIComputer("Local Computer",
                                         "localhost",
                                         COMPUTER_COMMENT_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                         pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                         pszHostname,
                                         NULL,
                                         &pComputer);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    } 
    else
    {
        if (_pDirNode->fPlugInRootConnection)
        {
            macError = CreateLWIComputer(pszHostname,
                                         pszHostname,
                                         COMPUTER_COMMENT_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                         pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                         pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                         pszHostname,
                                         NULL,
                                         &pComputer);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = CreateLWIComputer("Computer managed by GPO",
                                         "GPOComputer",
                                         COMPUTER_COMMENT_GPO,
                                         NULL, /* Generated ID */
                                         COMPUTER_MAC_GPO,
                                         COMPUTER_LOOPBACK_IP,
                                         NULL, /* Not aliased, no keyword */
                                         NULL,
                                         &pComputer);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

    *ppComputer = pComputer;
    pComputer = NULL;
    
cleanup:
        
    LW_SAFE_FREE_STRING(pszHostname);
    
    return macError;
}

long
LWIQuery::GetComputerByENetAddress(
    IN const char* pszENetAddress,
    OUT PLWICOMPUTER* ppComputer
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;
    PNETADAPTERINFO pTempNetInfo = _pNetAdapterList;
    PSTR pszHostname = NULL;
       
    while (pTempNetInfo)
    {
        if (pTempNetInfo->pszENetAddress)
        {
            if (!strcmp(pTempNetInfo->pszENetAddress, pszENetAddress))
            {
                LOG("GetComputerByENetAddress(%s) found adapter...", pszENetAddress);
                LOG("Name: %s", pTempNetInfo->pszName);
                LOG("ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
                LOG("IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
                LOG("Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
                LOG("Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
                break;
            }
        }    
        pTempNetInfo = pTempNetInfo->pNext;
    }
    
    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!_pDirNode->fPlugInRootConnection && !strcmp(pszENetAddress, COMPUTER_MAC_GPO))
    {
        macError = CreateLWIComputer("Computer managed by GPO",
                                     "GPOComputer",
                                     COMPUTER_COMMENT_GPO,
                                     COMPUTER_ID_GPO,
                                     COMPUTER_MAC_GPO,
                                     COMPUTER_LOOPBACK_IP,
                                     NULL, /* Not aliased, no keyword */
                                     NULL,
                                     &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (pTempNetInfo)
    {
        macError = CreateLWIComputer(pszHostname,
                                     pszHostname,
                                     COMPUTER_COMMENT_LOCAL,
                                     pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                     pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                     pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                     pszHostname,
                                     NULL,
                                     &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppComputer = pComputer;
    pComputer = NULL;
    
cleanup:
        
    LW_SAFE_FREE_STRING(pszHostname);

    return macError;
}

long
LWIQuery::GetComputerByIPAddress(
    IN const char* pszIPAddress,
    OUT PLWICOMPUTER* ppComputer
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;
    PNETADAPTERINFO pTempNetInfo = _pNetAdapterList;
    PSTR pszHostname = NULL;
        
    while (pTempNetInfo)
    {
        if (pTempNetInfo->pszIPAddress)
        {
            if (!strcmp(pTempNetInfo->pszIPAddress, pszIPAddress))
            {
                LOG("GetComputerByIPAddress(%s) found adapter...", pszIPAddress);
                LOG("Name: %s", pTempNetInfo->pszName);
                LOG("ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
                LOG("IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
                LOG("Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
                LOG("Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
                break;
            }
        }
        pTempNetInfo = pTempNetInfo->pNext;
    }

    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (!_pDirNode->fPlugInRootConnection && !strcmp(pszIPAddress, COMPUTER_LOOPBACK_IP))
    {
        macError = CreateLWIComputer("Computer managed by GPO",
                                     "GPOComputer",
                                     COMPUTER_COMMENT_GPO,
                                     COMPUTER_ID_GPO,
                                     COMPUTER_MAC_GPO,
                                     COMPUTER_LOOPBACK_IP,
                                     NULL, /* Not aliased, no keyword */
                                     NULL,
                                     &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (pTempNetInfo)
    {
        macError = CreateLWIComputer(pszHostname,
                                     pszHostname,
                                     COMPUTER_COMMENT_LOCAL,
                                     pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                     pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                     pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                     pszHostname,
                                     NULL,
                                     &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppComputer = pComputer;
    pComputer = NULL;
    
cleanup:
        
    LW_SAFE_FREE_STRING(pszHostname);
        
    return macError;
}

long
LWIQuery::GetComputerByGeneratedUID(
    IN const char* pszGeneratedUID,
    OUT PLWICOMPUTER* ppComputer
    )
{
    long macError = eDSNoErr;
    PLWICOMPUTER pComputer = NULL;
    PNETADAPTERINFO pTempNetInfo = _pNetAdapterList;
    PSTR pszHostname = NULL;

    macError = GetDnsHostName(&pszHostname);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (!strcmp(pszGeneratedUID, COMPUTER_ID_GPO))
    {
        macError = CreateLWIComputer("Computer managed by GPO",
                                     "GPOComputer",
                                     COMPUTER_COMMENT_GPO,
                                     COMPUTER_ID_GPO,
                                     COMPUTER_MAC_GPO,
                                     COMPUTER_LOOPBACK_IP,
                                     NULL, /* Not aliased, no keyword */
                                     NULL,
                                     &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (!strcmp(pszGeneratedUID, COMPUTER_LOOPBACK_IP))
    {
        while (pTempNetInfo)
        {
            if (pTempNetInfo->pszName)
            {
                if (!strcmp(pTempNetInfo->pszName, "en0"))
                {
                    LOG("GetComputerByGeneratedUID(%s) found primary en0 adapter...", pszGeneratedUID);
                    LOG("Name: %s", pTempNetInfo->pszName);
                    LOG("ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
                    LOG("IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
                    LOG("Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
                    LOG("Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");
                    break;
                }
            }
            pTempNetInfo = pTempNetInfo->pNext;
        }

        macError = CreateLWIComputer(pszHostname,
                                     pszHostname,
                                     COMPUTER_COMMENT_LOCAL,
                                     pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                     pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                     pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                     pszHostname,
                                     NULL,
                                     &pComputer);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        while (pTempNetInfo)
        {
            if (pTempNetInfo->pszENetAddress)
            {
                if (!strcmp(pTempNetInfo->pszENetAddress, pszGeneratedUID))
                {
                    LOG("GetComputerByGeneratedUID(%s) found adapter...", pszGeneratedUID);
                    LOG("Name: %s", pTempNetInfo->pszName);
                    LOG("ENet: %s", pTempNetInfo->pszENetAddress ? pTempNetInfo->pszENetAddress : "----");
                    LOG("IP: %s", pTempNetInfo->pszIPAddress ? pTempNetInfo->pszIPAddress : "----");
                    LOG("Up: %s", pTempNetInfo->IsUp ? "yes" : "no");
                    LOG("Running: %s", pTempNetInfo->IsRunning ? "yes" : "no");

                    macError = CreateLWIComputer(pszHostname,
                                                 pszHostname,
                                                 COMPUTER_COMMENT_LOCAL,
                                                 pTempNetInfo ? pTempNetInfo->pszENetAddress : COMPUTER_ID_LOCAL,
                                                 pTempNetInfo ? pTempNetInfo->pszENetAddress : NULL,
                                                 pTempNetInfo ? pTempNetInfo->pszIPAddress : COMPUTER_LOOPBACK_IP,
                                                 pszHostname,
                                                 NULL,
                                                 &pComputer);
                    GOTO_CLEANUP_ON_MACERROR(macError);
                    break;
                }
            }
            pTempNetInfo = pTempNetInfo->pNext;
        }

        if (!pTempNetInfo)
        {
            macError = eDSRecordNotFound;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

    *ppComputer = pComputer;
    pComputer = NULL;
    
cleanup:
        
    LW_SAFE_FREE_STRING(pszHostname);
        
    return macError;
}

long
LWIQuery::SetDistinguishedName(PDSRECORD pRecord, const char* pszName, const char* pszNameAsQueried, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    // Users distinguished or real name, we also return this as the following for compatibility with Mac AD plugin:
    // K_DS_ATTR_DISPLAY_NAME
    // K_DS_ATTR_CN
    // K_DS_ATTR_NAME
    
    if (pszName)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrDistinguishedName, pszName, pRecord, &pAttribute);
            if (macError) goto exit;

            if (pszNameAsQueried && strcmp(pszName, pszNameAsQueried))
            {
                LOG("Returning addional realname %s", pszNameAsQueried);
                macError = SetAttributeValue(pAttribute, pszNameAsQueried);
                if (macError) goto exit;
            }
            
            macError = AddAttributeAndValue(K_DS_ATTR_DISPLAY_NAME, pszName, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_CN, pszName, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_NAME, pszName, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDS1AttrDistinguishedName, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_DISPLAY_NAME, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_CN, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_NAME, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }

exit:
    
    return macError;
}

long
LWIQuery::SetGeneratedUID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // Used for 36 character (128 bit ) Unique ID. Usually found in user, group and computer records.
    // An example value is "A579E95E-CDFE-4EBC-B7E7-F2158562170F"
    // The standard format contains 32 hex characters and four hyphen characters.
    long macError = eDSNoErr;
    char* pszGUID = NULL;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue && pUser)
    {
        macError = BuildGeneratedUID(pUser->pw_uid, &pszGUID);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddAttributeAndValue(kDS1AttrGeneratedUID, pszGUID, pRecord, &pAttribute);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = AddAttribute(kDS1AttrGeneratedUID, pRecord, &pAttribute);
    }

cleanup:

    if (pszGUID)
    {
        LW_SAFE_FREE_STRING(pszGUID);
    }

    return eDSNoErr;
}

long
LWIQuery::SetGeneratedUID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    // Used for 36 character (128 bit ) Unique ID. Usually found in user, group and computer records.
    // An example value is "A579E95E-CDFE-4EBC-B7E7-F2158562170F"
    // The standard format contains 32 hex characters and four hyphen characters.
    long macError = eDSNoErr;
    char* pszGUID = NULL;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue && pGroup)
    {
        if (pGroup->guid && pGroup->gr_gid == UNSET_GID_UID_ID)
        {
            macError = LwAllocateString(pGroup->guid, &pszGUID);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = BuildGeneratedGID(pGroup->gr_gid, &pszGUID);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        macError = AddAttributeAndValue(kDS1AttrGeneratedUID, pszGUID, pRecord, &pAttribute);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = AddAttribute(kDS1AttrGeneratedUID, pRecord, &pAttribute);
    }

cleanup:

    if (pszGUID)
    {
        LW_SAFE_FREE_STRING(pszGUID);
    }

    return eDSNoErr;
}

long
LWIQuery::SetGeneratedUID(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    // Used for 36 character (128 bit ) Unique ID. Usually found in user, group and computer records.
    // An example value is "A579E95E-CDFE-4EBC-B7E7-F2158562170F"
    // The standard format contains 32 hex characters and four hyphen characters.
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputer && pComputer->guid)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrGeneratedUID, pComputer->guid, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrGeneratedUID, pRecord, &pAttribute);
        }
    }

cleanup:

    return eDSNoErr;
}

long
LWIQuery::SetGeneratedUID(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    // Used for 36 character (128 bit ) Unique ID. Usually found in user, group and computer records.
    // An example value is "A579E95E-CDFE-4EBC-B7E7-F2158562170F"
    // The standard format contains 32 hex characters and four hyphen characters.
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerList && pComputerList->guid)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrGeneratedUID, pComputerList->guid, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrGeneratedUID, pRecord, &pAttribute);
        }
    }

cleanup:

    return eDSNoErr;
}

long
LWIQuery::SetGeneratedUID(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    // Used for 36 character (128 bit ) Unique ID. Usually found in user, group and computer records.
    // An example value is "A579E95E-CDFE-4EBC-B7E7-F2158562170F"
    // The standard format contains 32 hex characters and four hyphen characters.
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerGroup && pComputerGroup->guid)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrGeneratedUID, pComputerGroup->guid, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrGeneratedUID, pRecord, &pAttribute);
        }
    }

cleanup:

    return eDSNoErr;
}

long
LWIQuery::SetNFSHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // kDS1AttrNFSHomeDirectory
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser && pUser->pw_nfs_home_dir)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrNFSHomeDirectory, pUser->pw_nfs_home_dir, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttribute(kDS1AttrNFSHomeDirectory, pRecord, &pAttribute);
        }
    }

    return macError;
}

long
LWIQuery::SetHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // kDSNAttrHomeDirectory
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser && pUser->pw_home_dir)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrHomeDirectory, pUser->pw_home_dir, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttribute(kDSNAttrHomeDirectory, pRecord, &pAttribute);
        }
    }

    return macError;
}

long
LWIQuery::SetOriginalHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // kDSNAttrOriginalHomeDirectory
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser && pUser->pw_orig_home_dir)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrOriginalHomeDirectory, pUser->pw_orig_home_dir, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttribute(kDSNAttrOriginalHomeDirectory, pRecord, &pAttribute);
        }
    }

    return macError;
}

long
LWIQuery::SetOriginalNFSHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // kDS1AttrOriginalNFSHomeDirectory
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser && pUser->pw_orig_nfs_home_dir)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrOriginalNFSHomeDirectory, pUser->pw_orig_nfs_home_dir, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttribute(kDS1AttrOriginalNFSHomeDirectory, pRecord, &pAttribute);
        }
    }

    return macError;
}

long
LWIQuery::SetPassword(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // Holds the password or credential value
    long macError = eDSNoErr;
    return macError;
}

long
LWIQuery::SetPassword(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    // Holds the password or credential value
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pGroup && pGroup->gr_passwd)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrPassword, pGroup->gr_passwd, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttribute(kDS1AttrPassword, pRecord, &pAttribute);
        }
    }

    return macError;
}

long
LWIQuery::SetPasswordPlus(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // Holds marker data to indicate possible authentication redirection
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrPasswordPlus, kDSValueNonCryptPasswordMarker, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttribute(kDS1AttrPasswordPlus, pRecord, &pAttribute);
        }
    }

    return macError;
}

long
LWIQuery::SetPasswordPolicyOptions(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    return eDSNoErr;
}

long
LWIQuery::SetPrimaryGroupID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser->pw_gid == UNSET_GID_UID_ID)
        return macError;
        
    if (bSetValue && pUser)
    {
        char szBuf[128];

        sprintf(szBuf, "%d", pUser->pw_gid);
        macError = AddAttributeAndValue(kDS1AttrPrimaryGroupID,
                                        szBuf,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrPrimaryGroupID, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetPrimaryGroupID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pGroup->gr_gid == UNSET_GID_UID_ID)
        return macError;
        
    if (bSetValue && pGroup)
    {
        char szBuf[128];

        sprintf(szBuf, "%d", pGroup->gr_gid);
        macError = AddAttributeAndValue(kDS1AttrPrimaryGroupID,
                                        szBuf,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrPrimaryGroupID, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetPrimaryGroupID(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerList->primaryId == UNSET_GID_UID_ID)
        return macError;
        
    if (bSetValue && pComputerList)
    {
        char szBuf[128];

        sprintf(szBuf, "%d", pComputerList->primaryId);
        macError = AddAttributeAndValue(kDS1AttrPrimaryGroupID,
                                        szBuf,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrPrimaryGroupID, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetPrimaryGroupID(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerGroup->primaryId == UNSET_GID_UID_ID)
        return macError;
        
    if (bSetValue && pComputerGroup)
    {
        char szBuf[128];

        sprintf(szBuf, "%d", pComputerGroup->primaryId);
        macError = AddAttributeAndValue(kDS1AttrPrimaryGroupID,
                                        szBuf,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrPrimaryGroupID, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetUniqueID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue && pUser)
    {
        char szBuf[128];

        sprintf(szBuf, "%d", pUser->pw_uid);
        macError = AddAttributeAndValue(kDS1AttrUniqueID,
                                        szBuf,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrUniqueID, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetUniqueID(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue && pGroup)
    {
        char szBuf[128];

        sprintf(szBuf, "%d", pGroup->gr_gid);
        macError = AddAttributeAndValue(kDS1AttrUniqueID,
                                        szBuf,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrUniqueID, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetUserShell(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser && pUser->pw_shell)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrUserShell, pUser->pw_shell, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttribute(kDS1AttrUserShell, pRecord, &pAttribute);
        }
    }

    return macError;
}

long
LWIQuery::SetGroupMembership(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    return eDSNoErr;
}

long
LWIQuery::SetGroupMembership(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = AddAttribute(kDSNAttrGroupMembership, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pGroup)
    {
        if (pGroup->gr_membership)
        {
            for (int index = 0; pGroup->gr_membership[index] && *(pGroup->gr_membership[index]); index++)
            {
                macError = SetAttributeValue(pAttribute, pGroup->gr_membership[index]);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetGroupMembership(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = AddAttribute(kDSNAttrGroupMembership, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerGroup)
    {
        if (pComputerGroup->membership)
        {
            for (int index = 0; pComputerGroup->membership[index] && *(pComputerGroup->membership[index]); index++)
            {
                macError = SetAttributeValue(pAttribute, pComputerGroup->membership[index]);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetGroupMembers(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = AddAttribute(kDSNAttrGroupMembers, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pGroup)
    {
        if (pGroup->gr_members)
        {
            for (int index = 0; pGroup->gr_members[index] && *(pGroup->gr_members[index]); index++)
            {
                macError = SetAttributeValue(pAttribute, pGroup->gr_members[index]);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetGroupMembers(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = AddAttribute(kDSNAttrGroupMembers, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerGroup)
    {
        if (pComputerGroup->members)
        {
            for (int index = 0; pComputerGroup->members[index] && *(pComputerGroup->members[index]); index++)
            {
                macError = SetAttributeValue(pAttribute, pComputerGroup->members[index]);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetPasswordAgingPolicy(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    return eDSNoErr;
}

long
LWIQuery::SetPasswordChange(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    return macError;
}

long
LWIQuery::SetPasswordExpire(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    return macError;
}

long
LWIQuery::SetMetaNodeLocation(PDSRECORD pRecord, PSTR pszPath, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDSNAttrMetaNodeLocation,
                                        pszPath ? pszPath : PLUGIN_ROOT_PATH,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDSNAttrMetaNodeLocation, pRecord, &pAttribute);
    }

    return macError;
}

/*!
 * @defined kDS1AttrTimeToLive
 * @discussion Attribute recommending how long to cache the record's attribute values.
 * Format is a decimal string number of seconds (e.g. "300" is 5 minutes).
 */
long
LWIQuery::SetTimeToLive(PDSRECORD pRecord, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        char number[10];

        snprintf(number, sizeof(number), "%u", DEFAULT_ATTRIBUTE_TTL_SECONDS);
        macError = AddAttributeAndValue(kDS1AttrTimeToLive,
                                        number,
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrTimeToLive, pRecord, &pAttribute);
    }

    return macError;
}

#define HAS_MCX_SETTINGS "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\">\n<dict>\n	<key>has_mcx_settings</key>\n		<true/>\n	</dict>\n</plist>"
#define HAS_NO_MCX_SETTINGS "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\">\n<dict>\n	<key>has_mcx_settings</key>\n		<false/>\n	</dict>\n</plist>"

long
LWIQuery::SetMCXFlags(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        if (pUser && pUser->pMCXValues)
        {    
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_MCX_SETTINGS, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_NO_MCX_SETTINGS, pRecord, &pAttribute);
        }
    }
    else
    {
        macError = AddAttribute(kDS1AttrMCXFlags, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetMCXFlags(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        if (pGroup && pGroup->pMCXValues)
        {    
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_MCX_SETTINGS, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_NO_MCX_SETTINGS, pRecord, &pAttribute);
        }
    }
    else
    {
        macError = AddAttribute(kDS1AttrMCXFlags, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetMCXFlags(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        if (pComputer && pComputer->pMCXValues)
        {    
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_MCX_SETTINGS, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_NO_MCX_SETTINGS, pRecord, &pAttribute);
        }
    }
    else
    {
        macError = AddAttribute(kDS1AttrMCXFlags, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetMCXFlags(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        if (pComputerList && pComputerList->pMCXValues)
        {
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_MCX_SETTINGS, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_NO_MCX_SETTINGS, pRecord, &pAttribute);
        }
    }
    else
    {
        macError = AddAttribute(kDS1AttrMCXFlags, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetMCXFlags(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        if (pComputerGroup && pComputerGroup->pMCXValues)
        {
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_MCX_SETTINGS, pRecord, &pAttribute);
        }
        else
        {
            macError = AddAttributeAndValue(kDS1AttrMCXFlags, HAS_NO_MCX_SETTINGS, pRecord, &pAttribute);
        }
    }
    else
    {
        macError = AddAttribute(kDS1AttrMCXFlags, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetMCXSettings(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser && pUser->pMCXValues)
    {
        if (bSetValue)
        {
            PMCXVALUE pMCXValue = pUser->pMCXValues->pNext;

            macError = AddAttributeAndValue(kDS1AttrMCXSettings,
                                            pUser->pMCXValues->pValueData,
                                            pUser->pMCXValues->iValLen,
                                            pRecord,
                                            &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);

            while (pMCXValue)
            {
                SetAttributeValue(pAttribute, pMCXValue->pValueData, pMCXValue->iValLen);
                GOTO_CLEANUP_ON_MACERROR(macError);

                pMCXValue = pMCXValue->pNext;
            }
        }
        else
        {
            macError = AddAttribute(kDS1AttrMCXSettings, pRecord, &pAttribute);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetMCXSettings(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pGroup && pGroup->pMCXValues)
    {
        if (bSetValue)
        {
            PMCXVALUE pMCXValue = pGroup->pMCXValues->pNext;

            macError = AddAttributeAndValue(kDS1AttrMCXSettings,
                                            pGroup->pMCXValues->pValueData,
                                            pGroup->pMCXValues->iValLen,
                                            pRecord,
                                            &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);

            while (pMCXValue)
            {
                SetAttributeValue(pAttribute, pMCXValue->pValueData, pMCXValue->iValLen);
                GOTO_CLEANUP_ON_MACERROR(macError);

                pMCXValue = pMCXValue->pNext;
            }
        }
        else
        {
            macError = AddAttribute(kDS1AttrMCXSettings, pRecord, &pAttribute);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetMCXSettings(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputer && pComputer->pMCXValues)
    {
        if (bSetValue)
        {
            PMCXVALUE pMCXValue = pComputer->pMCXValues->pNext;

            macError = AddAttributeAndValue(kDS1AttrMCXSettings,
                                            pComputer->pMCXValues->pValueData,
                                            pComputer->pMCXValues->iValLen,
                                            pRecord,
                                            &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);

            while (pMCXValue)
            {
                SetAttributeValue(pAttribute, pMCXValue->pValueData, pMCXValue->iValLen);
                GOTO_CLEANUP_ON_MACERROR(macError);

                pMCXValue = pMCXValue->pNext;
            }
        }
        else
        {
            macError = AddAttribute(kDS1AttrMCXSettings, pRecord, &pAttribute);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetMCXSettings(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerList && pComputerList->pMCXValues)
    {
        if (bSetValue)
        {
            PMCXVALUE pMCXValue = pComputerList->pMCXValues->pNext;

            macError = AddAttributeAndValue(kDS1AttrMCXSettings,
                                            pComputerList->pMCXValues->pValueData,
                                            pComputerList->pMCXValues->iValLen,
                                            pRecord,
                                            &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);

            while (pMCXValue)
            {
                SetAttributeValue(pAttribute, pMCXValue->pValueData, pMCXValue->iValLen);
                GOTO_CLEANUP_ON_MACERROR(macError);

                pMCXValue = pMCXValue->pNext;
            }
        }
        else
        {
            macError = AddAttribute(kDS1AttrMCXSettings, pRecord, &pAttribute);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetMCXSettings(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerGroup && pComputerGroup->pMCXValues)
    {
        if (bSetValue)
        {
            PMCXVALUE pMCXValue = pComputerGroup->pMCXValues->pNext;

            macError = AddAttributeAndValue(kDS1AttrMCXSettings,
                                            pComputerGroup->pMCXValues->pValueData,
                                            pComputerGroup->pMCXValues->iValLen,
                                            pRecord,
                                            &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);

            while (pMCXValue)
            {
                SetAttributeValue(pAttribute, pMCXValue->pValueData, pMCXValue->iValLen);
                GOTO_CLEANUP_ON_MACERROR(macError);

                pMCXValue = pMCXValue->pNext;
            }
        }
        else
        {
            macError = AddAttribute(kDS1AttrMCXSettings, pRecord, &pAttribute);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetComment(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pGroup && pGroup->comment)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrComment, pGroup->comment, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrComment, pRecord, &pAttribute);
        }
    }

cleanup:

    return eDSNoErr;
}

long
LWIQuery::SetComment(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputer && pComputer->comment)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrComment, pComputer->comment, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrComment, pRecord, &pAttribute);
        }
    }

cleanup:

    return eDSNoErr;
}

long
LWIQuery::SetComment(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerList && pComputerList->comment)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrComment, pComputerList->comment, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrComment, pRecord, &pAttribute);
        }
    }

cleanup:

    return eDSNoErr;
}

long
LWIQuery::SetComment(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputerGroup && pComputerGroup->comment)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrComment, pComputerGroup->comment, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrComment, pRecord, &pAttribute);
        }
    }

cleanup:

    return eDSNoErr;
}

long
LWIQuery::SetComputers(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrComputers, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerList)
    {
        if (pComputerList->computers)
        {
            for (int index = 0; pComputerList->computers[index] && *(pComputerList->computers[index]); index++)
            {
                macError = SetAttributeValue(pAttribute, pComputerList->computers[index]);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetComputers(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrComputers, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerGroup)
    {
        if (pComputerGroup->computers)
        {
            for (int index = 0; pComputerGroup->computers[index] && *(pComputerGroup->computers[index]); index++)
            {
                macError = SetAttributeValue(pAttribute, pComputerGroup->computers[index]);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetShortName(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordName, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputer && pComputer->shortname)
    {
        macError = SetAttributeValue(pAttribute, pComputer->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::SetShortName(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordName, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerList && pComputerList->shortname)
    {
        macError = SetAttributeValue(pAttribute, pComputerList->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::SetShortName(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordName, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerGroup && pComputerGroup->shortname)
    {
        macError = SetAttributeValue(pAttribute, pComputerGroup->shortname);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::SetRealName(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDS1AttrDistinguishedName, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputer && pComputer->name)
    {
        macError = SetAttributeValue(pAttribute, pComputer->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::SetRealName(PDSRECORD pRecord, const PLWICOMPUTERLIST pComputerList, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDS1AttrDistinguishedName, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerList && pComputerList->name)
    {
        macError = SetAttributeValue(pAttribute, pComputerList->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::SetRealName(PDSRECORD pRecord, const PLWICOMPUTERGROUP pComputerGroup, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDS1AttrDistinguishedName, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputerGroup && pComputerGroup->name)
    {
        macError = SetAttributeValue(pAttribute, pComputerGroup->name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::SetENetAddress(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputer && pComputer->ethernetID)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrENetAddress, pComputer->ethernetID, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDS1AttrENetAddress, pRecord, &pAttribute);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetIPAddress(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pComputer && pComputer->IPaddress)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrIPAddress, pComputer->IPaddress, pRecord, &pAttribute);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        else
        {
            macError = AddAttribute(kDSNAttrIPAddress, pRecord, &pAttribute);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetKeywords(PDSRECORD pRecord, const PLWICOMPUTER pComputer, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrKeywords, NULL);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (bSetValue && pComputer && pComputer->keywords)
    {
        for (int index = 0; pComputer->keywords[index] && *(pComputer->keywords[index]); index++)
        {
            macError = SetAttributeValue(pAttribute, pComputer->keywords[index]);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::SetFirstName(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
   
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszFirstName)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrFirstName, pUser->padUserInfo->pszFirstName, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttributeAndValue(K_DS_ATTR_GIVEN_NAME, pUser->padUserInfo->pszFirstName, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDS1AttrFirstName, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_GIVEN_NAME, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetLastName(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszLastName)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDS1AttrLastName, pUser->padUserInfo->pszLastName, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttributeAndValue(K_DS_ATTR_SN, pUser->padUserInfo->pszLastName, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDS1AttrLastName, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_SN, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetDomain(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszADDomain)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_AD_DOMAIN, pUser->padUserInfo->pszADDomain, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_AD_DOMAIN, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetKerberosPrincipal(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszKerberosPrincipal)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_KERBEROS_PRINCIPAL, pUser->padUserInfo->pszKerberosPrincipal, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttributeAndValue(K_DS_ATTR_USER_PRINCIPAL_NAME, pUser->padUserInfo->pszKerberosPrincipal, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_KERBEROS_PRINCIPAL, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_USER_PRINCIPAL_NAME, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetEMail(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszEMailAddress)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrEMailAddress, pUser->padUserInfo->pszEMailAddress, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_EMAIL_ADDRESS, pUser->padUserInfo->pszEMailAddress, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrEMailAddress, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_EMAIL_ADDRESS, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }

    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszMSExchHomeServerName)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_MS_EXCH_HOME_SERVER_NAME, pUser->padUserInfo->pszMSExchHomeServerName, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_MS_EXCH_HOME_SERVER_NAME, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszMSExchHomeMDB)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_MS_EXCH_HOME_MDB, pUser->padUserInfo->pszMSExchHomeMDB, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_MS_EXCH_HOME_MDB, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetPhone(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszTelephoneNumber)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrPhoneNumber, pUser->padUserInfo->pszTelephoneNumber, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_TELEPHONE_NUMBER, pUser->padUserInfo->pszTelephoneNumber, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrPhoneNumber, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_TELEPHONE_NUMBER, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszFaxTelephoneNumber)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrFaxNumber, pUser->padUserInfo->pszFaxTelephoneNumber, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_FACSIMILIE_TELEPHONE_NUMBER, pUser->padUserInfo->pszFaxTelephoneNumber, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrFaxNumber, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_FACSIMILIE_TELEPHONE_NUMBER, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszMobileTelephoneNumber)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrMobileNumber, pUser->padUserInfo->pszMobileTelephoneNumber, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_MOBILE, pUser->padUserInfo->pszMobileTelephoneNumber, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrMobileNumber, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_MOBILE, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetAddress(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszStreetAddress)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrStreet, pUser->padUserInfo->pszStreetAddress, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_STREET_ADDRESS, pUser->padUserInfo->pszStreetAddress, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrStreet, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_STREET_ADDRESS, pRecord, &pAttribute);
            if (macError) goto exit;            
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszPostOfficeBox)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_POST_OFFICE_BOX, pUser->padUserInfo->pszPostOfficeBox, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_POST_OFFICE_BOX, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszCity)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrCity, pUser->padUserInfo->pszCity, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_CITY, pUser->padUserInfo->pszCity, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrCity, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_CITY, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszState)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrState, pUser->padUserInfo->pszState, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_STATE, pUser->padUserInfo->pszState, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrState, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_STATE, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszPostalCode)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrPostalCode, pUser->padUserInfo->pszPostalCode, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_POSTAL_CODE, pUser->padUserInfo->pszPostalCode, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrPostalCode, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_POSTAL_CODE, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszCountry)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrCountry, pUser->padUserInfo->pszCountry, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttributeAndValue(K_DS_ATTR_COUNTRY, pUser->padUserInfo->pszCountry, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrCountry, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_COUNTRY, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetWork(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszTitle)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(kDSNAttrJobTitle, pUser->padUserInfo->pszTitle, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_TITLE, pUser->padUserInfo->pszTitle, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(kDSNAttrJobTitle, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_TITLE, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszCompany)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_COMPANY_AD, pUser->padUserInfo->pszCompany, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_COMPANY_MAC, pUser->padUserInfo->pszCompany, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_COMPANY_AD, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_COMPANY_MAC, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszDepartment)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_DEPARTMENT, pUser->padUserInfo->pszDepartment, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(kDSNAttrDepartment, pUser->padUserInfo->pszDepartment, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_DEPARTMENT, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(kDSNAttrDepartment, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetProfile(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;    
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszHomeDirectory)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_HOME_DIRECTORY, pUser->padUserInfo->pszHomeDirectory, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_HOME_DIRECTORY, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszHomeDrive)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_HOME_DRIVE, pUser->padUserInfo->pszHomeDrive, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_HOME_DRIVE, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

long
LWIQuery::SetLogon(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;    

    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszPasswordLastSet)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_PWD_LAST_SET, pUser->padUserInfo->pszPasswordLastSet, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_PWD_LAST_SET, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszUserAccountControl)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_USER_ACCOUNT_CONTROL, pUser->padUserInfo->pszUserAccountControl, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_USER_ACCOUNT_CONTROL, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

void
ConvertTimeUnix2Nt(
    unsigned long long unixTime,
    unsigned long long * pNtTime
    )
{
    unsigned long long ntTime = 0;

    ntTime = (unixTime+11644473600LL)*10000000LL;

    *pNtTime = ntTime;
}

long
LWIQuery::SetPolicy(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    char* pszEndPtr = NULL;
    long long MaxPwdAge = 0;
    long long MinPwdAge = 0;
    unsigned long long qwPwdLastSet = 0;
    long MaxPwdAgeDays = 0;
    long MinPwdAgeDays = 0;
    long DaysTillPwdExpiry = 0;
    char szMaxPwdAgeDays[50] = { 0 };
    char szMinPwdAgeDays[50] = { 0 };
    char szDaysTillPwdExpiry[50] = { 0 };
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszMaxMinutesUntilChangePassword)
    {
        if (bSetValue)
        {
            MaxPwdAge = strtoll(pUser->padUserInfo->pszMaxMinutesUntilChangePassword, &pszEndPtr, 10);
            if (pszEndPtr == NULL ||
                pszEndPtr == pUser->padUserInfo->pszMaxMinutesUntilChangePassword ||
                *pszEndPtr != '\0')
            {
                MaxPwdAge = 0;
                MaxPwdAgeDays = 0;
            }
            else
            {
                if (MaxPwdAge < 0)
                    MaxPwdAge = 0 - MaxPwdAge;

                MaxPwdAgeDays = (long) (MaxPwdAge / (10000000LL*24*60*60));
            }

            sprintf(szMaxPwdAgeDays, "%ld", MaxPwdAgeDays);

            macError = AddAttributeAndValue(K_DS_ATTR_MAX_MINUTES_UNTIL_CHANGE_PASSWORD, pUser->padUserInfo->pszMaxMinutesUntilChangePassword, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttributeAndValue(K_DS_ATTR_MAX_PWD_AGE, pUser->padUserInfo->pszMaxMinutesUntilChangePassword, pRecord, &pAttribute);
            if (macError) goto exit;
 
            macError = AddAttributeAndValue(K_DS_ATTR_MAX_PWD_AGE_DAYS, szMaxPwdAgeDays, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_MAX_MINUTES_UNTIL_CHANGE_PASSWORD, pRecord, &pAttribute);
            if (macError) goto exit;
 
            macError = AddAttribute(K_DS_ATTR_MAX_PWD_AGE, pRecord, &pAttribute);
            if (macError) goto exit;
 
            macError = AddAttribute(K_DS_ATTR_MAX_PWD_AGE_DAYS, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
 
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszMinMinutesUntilChangePassword)
    {
        if (bSetValue)
        {
            MinPwdAge = strtoll(pUser->padUserInfo->pszMinMinutesUntilChangePassword, &pszEndPtr, 10);
            if (pszEndPtr == NULL ||
                pszEndPtr == pUser->padUserInfo->pszMinMinutesUntilChangePassword ||
                *pszEndPtr != '\0')
            {
                MinPwdAge = 0;
                MinPwdAgeDays = 0;
            }
            else
            {
                if (MinPwdAge < 0)
                    MinPwdAge = 0 - MinPwdAge;

                MinPwdAgeDays = (long) (MinPwdAge / (10000000LL*24*60*60));
            }

            sprintf(szMinPwdAgeDays, "%ld", MinPwdAgeDays);

            macError = AddAttributeAndValue(K_DS_ATTR_MIN_MINUTES_UNTIL_CHANGE_PASSWORD, pUser->padUserInfo->pszMinMinutesUntilChangePassword, pRecord, &pAttribute);
            if (macError) goto exit;
 
            macError = AddAttributeAndValue(K_DS_ATTR_MIN_PWD_AGE, pUser->padUserInfo->pszMinMinutesUntilChangePassword, pRecord, &pAttribute);
            if (macError) goto exit;
 
            macError = AddAttributeAndValue(K_DS_ATTR_MIN_PWD_AGE_DAYS, szMinPwdAgeDays, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_MIN_MINUTES_UNTIL_CHANGE_PASSWORD, pRecord, &pAttribute);
            if (macError) goto exit;
 
            macError = AddAttribute(K_DS_ATTR_MIN_PWD_AGE, pRecord, &pAttribute);
            if (macError) goto exit;
 
            macError = AddAttribute(K_DS_ATTR_MIN_PWD_AGE_DAYS, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
 
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszPasswordLastSet && MaxPwdAge > 0)
    {
        if (bSetValue)
        {
            qwPwdLastSet = strtoull(pUser->padUserInfo->pszPasswordLastSet, &pszEndPtr, 10);
            if (pszEndPtr == NULL ||
                pszEndPtr == pUser->padUserInfo->pszPasswordLastSet ||
                *pszEndPtr != '\0')
            {
                qwPwdLastSet = 0;
                DaysTillPwdExpiry = 0;
            }
            else
            {
                struct timeval current_tv;
                unsigned long long qwCurrentTime = 0;
                long long NanosecsToPwdExpiry = 0;

                if (gettimeofday(&current_tv, NULL) >= 0)
                {
                    ConvertTimeUnix2Nt(current_tv.tv_sec, &qwCurrentTime);

                    NanosecsToPwdExpiry = MaxPwdAge - (qwCurrentTime - qwPwdLastSet);

                    DaysTillPwdExpiry = (long)(NanosecsToPwdExpiry / (10000000LL*24*60*60));
                }
            }

            sprintf(szDaysTillPwdExpiry, "%ld", DaysTillPwdExpiry);

            macError = AddAttributeAndValue(K_DS_ATTR_DAYS_TILL_PWD_EXPIRES,
                                            szDaysTillPwdExpiry,
                                            pRecord,
                                            &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_DAYS_TILL_PWD_EXPIRES, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszMaxFailedLoginAttempts)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_MAX_FAILED_LOGIN_ATTEMPTS, pUser->padUserInfo->pszMaxFailedLoginAttempts, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttributeAndValue(K_DS_ATTR_LOCKOUT_THRESHHOLD, pUser->padUserInfo->pszMaxFailedLoginAttempts, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_MAX_FAILED_LOGIN_ATTEMPTS, pRecord, &pAttribute);
            if (macError) goto exit;

            macError = AddAttribute(K_DS_ATTR_LOCKOUT_THRESHHOLD, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszAllowedPasswordHistory)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_USING_HISTORY, pUser->padUserInfo->pszAllowedPasswordHistory, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttributeAndValue(K_DS_ATTR_PWD_HISTORY_LENGTH, pUser->padUserInfo->pszAllowedPasswordHistory, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_USING_HISTORY, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_PWD_HISTORY_LENGTH, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
    if (pUser && pUser->padUserInfo && pUser->padUserInfo->pszMinCharsAllowedInPassword)
    {
        if (bSetValue)
        {
            macError = AddAttributeAndValue(K_DS_ATTR_MIN_CHARS, pUser->padUserInfo->pszMinCharsAllowedInPassword, pRecord, &pAttribute);
            if (macError) goto exit;
        
            macError = AddAttributeAndValue(K_DS_ATTR_MIN_PWD_LENGTH, pUser->padUserInfo->pszMinCharsAllowedInPassword, pRecord, &pAttribute);
            if (macError) goto exit;
        }
        else
        {
            macError = AddAttribute(K_DS_ATTR_MIN_CHARS, pRecord, &pAttribute);
            if (macError) goto exit;
            
            macError = AddAttribute(K_DS_ATTR_MIN_PWD_LENGTH, pRecord, &pAttribute);
            if (macError) goto exit;
        }
    }
    
exit:
    
    return macError;
}

void
LWIQuery::FreeAttributeValue(
    PDSATTRIBUTEVALUE pAttributeValue
    )
{
    if (pAttributeValue->pszValue)
    {
        // The value does not have to be a string
        LwFreeMemory(pAttributeValue->pszValue);
    }
    LwFreeMemory(pAttributeValue);
}

void
LWIQuery::FreeAttributeValueList(
    PDSATTRIBUTEVALUE pAttributeValueList
    )
{
    PDSATTRIBUTEVALUE pAttrValue = NULL;
    while (pAttributeValueList)
    {
        pAttrValue = pAttributeValueList;
        pAttributeValueList = pAttributeValueList->pNext;

        FreeAttributeValue(pAttrValue);
    }
}

void
LWIQuery::FreeAttribute(
    PDSATTRIBUTE pAttribute
    )
{
    if (pAttribute->pValueListHead)
    {
       FreeAttributeValueList(pAttribute->pValueListHead);
    }

    if (pAttribute->pszName)
    {
        LW_SAFE_FREE_STRING(pAttribute->pszName);
    }

    LwFreeMemory(pAttribute);
}

void
LWIQuery::FreeAttributeList(
    PDSATTRIBUTE pAttributeList
    )
{
    PDSATTRIBUTE pAttribute = NULL;
    while (pAttributeList)
    {
        pAttribute = pAttributeList;
        pAttributeList = pAttributeList->pNext;
        FreeAttribute(pAttribute);
    }
}

void
LWIQuery::FreeRecord(PDSRECORD pRecord)
{
    if (pRecord->pAttributeListHead)
       FreeAttributeList(pRecord->pAttributeListHead);
   
    if (pRecord->pszName)
    {
        LW_SAFE_FREE_STRING(pRecord->pszName);
    }

    if (pRecord->pszType)
    {
        LW_SAFE_FREE_STRING(pRecord->pszType);
    }

    LwFreeMemory(pRecord);
}

void
LWIQuery::FreeRecordList(PDSRECORD pRecordList)
{
    PDSRECORD pTmpRecord = NULL;
    while (pRecordList)
    {
        pTmpRecord = pRecordList;
        pRecordList = pRecordList->pNext;
        FreeRecord(pTmpRecord);
    }
}

void
LWIQuery::FreeMessage(PDSMESSAGE pMessage)
{
    if (pMessage->pHeader)
    {
       FreeMessageHeader(pMessage->pHeader);
    }

    if (pMessage->pRecordList)
    {
       FreeRecordList(pMessage->pRecordList);
    }
}

void
LWIQuery::FreeMessageHeader(PDSMESSAGEHEADER pHeader)
{
    if (pHeader->pOffsets)
    {
       LwFreeMemory(pHeader->pOffsets);
    }

    LwFreeMemory(pHeader);
}

long
LWIQuery::CreateMemberList(DWORD dwMemberCount, PLSA_SECURITY_OBJECT* ppMembers, PLWIMEMBERLIST* ppMemberList)
{
    long macError = eDSNoErr;
    PLWIMEMBERLIST pList = NULL;
    PLWIMEMBERLIST pPrev = NULL;
    PLWIMEMBERLIST pNew = NULL;
    DWORD iMember = 0;

    if (ppMembers)
    {
        for (iMember = 0; iMember < dwMemberCount; iMember++)
        {
            if(ppMembers[iMember] && ppMembers[iMember]->type == LSA_OBJECT_TYPE_USER)
            {
                macError = LwAllocateMemory(sizeof(LWIMEMBERLIST), (PVOID*)&pNew);
                GOTO_CLEANUP_ON_MACERROR(macError);

                macError = LwAllocateString(ppMembers[iMember]->userInfo.pszUnixName, &pNew->pszName);
                GOTO_CLEANUP_ON_MACERROR(macError);

                pNew->uid = ppMembers[iMember]->userInfo.uid;

                if (pPrev)
                {
                    pPrev->pNext = pNew;
                }
                else
                {
                    pList = pNew;
                }

                pPrev = pNew;
                pNew = NULL;

                iMember++;
            }
        }
    }

    *ppMemberList = pList;
    pList = NULL;

cleanup:

    FreeMemberList(pList);

    return macError;
}

void
LWIQuery::FreeMemberList(PLWIMEMBERLIST pMemberList)
{
    while(pMemberList)
    {
        PLWIMEMBERLIST pCur = pMemberList;

        pMemberList = pMemberList->pNext;

        LW_SAFE_FREE_STRING(pCur->pszName);
        LW_SAFE_FREE_STRING(pCur->pszUPN);
        LwFreeMemory(pCur);
    }
}

long
LWIQuery::AddUserRecordHelper(
    IN PLSA_SECURITY_OBJECT pUserObject,
    IN OPTIONAL const char * pszNameAsQueried
    )
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;
    PAD_USER_ATTRIBUTES padUserInfo = NULL;
    PMCXVALUE pMCXValueList = NULL;
    PSTR pszUserName = NULL;
    PSTR pszNFSHomeDirectory = NULL;
    PSTR pszHomeDirectory = NULL;
    PSTR pszOriginalNFSHomeDirectory = NULL;
    PSTR pszOriginalHomeDirectory = NULL;
    DWORD Flags = _dwFlags;

    macError = GetADUserInfo(pUserObject->userInfo.uid, &padUserInfo);
    if (macError)
    {
        // LOG("No cached AD attributes found for user: %s",
        //     pUserObject->userInfo.pszUnixName ? pUserObject->userInfo.pszUnixName : "<null>");
        macError = eDSNoErr;
    }

    if (padUserInfo)
    {
        macError = GetUserHomeFolderSettings(padUserInfo->pszHomeDirectory,
                                             Flags,
                                             &pszNFSHomeDirectory,
                                             &pszHomeDirectory,
                                             &pszOriginalNFSHomeDirectory,
                                             &pszOriginalHomeDirectory);
        GOTO_CLEANUP_ON_MACERROR(macError);

        pszUserName = padUserInfo->pszDisplayName;
    }

    if (!pszUserName)
    {
        pszUserName = pUserObject->userInfo.pszUnixName;
    }

    if (Flags & LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK &&
        !pszOriginalNFSHomeDirectory &&
        !pszOriginalHomeDirectory)
    {
        /* User does not have a homeDirectory attribute set, so don't bother adding
           MCXSetting for Force Local Home Directory on Startup Disk */
        Flags = Flags & ~LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK;
    }

    macError = GetGPOGroupMCXSettingsForUser(pUserObject->userInfo.pszUnixName, pUserObject->userInfo.uid, &pMCXValueList, Flags);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CreateLWIUser(pUserObject->userInfo.pszUnixName, /* Record Name */
                             pszUserName, /* Display name */
                             pszNameAsQueried, /* Name as queried */
                             NULL, /* Password */
                             NULL, /* Class */
                             pUserObject->userInfo.pszGecos,
                             pszNFSHomeDirectory ? pszNFSHomeDirectory : pUserObject->userInfo.pszHomedir,
                             pszHomeDirectory,
                             pszOriginalNFSHomeDirectory,
                             pszOriginalHomeDirectory,
                             pUserObject->userInfo.pszShell,
                             pUserObject->userInfo.uid,
                             pUserObject->userInfo.gid,
                             pMCXValueList,
                             padUserInfo,
                             &pUser);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = AddUserRecord(pUser, pszUserName);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    FreeLWIUser(pUser);
    FreeMCXValueList(pMCXValueList);
    FreeADUserInfo(padUserInfo);
    LW_SAFE_FREE_STRING(pszNFSHomeDirectory);
    LW_SAFE_FREE_STRING(pszHomeDirectory);
    LW_SAFE_FREE_STRING(pszOriginalNFSHomeDirectory);
    LW_SAFE_FREE_STRING(pszOriginalHomeDirectory);

    return macError;
}

long
LWIQuery::AddUserRecord(
    IN PLWIUSER pUser,
    IN OPTIONAL const char* AltName
    )
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    long dirNodeRef = GetDirNodeRef();

    macError = BuildRecord(kDSStdRecordTypeUsers, pUser->pw_name, dirNodeRef, &pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = ProcessUserAttributes(pRecord, AltName, pUser);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CommitRecord(pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord = NULL;

cleanup:

    if (pRecord)
       FreeRecord(pRecord);

    return macError;
}

long
LWIQuery::AddGroupRecordHelper(
    IN PLSA_SECURITY_OBJECT pGroupObject,
    IN bool bExpandMembers,
    IN OPTIONAL const char* pszNameAsQueried
    )
{
    long macError = eDSNoErr;
    PLWIGROUP pGroup = NULL;
    PLWIMEMBERLIST pMembers = NULL;
    PLSA_SECURITY_OBJECT* ppGroupMembers = NULL;
    DWORD dwMemberCount = 0;

    if (bExpandMembers)
    {
        macError = ExpandGroupMembers(pGroupObject->pszObjectSid, &ppGroupMembers, &dwMemberCount);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = CreateMemberList(dwMemberCount, ppGroupMembers, &pMembers);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = CreateLWIGroup(pGroupObject->groupInfo.pszUnixName, /* Group Display Name */
                              pszNameAsQueried, /* Name as queried */
                              pGroupObject->groupInfo.pszPasswd,
                              pGroupObject->groupInfo.pszUnixName, /* Group Name */
                              NULL, /* Comment */
                              pMembers,
                              NULL, /* Generated UID - Computed automatically later */
                              pGroupObject->groupInfo.gid,
                              NULL, /* MCXValues */
                              &pGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = AddGroupRecord(pGroup, pGroupObject->groupInfo.pszUnixName);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (ppGroupMembers)
    {
        FreeObjectList(dwMemberCount, ppGroupMembers);
    }

    FreeLWIGroup(pGroup);
    FreeMemberList(pMembers);

    return macError;
}

long
LWIQuery::AddGroupRecord(
    IN PLWIGROUP pGroup,
    IN OPTIONAL const char* AltName
    )
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    long dirNodeRef = GetDirNodeRef();

    macError = BuildRecord(kDSStdRecordTypeGroups, pGroup->gr_name, dirNodeRef, &pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = ProcessGroupAttributes(pRecord, AltName, pGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CommitRecord(pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord = NULL;

cleanup:

    if (pRecord)
       FreeRecord(pRecord);

    return macError;
}

long
LWIQuery::AddComputerRecord(
    IN PLWICOMPUTER pComputer
    )
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    long dirNodeRef = GetDirNodeRef();

    macError = BuildRecord(kDSStdRecordTypeComputers, pComputer->name, dirNodeRef, &pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = ProcessComputerAttributes(pRecord, pComputer);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CommitRecord(pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord = NULL;

cleanup:

    if (pRecord)
       FreeRecord(pRecord);

    return macError;
}

long
LWIQuery::AddComputerListRecord(
    IN PLWICOMPUTERLIST pComputerList
    )
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    long dirNodeRef = GetDirNodeRef();

    macError = BuildRecord(kDSStdRecordTypeComputerLists, pComputerList->name, dirNodeRef, &pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = ProcessComputerListAttributes(pRecord, pComputerList);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CommitRecord(pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord = NULL;

cleanup:

    if (pRecord)
       FreeRecord(pRecord);

    return macError;
}

long
LWIQuery::AddComputerGroupRecord(
    IN PLWICOMPUTERGROUP pComputerGroup
    )
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    long dirNodeRef = GetDirNodeRef();

    macError = BuildRecord(kDSStdRecordTypeComputerGroups, pComputerGroup->name, dirNodeRef, &pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = ProcessComputerGroupAttributes(pRecord, pComputerGroup);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CommitRecord(pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord = NULL;

cleanup:

    if (pRecord)
       FreeRecord(pRecord);

    return macError;
}

long
LWIQuery::BuildRecord(
    const char * pszType,
    const char * pszName,
    long dirNodeRef,
    PDSRECORD* ppRecord
    )
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;

    if ( !pszType || !pszName )
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR( macError );
    }

    macError = LwAllocateMemory(sizeof(DSRECORD), (PVOID*)&pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LwAllocateString(pszName, &pRecord->pszName);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord->nameLen = strlen(pszName);

    macError = LwAllocateString(pszType, &pRecord->pszType);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord->typeLen = strlen(pszType);

    pRecord->dirNodeRef = dirNodeRef;
    pRecord->fDirty = FALSE;

    *ppRecord = pRecord;
    pRecord = NULL;

cleanup:

    if ( pRecord )
    {
        FreeRecord(pRecord);
    }

    return macError;
}

long
LWIQuery::CommitRecord(
    PDSRECORD pRecord
    )
{
    long macError = eDSNoErr;

    if (!_pRecordListHead)
    {
        _pRecordListHead = pRecord;
        _pRecordListTail = pRecord;
    }
    else
    {
        _pRecordListTail->pNext = pRecord;
        _pRecordListTail = pRecord;
    }

    return macError;
}

long
LWIQuery::AddAttributeEx(
    OUT PDSATTRIBUTE* ppAttribute,
    IN PDSRECORD pRecord,
    IN const char* pszAttributeName,
    IN OPTIONAL const char* pszValue
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = AddAttribute(pszAttributeName, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pszValue)
    {
        macError = SetAttributeValue(pAttribute, pszValue, strlen(pszValue));
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    *ppAttribute = pAttribute;

cleanup:

    // Don't clean up the attribute

    return macError;
}

long
LWIQuery::AddAttribute(
    const char* pszAttributeName,
    PDSRECORD pRecord,
    PDSATTRIBUTE* ppAttribute
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = LwAllocateMemory(sizeof(DSATTRIBUTE), (PVOID*)&pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LwAllocateString(pszAttributeName, &pAttribute->pszName);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pAttribute->nameLen = strlen(pszAttributeName);

    if (!pRecord->pAttributeListHead)
    {
        pRecord->pAttributeListHead = pAttribute;
        pRecord->pAttributeListTail = pAttribute;
    }
    else
    {
        pRecord->pAttributeListTail->pNext = pAttribute;
        pRecord->pAttributeListTail = pAttribute;
    }

    *ppAttribute = pAttribute;
    pAttribute = NULL;

cleanup:

    if (pAttribute)
    {
        FreeAttribute(pAttribute);
    }

    return macError;
}

long
LWIQuery::SetAttributeValue(
    PDSATTRIBUTE pAttribute,
    const char * pszValue
    )
{
    return SetAttributeValue(pAttribute, pszValue, ((pszValue && *pszValue) ? strlen(pszValue): 0));
}

long
LWIQuery::SetAttributeValue(
    PDSATTRIBUTE pAttribute,
    const char * pszValue,
    int          valLen
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTEVALUE pAttrValue = NULL;

    macError = LwAllocateMemory(sizeof(DSATTRIBUTEVALUE), (PVOID*)&pAttrValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pAttrValue->valLen = valLen;

    if (valLen)
    {
        macError = LwAllocateMemory(valLen, (PVOID*)&pAttrValue->pszValue);
        GOTO_CLEANUP_ON_MACERROR(macError);

        memcpy(pAttrValue->pszValue, pszValue, valLen);
    }
    else
    {
        pAttrValue->pszValue = NULL;
    }

    if (!pAttribute->pValueListHead)
    {
        pAttribute->pValueListHead = pAttrValue;
        pAttribute->pValueListTail = pAttrValue;
    }
    else
    {
        pAttribute->pValueListTail->pNext = pAttrValue;
        pAttribute->pValueListTail = pAttrValue;
    }

    pAttrValue = NULL;

cleanup:

    if (pAttrValue)
    {
        FreeAttributeValue(pAttrValue);
    }

    return macError;
}

long
LWIQuery::AddAttributeAndValue(
    const char* pszAttributeName,
    const char* pszValue,
    PDSRECORD pRecord,
    PDSATTRIBUTE* ppAttribute
    )
{
    int len = 0;

    if (pszValue)
    {
        len = strlen(pszValue);
    }

    return AddAttributeAndValue(pszAttributeName, pszValue, len, pRecord, ppAttribute);
}

long
LWIQuery::AddAttributeAndValue(
    const char* pszAttributeName,
    const char* pszValue,
    int         valLen,
    PDSRECORD   pRecord,
    PDSATTRIBUTE* ppAttribute
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    macError = AddAttribute(pszAttributeName, pRecord, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = SetAttributeValue(pAttribute, pszValue, valLen);
    GOTO_CLEANUP_ON_MACERROR(macError);

    *ppAttribute = pAttribute;

cleanup:

    // Don't clean up the attribute

    return macError;
}

void
LWIQuery::RemoveAttributeAndValues(
    const char* pszAttributeName,
    PDSRECORD pRecord
    )
{
    PDSATTRIBUTE pAttribute = NULL;
    PDSATTRIBUTE pAttributeList = NULL;
    PDSATTRIBUTE pPrev = NULL;

    if (pRecord->pAttributeListHead)
        pAttributeList = pRecord->pAttributeListHead;

    while (pAttributeList)
    {
        if (!strcmp(pAttributeList->pszName, pszAttributeName))
        {
            pAttribute = pAttributeList;

            if (pPrev)
            {
                pPrev->pNext = pAttributeList->pNext;
            }
            else
            {
                pRecord->pAttributeListHead = pAttributeList->pNext;
            }

            pAttributeList = pAttributeList->pNext;
            FreeAttribute(pAttribute);
        }
        else
        {
            pPrev = pAttributeList;
            pAttributeList = pAttributeList->pNext;
        }
    }

    if (pPrev)
    {
        pRecord->pAttributeListTail = pPrev;
    }
    else
    {
        pRecord->pAttributeListTail = NULL;
    }
}

//
// Format:
//
// Header            'StdA'                 4 bytes
// Number of records #Records               4 bytes
// Record Offsets    4 * #Records
// Footer            'EndT'                 4 bytes
// Record            recordSize             4 bytes
//                   Record type length     2 bytes
//                   Record type            length of record type string
//                   Record name length     2 bytes
//                   Record name            length of record name string
//                   #Attributes            4 bytes
//                   AttributeSize          4 bytes
//                   Attribute name length  2 bytes
//                   Attribute name         length of attribute name string
//                   #Attribute values      2 bytes
//                   Attribute value length 4 bytes
//                   Attribute value        length of attribute value
//                   Next Attribute...
// Next Record...
//
long
LWIQuery::WriteResponse(
    char* buffer,
    unsigned long maxBufferSize,
    unsigned long& bytesWritten,
    unsigned long& nRecordsWritten,
    unsigned long& TotalRecords,
    uint32_t headerType
    )
{
    long macError = eDSNoErr;
    int  nRecords = 0;
    int  nTotalRecords = 0;
    int  iRecord = 0;
    PDSRECORD pRecord = NULL;
    unsigned long offset = 0;
    
    /* Initialize the return value */
    nRecordsWritten = 0;
    TotalRecords = 0;

    // What is the start of records we are to return?
    if (_pCurrentRecord == NULL)
    {
        _pCurrentRecord = _pRecordListHead;
    }

    macError = DetermineRecordsToFitInBuffer(maxBufferSize, nRecords, nTotalRecords);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (nRecords > 0)
    {
        macError = WriteHeader(buffer, nRecords, offset, headerType);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (pRecord = _pCurrentRecord; pRecord && (iRecord < nRecords); pRecord = pRecord->pNext, iRecord++)
        {
            macError = WriteRecord(buffer, iRecord, pRecord, offset);
            GOTO_CLEANUP_ON_MACERROR(macError);
            
            nRecordsWritten++;
        }

        // If the list had more items, save the position of the record we need to continue on.
        if (pRecord)
        {
            _pCurrentRecord = pRecord;
        }
    }

    bytesWritten = offset;
    TotalRecords = nTotalRecords;

    LOG("WriteResponse success, bytesWritten = %d, recordsWritten = %d, TotalRecords = %d", bytesWritten, nRecordsWritten, TotalRecords);

cleanup:

    return macError;
}

long
LWIQuery::WriteGDNIResponse(
    char* buffer,
    unsigned long maxBufferSize,
    unsigned long& bytesWritten,
    unsigned long& nRecordsWritten,
    unsigned long& TotalRecords
    )
{
    return WriteResponse(buffer, maxBufferSize, bytesWritten, nRecordsWritten, TotalRecords, 'Gdni');
}

long
LWIQuery::ReadResponseAttributeValue(
    char* buffer,
    unsigned long  bufferSize,
    unsigned long offset,
    PDSATTRIBUTEVALUE* ppAttributeValue
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTEVALUE pAttrValue = NULL;
    unsigned long woffset = offset;

    macError = LwAllocateMemory(sizeof(DSATTRIBUTEVALUE), (PVOID*)&pAttrValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* attribute value length: 4 bytes */
    CHECK_INVALID_OFFSET(woffset+4, bufferSize);

    memcpy(&pAttrValue->valLen, buffer+woffset, 4);
    woffset += 4;

    /* attribute value:                */
    if (pAttrValue->valLen)
    {
       CHECK_INVALID_OFFSET(woffset+pAttrValue->valLen, bufferSize);

       // Should we allocate an extra byte?
       macError = LwAllocateMemory(pAttrValue->valLen+1, (PVOID*)&pAttrValue->pszValue);
       GOTO_CLEANUP_ON_MACERROR(macError);

       memcpy(pAttrValue->pszValue, buffer+woffset, pAttrValue->valLen);
       woffset += pAttrValue->valLen;
    }

    *ppAttributeValue = pAttrValue;
    pAttrValue = NULL;

cleanup:

    if (pAttrValue)
       FreeAttributeValue(pAttrValue);

    return macError;
}

long
LWIQuery::ReadResponseAttribute(
    char* buffer,
    unsigned long  bufferSize,
    unsigned long  offset,
    PDSATTRIBUTE* ppAttribute
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;
    unsigned long woffset = offset;
    uint16_t nValues = 0;
    uint16_t iValue = 0;
    PDSATTRIBUTEVALUE pAttrValueListHead = NULL;
    PDSATTRIBUTEVALUE pAttrValueListTail = NULL;
    PDSATTRIBUTEVALUE pAttrValue = NULL;

    macError = LwAllocateMemory(sizeof(DSATTRIBUTE), (PVOID*)&pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* attribute name length: 2 bytes */
    CHECK_INVALID_OFFSET(woffset + 2, bufferSize);

    memcpy(&pAttribute->nameLen, buffer+woffset, 2);
    woffset += 2;

    /* attribute name string:         */
    if (pAttribute->nameLen)
    {
        CHECK_INVALID_OFFSET(woffset + pAttribute->nameLen, bufferSize);

        macError = LwAllocateMemory(pAttribute->nameLen+1, (PVOID*)&pAttribute->pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);

        memcpy(pAttribute->pszName, buffer+woffset, pAttribute->nameLen);

        woffset += pAttribute->nameLen;
    }

    /* number of values:      2 bytes */
    CHECK_INVALID_OFFSET(woffset+2, bufferSize);

    memcpy(&nValues, buffer+woffset, 2);
    woffset += 2;

    /* values:                        */
    for (iValue = 0; iValue < nValues; iValue++)
    {
        macError = ReadResponseAttributeValue(buffer, bufferSize, woffset, &pAttrValue);
        GOTO_CLEANUP_ON_MACERROR(macError);

        woffset += 4 + pAttrValue->valLen;

        if (!pAttrValueListHead)
        {
            pAttrValueListHead = pAttrValue;
            pAttrValueListTail = pAttrValue;
        }
        else
        {
            pAttrValueListTail->pNext = pAttrValue;
            pAttrValueListTail = pAttrValue;
        }
        pAttrValue = NULL;
    }

    pAttribute->pValueListHead = pAttrValueListHead;
    pAttrValueListHead = NULL;
    pAttribute->pValueListTail = pAttrValueListTail;
    pAttrValueListTail = NULL;

    *ppAttribute = pAttribute;
    pAttribute = NULL;

cleanup:

    if (pAttribute)
       FreeAttribute(pAttribute);

    if (pAttrValue)
       FreeAttributeValue(pAttrValue);

    if (pAttrValueListHead)
       FreeAttributeValueList(pAttrValueListHead);

    return macError;
}

long
LWIQuery::ReadResponseRecord(
    char* buffer,
    unsigned long  bufferSize,
    unsigned long  offset,
    PDSRECORD* ppRecord
    )
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    PDSATTRIBUTE pAttributeListHead = NULL;
    PDSATTRIBUTE pAttributeListTail = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    unsigned long woffset = offset;
    uint16_t nAttributes = 0;
    uint16_t iAttribute = 0;
    uint32_t attrBlockSize = 0;

    macError = LwAllocateMemory(sizeof(DSRECORD), (PVOID*)&pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* record type length:   2 bytes */
    CHECK_INVALID_OFFSET(woffset+2, bufferSize);

    memcpy(&pRecord->typeLen, buffer+woffset, 2);
    woffset += 2;

    /* record type string:           */
    if (pRecord->typeLen)
    {
        CHECK_INVALID_OFFSET(woffset+pRecord->typeLen, bufferSize);

        macError = LwAllocateMemory(pRecord->typeLen+1, (PVOID*)&pRecord->pszType);
        GOTO_CLEANUP_ON_MACERROR(macError);

        memcpy(pRecord->pszType, buffer+woffset, pRecord->typeLen);
        woffset += pRecord->typeLen;
    }

    /* record name length:   2 bytes */
    CHECK_INVALID_OFFSET(woffset+2, bufferSize);

    memcpy(&pRecord->nameLen, buffer+woffset, 2);
    woffset += 2;

    /* record name string:           */
    if (pRecord->nameLen)
    {
        CHECK_INVALID_OFFSET(woffset+pRecord->nameLen, bufferSize);

        macError = LwAllocateMemory(pRecord->nameLen+1, (PVOID*)&pRecord->pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);

        memcpy(pRecord->pszName, buffer+woffset, pRecord->nameLen);
        woffset += pRecord->nameLen;
    }

     /* number of attributes: 2 bytes */
     CHECK_INVALID_OFFSET(woffset+2, bufferSize);

     memcpy(&nAttributes, buffer+woffset, 2);
     woffset += 2;

     /* Attribute list                */
     for (iAttribute = 0; iAttribute < nAttributes; iAttribute++)
     {
         CHECK_INVALID_OFFSET(woffset+4, bufferSize);

         /* length of attribute block */
         memcpy(&attrBlockSize, buffer+woffset, 4);
         woffset += 4;

         macError = ReadResponseAttribute(buffer,
                                          bufferSize,
                                          woffset,
                                          &pAttribute);
         GOTO_CLEANUP_ON_MACERROR(macError);

         woffset += attrBlockSize;

         if (!pAttributeListHead)
         {
            pAttributeListHead = pAttribute;
            pAttributeListTail = pAttribute;
         }
         else
         {
            pAttributeListTail->pNext = pAttribute;
            pAttributeListTail = pAttribute;
         }

         pAttribute = NULL;
    }

    pRecord->pAttributeListHead = pAttributeListHead;
    pAttributeListHead = NULL;
    pRecord->pAttributeListTail = pAttributeListTail;
    pAttributeListTail = NULL;

    pRecord->fDirty = FALSE;

    *ppRecord = pRecord;
    pRecord = NULL;

cleanup:

    if (pRecord)
       FreeRecord(pRecord);

    if (pAttributeListHead)
       FreeAttributeList(pAttributeListHead);

    if (pAttribute)
       FreeAttribute(pAttribute);

    return macError;
}

long
LWIQuery::ReadResponseRecords(
    char* buffer,
    unsigned long  bufferSize,
    PDSMESSAGEHEADER pHeader,
    PDSRECORD* ppRecordList
    )
{
    long macError = eDSNoErr;
    uint32_t iRecord = 0;
    PDSRECORD pRecordListHead = NULL;
    PDSRECORD pRecordListTail = NULL;
    PDSRECORD pRecord = NULL;
    uint32_t recordSize = 0;
    unsigned long offset = 0;

    for (iRecord = 0; iRecord < pHeader->nRecords; iRecord++)
    {
        offset = pHeader->pOffsets[iRecord];

        CHECK_INVALID_OFFSET(offset+4, bufferSize);
        memcpy(&recordSize, buffer+4, 4);
        offset += 4;

        macError = ReadResponseRecord(buffer, bufferSize, offset, &pRecord);
        GOTO_CLEANUP_ON_MACERROR(macError);

        if (!pRecordListHead)
        {
           pRecordListHead = pRecord;
           pRecordListTail = pRecord;
        }
        else
        {
            pRecordListTail->pNext = pRecord;
            pRecordListTail = pRecord;
        }
        pRecord = NULL;
    }

    *ppRecordList = pRecordListHead;
    pRecordListHead = NULL;

cleanup:

    if (pRecordListHead)
       FreeRecordList(pRecordListHead);

    if (pRecord)
       FreeRecord(pRecord);

    return macError;
}

long
LWIQuery::ReadResponseHeader(
    char* pszBuffer,
    unsigned long  bufferSize,
    PDSMESSAGEHEADER * ppHeader
    )
{
    long macError = eDSNoErr;
    PDSMESSAGEHEADER pHeader = NULL;
    uint32_t iRecord = 0;
    unsigned long offset = 0;

    macError = LwAllocateMemory(sizeof(DSMESSAGEHEADER), (PVOID*)&pHeader);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* start tag */
    CHECK_INVALID_OFFSET(offset+4, bufferSize);

    memcpy(&pHeader->startTag, pszBuffer+offset, 4);
    offset += 4;

    /* number of records */
    CHECK_INVALID_OFFSET(offset+4, bufferSize);

    memcpy(&pHeader->nRecords, pszBuffer+offset, 4);
    offset += 4;

    /* record offsets    */
    if (pHeader->nRecords)
    {
        CHECK_INVALID_OFFSET(offset + (4*pHeader->nRecords), bufferSize);

        macError = LwAllocateMemory(4*pHeader->nRecords, (PVOID*)&pHeader->pOffsets);
        GOTO_CLEANUP_ON_MACERROR(macError);

        // Do one at a time in case we want to adjust for endian-ness
        for (iRecord = 0; iRecord < pHeader->nRecords; iRecord++)
        {
            memcpy(&pHeader->pOffsets[iRecord], pszBuffer+offset, 4);
            offset += 4;
        }
    }

    /* end tag */
    CHECK_INVALID_OFFSET(offset+4, bufferSize);

    memcpy(&pHeader->endTag, pszBuffer+offset, 4);
    offset += 4;

    *ppHeader = pHeader;
    pHeader = NULL;

cleanup:

    if (pHeader)
       FreeMessageHeader(pHeader);

    return macError;
}

long
LWIQuery::ReadResponse(
    char* buffer,
    unsigned long  bufferSize,
    PDSMESSAGE* ppMessage
    )
{
    long macError = eDSNoErr;
    PDSMESSAGE pMessage = NULL;

    macError = LwAllocateMemory(sizeof(DSMESSAGE), (PVOID*)&pMessage);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = ReadResponseHeader(buffer, bufferSize, &pMessage->pHeader);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = ReadResponseRecords(buffer,
                                   bufferSize,
                                   pMessage->pHeader,
                                   &pMessage->pRecordList
                                   );
    GOTO_CLEANUP_ON_MACERROR(macError);

    *ppMessage = pMessage;
    pMessage = NULL;

cleanup:

    if (pMessage)
        FreeMessage(pMessage);

    return macError;
}

long
LWIQuery::FindAttributeByType(
    PDSRECORD pRecord,
    const char* pszAttrType,
    PDSATTRIBUTE* ppAttribute
    )
{
    long macError = eDSAttributeNotFound;
    PDSATTRIBUTE pAttribute = NULL;

    if (pRecord == NULL || pszAttrType == NULL)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    for (pAttribute = pRecord->pAttributeListHead; pAttribute != NULL; pAttribute = pAttribute->pNext)
    {
        if (!strcmp(pAttribute->pszName, pszAttrType))
        {
           macError = eDSNoErr;
           break;
        }
    }

    *ppAttribute = pAttribute;

cleanup:

    return macError;
}

long
LWIQuery::FindAttributeValueByID(
    PDSATTRIBUTE pAttribute,
    unsigned long attrValueID,
    PDSATTRIBUTEVALUE* ppAttributeValue
    )
{
    long macError = eDSAttributeValueNotFound;
    PDSATTRIBUTEVALUE pAttrValue = NULL;

    if (pAttribute == NULL)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    for (pAttrValue = pAttribute->pValueListHead; pAttrValue; pAttrValue = pAttrValue->pNext)
    {
        if (LWICRC::GetCRC(pAttrValue->pszValue, pAttrValue->valLen) == attrValueID)
        {
           macError = eDSNoErr;
           break;
        }
    }

cleanup:

    return macError;
}

long
LWIQuery::FindAttributeValueByIndex(
    PDSATTRIBUTE pAttribute,
    unsigned long attrValueIndex,
    PDSATTRIBUTEVALUE* ppAttributeValue
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTEVALUE currentElement = NULL;
    unsigned long currentIndex = 0;

    currentElement = pAttribute->pValueListHead;
    currentIndex = 1;
    while (currentElement && (currentIndex < attrValueIndex))
    {
        currentElement = currentElement->pNext;
        currentIndex++;
    }

    *ppAttributeValue = currentElement;

    macError = currentElement ? eDSNoErr : eDSAttributeValueNotFound;

    return macError;
}

long
LWIQuery::CreateAttributeEntry(
    OUT tAttributeEntryPtr* ppAttributeEntry,
    IN PDSATTRIBUTE pAttribute
    )
{
    long macError = eDSNoErr;
    tAttributeEntryPtr pAttributeEntry;

    macError = LwAllocateMemory(sizeof(*pAttributeEntry) + pAttribute->nameLen + MAGIC_PADDING, (PVOID*)&pAttributeEntry);
    GOTO_CLEANUP_ON_MACERROR(macError);

    for (PDSATTRIBUTEVALUE pAttrValue = pAttribute->pValueListHead; pAttrValue; pAttrValue = pAttrValue->pNext)
    {
        pAttributeEntry->fAttributeValueCount++;
        pAttributeEntry->fAttributeDataSize += pAttrValue->valLen;
    }

    // A "big enough" value.  We could porentially return just the max computed from
    // iteration?  The "documentation" for this field says that the field is the
    // "maximum size of a value of this attribute type".  Is that the max for the type
    // itself or for this instance of the type?  If the latter, a max would be fine.
    pAttributeEntry->fAttributeValueMaxSize = 512;

    pAttributeEntry->fAttributeSignature.fBufferSize = pAttribute->nameLen + MAGIC_PADDING;
    pAttributeEntry->fAttributeSignature.fBufferLength = pAttribute->nameLen;
    memcpy(pAttributeEntry->fAttributeSignature.fBufferData, pAttribute->pszName, pAttribute->nameLen);

cleanup:

    if (macError)
    {
        LwFreeMemory(pAttributeEntry);
        pAttributeEntry = NULL;
    }

    *ppAttributeEntry = pAttributeEntry;

    return macError;
}

long
LWIQuery::CreateAttributeValueEntry(
    OUT tAttributeValueEntryPtr* ppAttributeValueEntry,
    IN PDSATTRIBUTEVALUE pAttributeValue
    )
{
    long macError = eDSNoErr;
    tAttributeValueEntryPtr pAttributeValueEntry = NULL;

    macError = LwAllocateMemory(sizeof(*pAttributeValueEntry) + pAttributeValue->valLen + MAGIC_PADDING, (PVOID*)&pAttributeValueEntry);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pAttributeValueEntry->fAttributeValueData.fBufferSize = pAttributeValue->valLen + MAGIC_PADDING;
    pAttributeValueEntry->fAttributeValueData.fBufferLength = pAttributeValue->valLen;
    memcpy(pAttributeValueEntry->fAttributeValueData.fBufferData, pAttributeValue->pszValue, pAttributeValue->valLen);

    pAttributeValueEntry->fAttributeValueID = LWICRC::GetCRC(pAttributeValue->pszValue, pAttributeValue->valLen);

cleanup:

    if (macError)
    {
        LwFreeMemory(pAttributeValueEntry);
        pAttributeValueEntry = NULL;
    }

    *ppAttributeValueEntry = pAttributeValueEntry;

    return macError;
}

long
LWIQuery::GetNumberOfAttributes(PDSRECORD pRecord, uint16_t& nAttributes)
{
    unsigned short nAttr = 0;
    PDSATTRIBUTE pAttribute = NULL;

    for (pAttribute = pRecord->pAttributeListHead; pAttribute != NULL; pAttribute = pAttribute->pNext, nAttr++);

    nAttributes = nAttr;

    return eDSNoErr;
}

long
LWIQuery::GetNumberOfAttributeValues(PDSATTRIBUTE pAttribute, uint16_t& nValues)
{
    unsigned short nVals = 0;
    PDSATTRIBUTEVALUE pAttrVal = NULL;

    for (pAttrVal = pAttribute->pValueListHead; pAttrVal != NULL; pAttrVal = pAttrVal->pNext, nVals++);

    nValues = nVals;

    return eDSNoErr;
}

long
LWIQuery::DetermineRecordsToFitInBuffer(unsigned long maxBufferSize, int& nRecords, int& TotalRecords)
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    unsigned int recordSize = 0;
    unsigned long currentSize = 0;
    int total = 0;
    int result = 0;

    LOG_ENTER("maxBufferSize = %ld", maxBufferSize);

    if (_pCurrentRecord)
    {
        for (pRecord = _pCurrentRecord;
             pRecord; 
             pRecord = pRecord->pNext)
        {
            total++;
        }
        LOG("Counted %d records", total);

        for (pRecord = _pCurrentRecord;
             pRecord; 
             pRecord = pRecord->pNext)
        {
            macError = GetRecordSize(pRecord, recordSize);
            GOTO_CLEANUP_ON_MACERROR(macError);

            if((currentSize + recordSize + 4 + 4 + GetHeaderSize(result+1)) > maxBufferSize)
            {
                // Stopping here because this extra record will exceed buffer space provided
                break;
            }

            // Add the size of this record, it will fit.
            currentSize += 4; // This is in the offset in the header
            currentSize += 4; // This is to store the record size
            // before the contents of the record
            currentSize += recordSize;

            result++;
        }
        LOG("Will write %d record(s)", result);

        if (!pRecord)
        {
            LOG("All %d response records will fit into the buffer provided", total);
            currentSize += GetHeaderSize(result);
        }
        else
        {
            if (result == 0 && total > 0)
            {
                LOG("No record will fit into caller's buffer, size needed is %d", currentSize);
                nRecords = 0;
                TotalRecords = total;
                macError = eDSBufferTooSmall;
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
            else
            { 
                LOG("Grow buffer or write the results?");
                if (maxBufferSize < 32*1024)
                {
                    if (!_bRespondedWithTooSmallError)
                    {
                        LOG("Buffer too small to fit all %d records, only %d will fit (returning eDSBufferTooSmall to try a request for a larger buffer)", total, result);
                        currentSize += maxBufferSize + GetHeaderSize(result);
                        _bRespondedWithTooSmallError = true;
                        _lastResponseBufferSize = maxBufferSize;
                    }
                    else
                    {
                        if (_lastResponseBufferSize < maxBufferSize)
                        {
                            LOG("Buffer is still too small to fit all %d records, only %d will fit (returning eDSBufferTooSmall to encourage use of an larger one for better performance)", total, result);
                            currentSize += maxBufferSize + GetHeaderSize(result);
                            _bRespondedWithTooSmallError = true;
                            _lastResponseBufferSize = maxBufferSize;
                        }
                        else
                        {
                            LOG("Could not get a larger buffer from caller. Not all %d response records will fit into buffer provided, only %d will fit in the buffer", total, result);
                            currentSize += GetHeaderSize(result);
                        }
                    }
                }
                else
                {
                    LOG("Not all %d response records will fit into buffer provided, only %d will fit in the buffer", total, result);
                    currentSize += GetHeaderSize(result);
                }
            }
        }
    }
    else
    {
        LOG("No current record");
    }

    if (pRecord && !_bAllowIOContinue)
    {
        LOG("No record will fit into caller's buffer, size needed is %d", currentSize);
        nRecords = 0;
        TotalRecords = total;
        macError = eDSBufferTooSmall;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    LOG("%d record(s) will be written into caller's buffer, size needed is %d", result, currentSize);
    nRecords = result;
    TotalRecords = total;

cleanup:
    LOG_LEAVE("--> %d", macError);
    return macError;
}

//
// Format:
//
// Header            'StdA'                 4 bytes
// Number of records #Records               4 bytes
// Record Offsets    4 * #Records
// Footer            'EndT'                 4 bytes
unsigned int
LWIQuery::GetHeaderSize(int nRecords)
{
    return 4 + 4 + 4 * nRecords + 4;
}

//                   AttributeSize          4 bytes
//                   Attribute name length  2 bytes
//                   Attribute name         length of attribute name string
//                   #Attribute values      2 bytes
//                   Attribute value length 4 bytes
//                   Attribute value        length of attribute value
long
LWIQuery::GetAttributeSize(PDSATTRIBUTE pAttribute, uint32_t& attrSize)
{
    unsigned int result = 0;

    result += 2; // attribute name length
    result += pAttribute->nameLen;
    result += 2; // number of attribute values
    PDSATTRIBUTEVALUE pAttrVal = NULL;
    for (pAttrVal = pAttribute->pValueListHead; pAttrVal != NULL; pAttrVal = pAttrVal->pNext)
    {
        result += 4; // attribute value length
        result += pAttrVal->valLen;
    }

    attrSize = result;

    return eDSNoErr;
}

// Record            recordSize             4 bytes
//                   Record type length     2 bytes
//                   Record type            length of record type string
//                   Record name length     2 bytes
//                   Record name            length of record name string
//                   #Attributes            4 bytes
long
LWIQuery::GetRecordSize(PDSRECORD pRecord, uint32_t& recordSize)
{
    long macError = eDSNoErr;
    unsigned int result = 0;
    PDSATTRIBUTE pAttribute = NULL;
    unsigned int attrSize = 0;

    result += 2; // record type length
    result += pRecord->typeLen;
    result += 2; // record name length
    result += pRecord->nameLen;
    result += 2; // number of attributes
    for (pAttribute = pRecord->pAttributeListHead; pAttribute != NULL; pAttribute = pAttribute->pNext)
    {
        attrSize = 0;
        macError = GetAttributeSize(pAttribute, attrSize);
        GOTO_CLEANUP_ON_MACERROR(macError);
        result += 4; // length in bytes of attribute block
        result += attrSize;
    }

    recordSize = result;

cleanup:

    return macError;
}

//
// Format:
//
// Header            'StdA'                 4 bytes
// Number of records #Records               4 bytes
// Record Offsets    4 * #Records
// Footer            'EndT'                 4 bytes
//
long
LWIQuery::WriteHeader(char* buffer, int nRecords, unsigned long& offset, uint32_t headerType)
{
    uint32_t val = headerType;

    memcpy(buffer+offset, &val, 4);
    offset += 4;
    memcpy(buffer+offset, &nRecords, 4);
    offset += 4;
    offset += 4 * nRecords;
    val = 'EndT';
    memcpy(buffer+offset, &val, 4);
    offset += 4;

    return eDSNoErr;
}

long
LWIQuery::WriteRecord(char* buffer, int iRecord, PDSRECORD pRecord, unsigned long& offset)
{
    long macError = eDSNoErr;
    unsigned int recordSize = 0;
    unsigned short nAttributes = 0;
    PDSATTRIBUTE pAttribute = NULL;

    // Write the current record's offset
    memcpy(buffer + 4 /* Header size */ + 4 /* #records */ + (4 * iRecord), &offset, 4);

    macError = GetRecordSize(pRecord, recordSize);
    GOTO_CLEANUP_ON_MACERROR(macError);

    memcpy(buffer+offset, &recordSize, 4);
    offset += 4;

    memcpy(buffer+offset, &pRecord->typeLen, 2);
    offset += 2;

    memcpy(buffer+offset, pRecord->pszType, pRecord->typeLen);
    offset += pRecord->typeLen;

    memcpy(buffer+offset, &pRecord->nameLen, 2);
    offset += 2;

    memcpy(buffer+offset, pRecord->pszName, pRecord->nameLen);
    offset += pRecord->nameLen;

    macError = GetNumberOfAttributes(pRecord, nAttributes);
    GOTO_CLEANUP_ON_MACERROR(macError);

    memcpy(buffer+offset, &nAttributes, 2);
    offset += 2;

    for (pAttribute = pRecord->pAttributeListHead; pAttribute != NULL; pAttribute = pAttribute->pNext)
    {
        macError = WriteAttribute(buffer, pAttribute, offset);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

//                   AttributeSize          4 bytes
//                   Attribute name length  2 bytes
//                   Attribute name         length of attribute name string
//                   #Attribute values      2 bytes
//                   Attribute value length 4 bytes
//                   Attribute value        length of attribute value
//                   Next Attribute...
long
LWIQuery::WriteAttribute(char* buffer, PDSATTRIBUTE pAttribute, unsigned long& offset)
{
    long macError = eDSNoErr;
    unsigned int attrSize = 0;
    unsigned short nValues = 0;
    PDSATTRIBUTEVALUE pAttrVal = NULL;

    macError = GetAttributeSize(pAttribute, attrSize);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetNumberOfAttributeValues(pAttribute, nValues);
    GOTO_CLEANUP_ON_MACERROR(macError);

    memcpy(buffer+offset, &attrSize, 4);
    offset += 4;

    memcpy(buffer+offset, &pAttribute->nameLen, 2);
    offset += 2;

    memcpy(buffer+offset, pAttribute->pszName, pAttribute->nameLen);
    offset += pAttribute->nameLen;

    memcpy(buffer+offset, &nValues, 2);
    offset += 2;

    for (pAttrVal = pAttribute->pValueListHead; pAttrVal != NULL; pAttrVal = pAttrVal->pNext)
    {
        macError = WriteAttributeValue(buffer, pAttrVal, offset);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    return macError;
}

long
LWIQuery::WriteAttributeValue(
    char* buffer,
    PDSATTRIBUTEVALUE pAttributeValue,
    unsigned long& offset
    )
{
    memcpy(buffer+offset, &pAttributeValue->valLen, 4);
    offset += 4;
    memcpy(buffer+offset, pAttributeValue->pszValue, pAttributeValue->valLen);
    offset += pAttributeValue->valLen;

    return eDSNoErr;
}

//
// IO Continuation context list helper functions
//

long
InitializeContextList(
    )
{
    long macError = eDSNoErr;

    Global_ContextList = NULL;
    Global_LastHandleId = 0;

    if (pthread_mutex_init(&Global_ContextListMutexLock, NULL) < 0)
    {
        int libcError = errno;
        LOG_ERROR("Failied to init context cache lock: %s (%d)", strerror(libcError), libcError);
        macError = ePlugInInitError;
        GOTO_CLEANUP();
    }
    Global_ContextListMutexLockInitialized = TRUE;

cleanup:

    return macError;
}

void
UninitializeContextList(
    )
{
    if (Global_ContextListMutexLockInitialized)
    {
        pthread_mutex_lock(&Global_ContextListMutexLock);

        while (Global_ContextList)
        {
            PQUERYCONTEXT pTemp = Global_ContextList;

            Global_ContextList = Global_ContextList->pNext;

            if (pTemp->pQuery)
            {
                pTemp->pQuery->Release();
                pTemp->pQuery = NULL;
            }

            LwFreeMemory(pTemp);
        }

        Global_ContextList = NULL;
        Global_LastHandleId = 0;

        pthread_mutex_unlock(&Global_ContextListMutexLock);

        pthread_mutex_destroy(&Global_ContextListMutexLock);
    }
    Global_ContextListMutexLockInitialized = FALSE;
}

long
AddQueryToContextList(
    LWIQuery*     pQuery,
    tContextData* pHandleId
    )
{
    long macError = eDSNoErr;
    PQUERYCONTEXT pContext = NULL;
    tContextData HandleId = 0;

    if (!pQuery)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    // Create new query context
    macError = LwAllocateMemory(sizeof(QUERYCONTEXT), (PVOID*) &pContext);
    GOTO_CLEANUP_ON_MACERROR(macError);

    // Store query object into context
    pContext->pQuery = pQuery;

    // Insert new context into global list and assign handle identifier
    pthread_mutex_lock(&Global_ContextListMutexLock);

    if ((UInt32)Global_LastHandleId >= MAX_DS_CONTINUE_HANDLE)
    {
        // Time to loop around to new IDs
        Global_LastHandleId = 0;
    }

    HandleId = Global_LastHandleId = (tContextData)((UInt32)Global_LastHandleId + 1);
    pContext->HandleId = HandleId;
    
    pContext->pNext = Global_ContextList;
    Global_ContextList = pContext;
    pContext = NULL;

    pthread_mutex_unlock(&Global_ContextListMutexLock);

    *pHandleId = HandleId;

cleanup:

    if (pContext)
    {
        LwFreeMemory(pContext);
    }

    return macError;
}

long
GetQueryFromContextList(
    tContextData HandleId,
    LWIQuery**   ppQuery
    )
{
    long macError = eDSNoErr;
    PQUERYCONTEXT pCurrent = Global_ContextList;
    PQUERYCONTEXT pPrev = NULL;
    PQUERYCONTEXT pContext = NULL;

    pthread_mutex_lock(&Global_ContextListMutexLock);

    while (pCurrent)
    {
        if (pCurrent->HandleId == HandleId)
        {
            pContext = pCurrent;

            // Unlink the node from the list
            if (pPrev)
            {
                pPrev->pNext = pCurrent->pNext;
            }
            else
            {
                Global_ContextList = pCurrent->pNext;
            }

            pContext->pNext = NULL;

            break;
        }

        pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (!pContext)
    {
        macError = eDSRecordNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pthread_mutex_unlock(&Global_ContextListMutexLock);

    // Give the LWIQuery object to caller
    *ppQuery = pContext->pQuery;
    pContext->pQuery = NULL;

cleanup:

    // Free the old  context list item.
    if (pContext)
    {
        LwFreeMemory(pContext);
    }

    return macError;
}

void
LWIQuery::SetCacheLifeTime(
    DWORD dwCacheLifeTime
    )
{
    DEFAULT_ATTRIBUTE_TTL_SECONDS = dwCacheLifeTime;
}

