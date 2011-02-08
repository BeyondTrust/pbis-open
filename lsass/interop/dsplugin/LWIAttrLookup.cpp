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

#include "LWIAttrLookup.h"
#include "LWIMutexLockGuard.h"

static inline long AddToDictionary(
    IN OUT CFMutableDictionaryRef Dictionary,
    IN CFStringRef Key,
    IN LWIAttrLookup::Index_t Value
    )
{
    int value = Value;
    CFNumberRef number = CFNumberCreate(NULL, kCFNumberIntType, &value);
    if (!number)
    {
        return eDSAllocationFailed;
    }
    CFDictionarySetValue(Dictionary, Key, number);
    CFRelease(number);
    return eDSNoErr;
}

#define _INIT_KEY(StringLiteral) \
    do { \
        macError = AddToDictionary(_dictionary, CFSTR(StringLiteral), idx_ ## StringLiteral); \
        GOTO_CLEANUP_ON_MACERROR(macError); \
    } while (0)

CFMutableDictionaryRef LWIAttrLookup::_dictionary = NULL;
const char* LWIAttrLookup::_type = "attribute";

long LWIAttrLookup::Initialize()
{
    long macError = eDSNoErr;

    _dictionary = CFDictionaryCreateMutable(NULL,
                                            0,
                                            &kCFCopyStringDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    if (!_dictionary)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    _INIT_KEY(kDS1AttrDistinguishedName);
    _INIT_KEY(kDS1AttrGeneratedUID);
    _INIT_KEY(kDS1AttrNFSHomeDirectory);
    _INIT_KEY(kDS1AttrPassword);
    _INIT_KEY(kDS1AttrPasswordPlus);
    _INIT_KEY(kDS1AttrPasswordPolicyOptions);
    _INIT_KEY(kDS1AttrPrimaryGroupID);
    _INIT_KEY(kDS1AttrUniqueID);
    _INIT_KEY(kDS1AttrUserShell);
    _INIT_KEY(kDSNAttrAuthenticationAuthority);
    _INIT_KEY(kDSNAttrGroupMembership);
    _INIT_KEY(kDSNAttrHomeDirectory);
    _INIT_KEY(kDSNAttrRecordName);
    _INIT_KEY(kDS1AttrPwdAgingPolicy);
    _INIT_KEY(kDS1AttrChange);
    _INIT_KEY(kDS1AttrExpire);
    _INIT_KEY(kDSNAttrMetaNodeLocation);
    _INIT_KEY(kDSNAttrGroupMembers);
    _INIT_KEY(kDS1AttrTimeToLive);
    _INIT_KEY(kDSAttributesAll);
    _INIT_KEY(kDSAttributesStandardAll);
    _INIT_KEY(kDSNAttrAuthMethod);
    _INIT_KEY(kDS1AttrReadOnlyNode);
    _INIT_KEY(kDSNAttrNodePath);
    _INIT_KEY(kDSNAttrRecordType);
    _INIT_KEY(kDSNAttrSubNodes);
    _INIT_KEY(kDS1AttrDataStamp);
    _INIT_KEY(K_DS_ATTR_TRUST_INFORMATION);
    _INIT_KEY(kDS1AttrMCXFlags);
    _INIT_KEY(kDS1AttrMCXSettings);
    _INIT_KEY(kDSNAttrMCXSettings);
    _INIT_KEY(kDS1AttrENetAddress);
    _INIT_KEY(kDSNAttrIPAddress);
    _INIT_KEY(kDS1AttrComment);
    _INIT_KEY(kDSNAttrComputers);
    _INIT_KEY(kDSNAttrKeywords);
    _INIT_KEY(kDSNAttrOriginalHomeDirectory);
    _INIT_KEY(kDS1AttrOriginalNFSHomeDirectory);
    _INIT_KEY(K_DS_ATTR_DISPLAY_NAME);
    _INIT_KEY(K_DS_ATTR_CN);
    _INIT_KEY(K_DS_ATTR_NAME);
    _INIT_KEY(kDS1AttrFirstName);
    _INIT_KEY(K_DS_ATTR_GIVEN_NAME);
    _INIT_KEY(kDS1AttrLastName);
    _INIT_KEY(K_DS_ATTR_SN);
    _INIT_KEY(K_DS_ATTR_AD_DOMAIN);
    _INIT_KEY(K_DS_ATTR_KERBEROS_PRINCIPAL);
    _INIT_KEY(K_DS_ATTR_USER_PRINCIPAL_NAME);
    _INIT_KEY(K_DS_ATTR_EMAIL_ADDRESS);
    _INIT_KEY(K_DS_ATTR_MS_EXCH_HOME_SERVER_NAME);
    _INIT_KEY(K_DS_ATTR_MS_EXCH_HOME_MDB);
    _INIT_KEY(K_DS_ATTR_TELEPHONE_NUMBER);
    _INIT_KEY(K_DS_ATTR_FACSIMILIE_TELEPHONE_NUMBER);
    _INIT_KEY(K_DS_ATTR_MOBILE);
    _INIT_KEY(K_DS_ATTR_STREET_ADDRESS);
    _INIT_KEY(K_DS_ATTR_POST_OFFICE_BOX);
    _INIT_KEY(K_DS_ATTR_CITY);
    _INIT_KEY(K_DS_ATTR_STATE);
    _INIT_KEY(K_DS_ATTR_POSTAL_CODE);
    _INIT_KEY(K_DS_ATTR_COUNTRY);
    _INIT_KEY(K_DS_ATTR_TITLE);
    _INIT_KEY(K_DS_ATTR_COMPANY_AD);
    _INIT_KEY(K_DS_ATTR_DEPARTMENT);
    _INIT_KEY(K_DS_ATTR_HOME_DIRECTORY);
    _INIT_KEY(K_DS_ATTR_HOME_DRIVE);
    _INIT_KEY(K_DS_ATTR_PWD_LAST_SET);
    _INIT_KEY(K_DS_ATTR_USER_ACCOUNT_CONTROL);
    _INIT_KEY(K_DS_ATTR_MAX_MINUTES_UNTIL_CHANGE_PASSWORD);
    _INIT_KEY(K_DS_ATTR_MAX_PWD_AGE);
    _INIT_KEY(K_DS_ATTR_MAX_FAILED_LOGIN_ATTEMPTS);
    _INIT_KEY(K_DS_ATTR_LOCKOUT_THRESHHOLD);
    _INIT_KEY(K_DS_ATTR_USING_HISTORY);
    _INIT_KEY(K_DS_ATTR_PWD_HISTORY_LENGTH);
    _INIT_KEY(K_DS_ATTR_MIN_CHARS);
    _INIT_KEY(K_DS_ATTR_MIN_PWD_LENGTH);
    _INIT_KEY(kDSNAttrCity);
    _INIT_KEY(kDSNAttrState);
    _INIT_KEY(kDSNAttrCountry);
    _INIT_KEY(K_DS_ATTR_COMPANY_MAC);
    _INIT_KEY(kDSNAttrDepartment);
    _INIT_KEY(kDSNAttrEMailAddress);
    _INIT_KEY(kDSNAttrFaxNumber);
    _INIT_KEY(kDSNAttrJobTitle);
    _INIT_KEY(kDSNAttrMobileNumber);
    _INIT_KEY(kDSNAttrPhoneNumber);
    _INIT_KEY(kDSNAttrPostalCode);
    _INIT_KEY(kDSNAttrStreet);
    _INIT_KEY(K_DS_ATTR_MIN_MINUTES_UNTIL_CHANGE_PASSWORD);
    _INIT_KEY(K_DS_ATTR_MIN_PWD_AGE);
    _INIT_KEY(K_DS_ATTR_MAX_PWD_AGE_DAYS);
    _INIT_KEY(K_DS_ATTR_MIN_PWD_AGE_DAYS);
    _INIT_KEY(K_DS_ATTR_DAYS_TILL_PWD_EXPIRES);

cleanup:

    if (macError)
    {
        Cleanup();
    }

    return macError;
}

void LWIAttrLookup::Cleanup()
{
    if (_dictionary)
    {
        CFRelease(_dictionary);
        _dictionary = NULL;
    }
}

long
LWIAttrLookup::GetVector(tDataListPtr List, PLWIBITVECTOR* ppVector)
{
    int macError = eDSNoErr;
    PLWIBITVECTOR pResult = NULL;
    tDataNodePtr pDataNode = NULL;
    int nNodes = 0;
    Index_t value = idx_unknown;

    macError = LWIMakeBitVector(idx_sentinel, &pResult);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (List != NULL)
    {
       nNodes = dsDataListGetNodeCount(List);

       for (int iNode = 0; iNode < nNodes; iNode++)
       {
           macError = dsDataListGetNodeAlloc(0,
                                             List,
                                             iNode+1,
                                             &pDataNode);
           GOTO_CLEANUP_ON_MACERROR(macError);
           value = GetIndex(pDataNode->fBufferData);
           if (value)
           {
              LWI_BITVECTOR_SET(pResult, value);
           }

           dsDataNodeDeAllocate(NULL, pDataNode);
           pDataNode = NULL;
       }
    }

    *ppVector = pResult;
    pResult = NULL;

cleanup:

    if (pResult)
    {
        LWIFreeBitVector(pResult);
    }

    if (pDataNode)
    {
        dsDataNodeDeAllocate(0, pDataNode);
    }

    return macError;
}

long
LWIAttrLookup::GetVector(tDataNodePtr Node, PLWIBITVECTOR* ppVector)
{
    return GetVector(Node->fBufferData, ppVector);
}

long
LWIAttrLookup::GetVector(const char* Item, PLWIBITVECTOR* ppVector)
{
    int macError = eDSNoErr;
    PLWIBITVECTOR pResult = NULL;
    Index_t value = idx_unknown;

    macError = LWIMakeBitVector(idx_sentinel, &pResult);
    GOTO_CLEANUP_ON_MACERROR(macError);

    value = GetIndex(Item);
    if (value)
    {
        LWI_BITVECTOR_SET(pResult, value);
    }

    *ppVector = pResult;
    pResult = NULL;

cleanup:

    if (pResult)
    {
        LWIFreeBitVector(pResult);
    }

    return macError;
}

LWIAttrLookup::Index_t
LWIAttrLookup::GetIndex(const char* Item)
{
    CFTypeRef num = NULL;
    CFStringRef key = NULL;
    Index_t value = idx_unknown;
    Index_t index = idx_unknown;

    key = CFStringCreateWithCString(NULL, Item, kCFStringEncodingASCII);
    num = CFDictionaryGetValue(_dictionary, key);
    if (num && CFNumberGetValue((CFNumberRef)num, kCFNumberIntType, &value))
    {
        index = value;
    }
    else
    {
#ifdef SHOW_DEBUG_SPEW
        LOG("Unsupported %s - %s", _type, Item);
#endif
    }

    if (key)
    {
        CFRelease(key);
    }

    return index;
}
