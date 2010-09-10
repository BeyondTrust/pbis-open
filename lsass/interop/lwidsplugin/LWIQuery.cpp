/*
 *  LWIQuery.cpp
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/24/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "includes.h"
#include "LWIQuery.h"
#include "LWICRC.h"
#include <sys/stat.h>

// ISSUE-2007/06/07 -- The '16' is a magic padding value.  It is not clear why it is used.
// We used it because the Apple plug-ins do it too.  Further investigation would be good.
#define MAGIC_PADDING 16

//#define LWI_UUID_UID "315F6FA0-4CCB-42AC-8CA8-A1126E0FA7AE"
#define LWI_UUID_UID_PREFIX "315F6FA0-4CCB-42AC-8CA8-A112"
//#define LWI_UUID_GID "9B5F5F9B-660D-4F41-A791-795FF6B5352A"
#define LWI_UUID_GID_PREFIX "9B5F5F9B-660D-4F41-A791-795F"
#define LWI_GUID_LENGTH 36

#define CHECK_INVALID_OFFSET(offset, bufsize)    \
        if (offset > bufsize) {                  \
           macError = eDSInvalidBuffFormat;      \
           GOTO_CLEANUP_ON_MACERROR(macError);   \
        }

uint32_t LWIQuery::DEFAULT_ATTRIBUTE_TTL_SECONDS = 60; // Cache values for a minute

LWIQuery::LWIQuery(bool bGetValues)
    : _bGetValues(bGetValues),
      _pRecordListHead(NULL),
      _pRecordListTail(NULL),
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
LWIQuery::Create(
    bool bGetValues,
    OUT LWIQuery** ppQuery)
{
    long macError = eDSNoErr;

    LWIQuery* pQuery = new LWIQuery(bGetValues);
    if (!pQuery)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

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

bool
LWIQuery::ShouldQueryUserInformation()
{
    return (_recTypeSet && LWI_BITVECTOR_ISSET(_recTypeSet, LWIRecTypeLookup::idx_kDSStdRecordTypeUsers));
}

bool
LWIQuery::ShouldQueryGroupInformation()
{
    return (_recTypeSet && LWI_BITVECTOR_ISSET(_recTypeSet, LWIRecTypeLookup::idx_kDSStdRecordTypeGroups));
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

    return LWIAllocateString(szBuf, ppszUID);
}

long
LWIQuery::BuildGeneratedGID(gid_t gid, char** ppszGID)
{
    char szBuf[LWI_GUID_LENGTH+1];

    sprintf(szBuf, LWI_UUID_GID_PREFIX "%.8X", gid);

    return LWIAllocateString(szBuf, ppszGID);
}

long
LWIQuery::GetAuthString(
    IN const PLWIUSER pUser,
    OUT char** AuthString
    )
{
    long macError;
    char* guidString = NULL;
    char* upn = NULL;
    char* generatedUpn = NULL;
    char* userSamAccount = NULL;
    char* userDomain = NULL;
    char* authString = NULL;
    char* temp = NULL;

    macError = BuildGeneratedUID(pUser->pw_uid, &guidString);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = GetUserPrincipalNames(pUser->pw_name, &upn, &userSamAccount, &userDomain);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (userDomain != NULL)
    {
        // Convert the userDomain to a REALM
        temp = userDomain;
        while (*temp != '\0'){
            *temp = toupper(*temp);
            temp++;
        }
    }

    if (userSamAccount != NULL &&
        userDomain != NULL)
    {
        asprintf(&generatedUpn, "%s@%s", userSamAccount, userDomain);
    }
    else
    {
        if (upn != NULL)
        {
            asprintf(&generatedUpn, "%s", upn);
        }
        else
        {
            asprintf(&generatedUpn, "%s@%s", pUser->pw_name, "domain.not.online");
        }
    }

    if (!generatedUpn)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

    asprintf(&authString, "1.0;Kerberosv5;%s;%s;%s;", guidString, generatedUpn, userDomain);
    if (!authString)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

cleanup:

    if (guidString)
    {
        LW_SAFE_FREE_STRING(guidString);
    }

    if (generatedUpn)
    {
        LW_SAFE_FREE_STRING(generatedUpn);
    }

    if (upn)
    {
        LW_SAFE_FREE_STRING(upn);
    }

    if (userSamAccount)
    {
        LW_SAFE_FREE_STRING(userSamAccount);
    }

    if (userDomain)
    {
        LW_SAFE_FREE_STRING(userDomain);
    }

    if (macError)
    {
        if (authString)
        {
            free(authString);
            authString = NULL;
        }
    }
    *AuthString = authString;
    return macError;
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
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrGeneratedUID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrNFSHomeDirectory);
        // TODO: Implement Password?
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPassword);
        //LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPasswordPlus);
        //LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPasswordPolicyOptions);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPrimaryGroupID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrUniqueID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrUserShell);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrAuthenticationAuthority);
        //LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrGroupMembership);
        //LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrHomeDirectory);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordName);
        //LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPwdAgingPolicy);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrChange);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrExpire);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMetaNodeLocation);
		LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXFlags);
		LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrMCXSettings);
		LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrMCXSettings);
        //LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrTimeToLive);
        // TODO: Add idx_kDSNAttrRecordType?
    }

    for (iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(_attributeSet, iAttr))
        {
            switch (iAttr)
            {
            case LWIAttrLookup::idx_kDS1AttrDistinguishedName:
                macError = SetDistinguishedName(pRecord, pUser->pw_name, bSetValue);
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
            case LWIAttrLookup::idx_kDSNAttrAuthenticationAuthority:
                macError = AddAttributeEx(&pAttribute, pRecord, kDSNAttrAuthenticationAuthority, NULL);
                GOTO_CLEANUP_ON_MACERROR(macError);

                if (bSetValue)
                {
                    /* Try our best, but try not fail if we cannot get the auth string */
                    if (!authString)
                    {
                        macError = GetAuthString(pUser, &authString);
                    }
                    if (authString)
                    {
                        macError = SetAttributeValue(pAttribute, authString);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }
                    else
                    {
                        macError = SetAttributeValue(pAttribute, kDSValueAuthAuthorityDefault);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }
                }
                break;
            case LWIAttrLookup::idx_kDSNAttrGroupMembership:
                macError = SetGroupMembership(pRecord, pUser, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDSNAttrHomeDirectory:
                macError = SetHomeDirectory(pRecord, pUser, bSetValue);
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
                macError = SetMetaNodeLocation(pRecord, bSetValue);
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
            case LWIAttrLookup::idx_kDS1AttrMCXFlags:
            case LWIAttrLookup::idx_kDS1AttrMCXSettings:
            case LWIAttrLookup::idx_kDSNAttrMCXSettings:
                // Skipping since not applicable to Likewise Open
            break;
            default:
                LOG("Unsupported attribute index - %d", iAttr);
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
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrGeneratedUID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPassword);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDS1AttrPrimaryGroupID);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrGroupMembership);
        LWI_BITVECTOR_SET(_attributeSet, LWIAttrLookup::idx_kDSNAttrRecordName);
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
                macError = SetDistinguishedName(pRecord, pGroup->gr_name, bSetValue);
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
            case LWIAttrLookup::idx_kDSNAttrMetaNodeLocation:
                macError = SetMetaNodeLocation(pRecord, bSetValue);
                GOTO_CLEANUP_ON_MACERROR(macError);
                break;
            case LWIAttrLookup::idx_kDS1AttrMCXFlags:
            case LWIAttrLookup::idx_kDS1AttrMCXSettings:
            case LWIAttrLookup::idx_kDSNAttrMCXSettings:
                // Skipping since not applicable to Likewise Open
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
                LOG("Unsupported attribute index - %d", iAttr);
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
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;
    DWORD dwNumUsersFound = 0;
    DWORD iUser = 0;

    macError = GetUserObjects(&ppUserObjects, &dwNumUsersFound);
    GOTO_CLEANUP_ON_MACERROR(macError);

    for (iUser = 0; iUser < dwNumUsersFound; iUser++)
    {
        macError = AddUserRecordHelper(ppUserObjects[iUser]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

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
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD iGroup = 0;

    macError = GetGroupObjects(&ppGroupObjects, &dwNumGroupsFound);
    GOTO_CLEANUP_ON_MACERROR(macError);

    for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
    {
        macError = AddGroupRecordHelper(ppGroupObjects[iGroup]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppGroupObjects)
    {
        FreeObjectList(dwNumGroupsFound, ppGroupObjects);
    }

    return macError;
}

long
LWIQuery::QueryUserInformationByName(const char* pszName)
{
    long macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    if ( !strcmp(pszName, kDSRecordsAll) )
    {
        macError = QueryAllUserInformation(pszName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = GetUserObjectFromName(pszName, &ppUserObjects);
        GOTO_CLEANUP_ON_MACERROR(macError);

        macError = AddUserRecordHelper(ppUserObjects[0]);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

cleanup:

    if (ppUserObjects) {
        FreeObjectList(1, ppUserObjects);
    }

    return macError;
}

long
LWIQuery::QueryUserInformationById(uid_t uid)
{
    long macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    macError = GetUserObjectFromId(uid, &ppUserObjects);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = AddUserRecordHelper(ppUserObjects[0]);
    GOTO_CLEANUP_ON_MACERROR(macError);

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
    uid_t uid;

    macError = ExtractUIDFromGeneratedUID(pszGUID, uid);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = QueryUserInformationById(uid);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

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

    macError = GetGroupInformationById(gid);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

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

    return macError;
}

long
LWIQuery::QueryComputerGroupInformationByName(const char* pszName)
{
    long macError = eDSNoErr;

    return macError;
}

long
LWIQuery::QueryComputerInformationByName(const char* pszName)
{
    long macError = eDSNoErr;

    return macError;
}

long
LWIQuery::QueryGroupsForUser(
    IN PCSTR pszUserSid
    )
{
    MACERROR macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppGroups = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD iGroup = 0;

    macError = GetUserGroups(pszUserSid, &ppGroups, &dwNumGroupsFound);
    GOTO_CLEANUP_ON_MACERROR(macError);

    for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
    {
        macError = AddGroupRecordHelper(ppGroups[iGroup]);
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
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    macError = GetUserObjectFromName(pszName, &ppUserObjects);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = QueryGroupsForUser(ppUserObjects[0]->pszObjectSid);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (ppUserObjects) {
        FreeObjectList(1, ppUserObjects);
    }

    return macError;
}

long
LWIQuery::QueryGroupsForUserById(
    IN uid_t uid
    )
{
    MACERROR macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;

    macError = GetUserObjectFromId(uid, &ppUserObjects);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = QueryGroupsForUser(ppUserObjects[0]->pszObjectSid);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (ppUserObjects) {
        FreeObjectList(1, ppUserObjects);
    }

    return macError;
}

long
LWIQuery::GetGroupInformationById(
    IN gid_t gid
    )
{
    long macError = eDSNoErr;
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;

    macError = GetGroupObjectFromId(gid, &ppGroupObjects);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = AddGroupRecordHelper(ppGroupObjects[0]);
    GOTO_CLEANUP_ON_MACERROR(macError);

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
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;

    macError = GetGroupObjectFromName(pszName, &ppGroupObjects);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = AddGroupRecordHelper(ppGroupObjects[0]);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (ppGroupObjects)
    {
        FreeObjectList(1, ppGroupObjects);
    }

    return macError;
}


long
LWIQuery::SetDistinguishedName(PDSRECORD pRecord, const char* pszUsername, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    // Users distinguished or real name
    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDS1AttrDistinguishedName, pszUsername, pRecord, &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrDistinguishedName, pRecord, &pAttribute);
    }

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

    if (bSetValue)
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
        LWIFreeString(pszGUID);
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

    if (bSetValue)
    {
        macError = BuildGeneratedGID(pGroup->gr_gid, &pszGUID);
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
        LWIFreeString(pszGUID);
    }

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
LWIQuery::SetPassword(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // Holds the password or credential value
    long macError = eDSNoErr;
#if 0
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDS1AttrPassword, pUser->pw_passwd, pRecord, &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrPassword, pRecord, &pAttribute);
    }
#endif
    return macError;
}

long
LWIQuery::SetPassword(PDSRECORD pRecord, const PLWIGROUP pGroup, bool bSetValue)
{
    // Holds the password or credential value
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDS1AttrPassword, pGroup->gr_passwd, pRecord, &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrPassword, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetPasswordPlus(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // Holds marker data to indicate possible authentication redirection
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDS1AttrPasswordPlus, kDSValueNonCryptPasswordMarker, pRecord, &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrPasswordPlus, pRecord, &pAttribute);
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

    if (bSetValue)
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

    if (bSetValue)
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
LWIQuery::SetUniqueID(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
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
LWIQuery::SetUserShell(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDS1AttrUserShell, pUser->pw_shell, pRecord, &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrUserShell, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetAuthenticationAuthority(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDSNAttrAuthenticationAuthority, kDSValueAuthAuthorityDefault, pRecord, &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDSNAttrAuthenticationAuthority, pRecord, &pAttribute);
    }

    return macError;
}

long
LWIQuery::SetGroupMembership(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // A list of users that belong to a given group record
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
LWIQuery::SetHomeDirectory(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    // kDSNAttrHomeDirectory
    return eDSNoErr;
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
#if 0
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(
           kDS1AttrChange,
            (const char*)&pUser->pw_change,
            sizeof(pUser->pw_change),
            pRecord,
            &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrChange, pRecord, &pAttribute);
    }
#endif
    return macError;
}

long
LWIQuery::SetPasswordExpire(PDSRECORD pRecord, const PLWIUSER pUser, bool bSetValue)
{
    long macError = eDSNoErr;
#if 0
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDS1AttrExpire,
                                        (const char*)&pUser->pw_expire,
                                        sizeof(pUser->pw_expire),
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrExpire, pRecord, &pAttribute);
    }
#endif
    return macError;
}

long
LWIQuery::SetMetaNodeLocation(PDSRECORD pRecord, bool bSetValue)
{
    long macError = eDSNoErr;
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDSNAttrMetaNodeLocation,
                                        PLUGIN_ROOT_PATH,
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
 * Format is an unsigned 32 bit representing seconds. ie. 300 is 5 minutes.
 */
long
LWIQuery::SetTimeToLive(PDSRECORD pRecord, bool bSetValue)
{
    long macError = eDSNoErr;
#if 0
    PDSATTRIBUTE pAttribute = NULL;

    if (bSetValue)
    {
        macError = AddAttributeAndValue(kDS1AttrTimeToLive,
                                        (const char*)&DEFAULT_ATTRIBUTE_TTL_SECONDS,
                                        sizeof(DEFAULT_ATTRIBUTE_TTL_SECONDS),
                                        pRecord,
                                        &pAttribute);
    }
    else
    {
        macError = AddAttribute(kDS1AttrTimeToLive, pRecord, &pAttribute);
    }
#endif
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
        LWIFreeMemory(pAttributeValue->pszValue);
    }
    LWIFreeMemory(pAttributeValue);
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

    LWIFreeMemory(pAttribute);
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

    LWIFreeMemory(pRecord);
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
       LWIFreeMemory(pHeader->pOffsets);
    }

    LWIFreeMemory(pHeader);
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
    IN PLSA_SECURITY_OBJECT pUserObject
    )
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;
    PAD_USER_ATTRIBUTES padUserInfo = NULL;
    PSTR pszUserName = NULL;

    macError = GetADUserInfo(pUserObject->userInfo.uid, &padUserInfo);
    if (macError)
    {
        // LOG("No cached AD attributes found for user: %s",
        //     pUserObject->userInfo.pszUnixName ? pUserObject->userInfo.pszUnixName : "<null>");
        macError = eDSNoErr;
    }

    if (padUserInfo)
    {
        pszUserName = padUserInfo->pszDisplayName;
    }

    if (!pszUserName)
    {
        pszUserName = pUserObject->userInfo.pszUnixName;
    }

    macError = CreateLWIUser(pUserObject->userInfo.pszUnixName, /* Record Name */
                             pszUserName, /* Display name */
                             NULL, /* Password */
                             NULL, /* Class */
                             pUserObject->userInfo.pszGecos,
                             pUserObject->userInfo.pszHomedir,
                             NULL,
                             NULL,
                             NULL,
                             pUserObject->userInfo.pszShell,
                             pUserObject->userInfo.uid,
                             pUserObject->userInfo.gid,
                             padUserInfo,
                             &pUser);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = AddUserRecord(pUser, pszUserName);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    FreeLWIUser(pUser);
    FreeADUserInfo(padUserInfo);

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

    macError = BuildRecord(kDSStdRecordTypeUsers, pUser->pw_name, &pRecord);
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
    IN PLSA_SECURITY_OBJECT pGroupObject
    )
{
    long macError = eDSNoErr;
    PLWIGROUP pGroup = NULL;
    PLWIMEMBERLIST pMembers = NULL;
    PLSA_SECURITY_OBJECT* ppGroupMembers = NULL;
    DWORD dwMemberCount = 0;

    macError = ExpandGroupMembers(pGroupObject->pszObjectSid, &ppGroupMembers, &dwMemberCount);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CreateMemberList(dwMemberCount, ppGroupMembers, &pMembers);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = CreateLWIGroup(pGroupObject->groupInfo.pszUnixName, /* Group Display Name */
                              pGroupObject->groupInfo.pszPasswd,
                              pGroupObject->groupInfo.pszUnixName, /* Group Name */
                              NULL, /* Comment */
                              pMembers,
                              NULL, /* Generated UID - Computed automatically later */
                              pGroupObject->groupInfo.gid,
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

    macError = BuildRecord(kDSStdRecordTypeGroups, pGroup->gr_name, &pRecord);
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
LWIQuery::BuildRecord(
    const char * pszType,
    const char * pszName,
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

    macError = LWIAllocateMemory(sizeof(DSRECORD), (PVOID*)&pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIAllocateString(pszName, &pRecord->pszName);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord->nameLen = strlen(pszName);

    macError = LWIAllocateString(pszType, &pRecord->pszType);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord->typeLen = strlen(pszType);

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

    macError = LWIAllocateMemory(sizeof(DSATTRIBUTE), (PVOID*)&pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIAllocateString(pszAttributeName, &pAttribute->pszName);
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

    macError = LWIAllocateMemory(sizeof(DSATTRIBUTEVALUE), (PVOID*)&pAttrValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pAttrValue->valLen = valLen;

    if (valLen)
    {
        macError = LWIAllocateMemory(valLen, (PVOID*)&pAttrValue->pszValue);
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
    return AddAttributeAndValue(pszAttributeName, pszValue, strlen(pszValue), pRecord, ppAttribute);
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
    uint32_t headerType
    )
{
    long macError = eDSNoErr;
    int  nRecords = 0;
    int  iRecord = 0;
    PDSRECORD pRecord = NULL;
    unsigned long offset = 0;

    macError = DetermineRecordsToFitInBuffer(maxBufferSize, nRecords);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (nRecords > 0)
    {
        macError = WriteHeader(buffer, nRecords, offset, headerType);
        GOTO_CLEANUP_ON_MACERROR(macError);

        for (pRecord = _pRecordListHead; pRecord && (iRecord < nRecords); pRecord = pRecord->pNext, iRecord++)
        {
            macError = WriteRecord(buffer, iRecord, pRecord, offset);
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
    }

    nRecordsWritten = nRecords;
    bytesWritten = offset;

    LOG("WriteResponse success, bytesWritten = %d, recordsWritten = %d", bytesWritten, nRecordsWritten );

cleanup:

    return macError;
}

long
LWIQuery::WriteGDNIResponse(
    char* buffer,
    unsigned long maxBufferSize,
    unsigned long& bytesWritten,
    unsigned long& nRecordsWritten
    )
{
    return WriteResponse(buffer, maxBufferSize, bytesWritten, nRecordsWritten, 'Gdni');
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

    macError = LWIAllocateMemory(sizeof(DSATTRIBUTEVALUE), (PVOID*)&pAttrValue);
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
       macError = LWIAllocateMemory(pAttrValue->valLen+1, (PVOID*)&pAttrValue->pszValue);
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

    macError = LWIAllocateMemory(sizeof(DSATTRIBUTE), (PVOID*)&pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* attribute name length: 2 bytes */
    CHECK_INVALID_OFFSET(woffset + 2, bufferSize);

    memcpy(&pAttribute->nameLen, buffer+woffset, 2);
    woffset += 2;

    /* attribute name string:         */
    if (pAttribute->nameLen)
    {
        CHECK_INVALID_OFFSET(woffset + pAttribute->nameLen, bufferSize);

        macError = LWIAllocateMemory(pAttribute->nameLen+1, (PVOID*)&pAttribute->pszName);
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

    macError = LWIAllocateMemory(sizeof(DSRECORD), (PVOID*)&pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    /* record type length:   2 bytes */
    CHECK_INVALID_OFFSET(woffset+2, bufferSize);

    memcpy(&pRecord->typeLen, buffer+woffset, 2);
    woffset += 2;

    /* record type string:           */
    if (pRecord->typeLen)
    {
        CHECK_INVALID_OFFSET(woffset+pRecord->typeLen, bufferSize);

        macError = LWIAllocateMemory(pRecord->typeLen+1, (PVOID*)&pRecord->pszType);
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

        macError = LWIAllocateMemory(pRecord->nameLen+1, (PVOID*)&pRecord->pszName);
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

    macError = LWIAllocateMemory(sizeof(DSMESSAGEHEADER), (PVOID*)&pHeader);
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

        macError = LWIAllocateMemory(4*pHeader->nRecords, (PVOID*)&pHeader->pOffsets);
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

    macError = LWIAllocateMemory(sizeof(DSMESSAGE), (PVOID*)&pMessage);
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

    macError = LWIAllocateMemory(sizeof(*pAttributeEntry) + pAttribute->nameLen + MAGIC_PADDING, (PVOID*)&pAttributeEntry);
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
        LWIFreeMemory(pAttributeEntry);
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

    macError = LWIAllocateMemory(sizeof(*pAttributeValueEntry) + pAttributeValue->valLen + MAGIC_PADDING, (PVOID*)&pAttributeValueEntry);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pAttributeValueEntry->fAttributeValueData.fBufferSize = pAttributeValue->valLen + MAGIC_PADDING;
    pAttributeValueEntry->fAttributeValueData.fBufferLength = pAttributeValue->valLen;
    memcpy(pAttributeValueEntry->fAttributeValueData.fBufferData, pAttributeValue->pszValue, pAttributeValue->valLen);

    pAttributeValueEntry->fAttributeValueID = LWICRC::GetCRC(pAttributeValue->pszValue, pAttributeValue->valLen);

cleanup:
    if (macError)
    {
        LWIFreeMemory(pAttributeValueEntry);
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
LWIQuery::DetermineRecordsToFitInBuffer(unsigned long maxBufferSize, int& nRecords)
{
    long macError = eDSNoErr;
    PDSRECORD pRecord = NULL;
    unsigned int recordSize = 0;
    unsigned long currentSize = 0;
    int result = 0;

    LOG_ENTER("maxBufferSize: %ld", maxBufferSize);

    if (_pRecordListHead)
	{
        for (pRecord = _pRecordListHead;
	         pRecord; 
		     pRecord = pRecord->pNext)
        {
            macError = GetRecordSize(pRecord, recordSize);
            GOTO_CLEANUP_ON_MACERROR(macError);

            currentSize += 4; // This is in the offset in the header
            currentSize += 4; // This is to store the record size
            // before the contents of the record
            currentSize += recordSize;
			
			if(currentSize > maxBufferSize)
			{
			    break;
			}
			
		    result++;
		}
		
		if (!pRecord)
		{
            currentSize += GetHeaderSize(result);
		}
	}

	if (currentSize > maxBufferSize)
    {
        LOG("No record will fit into caller's buffer, size needed is %d", currentSize);
		nRecords = 0;
		macError = eDSBufferTooSmall;
		GOTO_CLEANUP_ON_MACERROR(macError);
	}

    LOG("%d record(s) will be written into caller's buffer, size needed is %d", result, currentSize);
    nRecords = result;

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
LWIQuery::WriteAttributeValue(char* buffer, PDSATTRIBUTEVALUE pAttributeValue, unsigned long& offset)
{
    memcpy(buffer+offset, &pAttributeValue->valLen, 4);
    offset += 4;
    memcpy(buffer+offset, pAttributeValue->pszValue, pAttributeValue->valLen);
    offset += pAttributeValue->valLen;

    return eDSNoErr;
}
