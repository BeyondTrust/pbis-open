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

#ifndef __LWIATTRLOOKUP_H__
#define __LWIATTRLOOKUP_H__

#include "LWIPlugIn.h"
#include "LWIMutexLock.h"

#define K_DS_ATTR_TRUST_INFORMATION "dsAttrTypeStandard:TrustInformation"

#define K_DS_ATTR_DISPLAY_NAME "dsAttrTypeNative:displayName" // pszDisplayName;
#define K_DS_ATTR_CN "dsAttrTypeNative:cn" // pszDisplayName;
#define K_DS_ATTR_NAME "dsAttrTypeNative:name" // pszDisplayName;
#define K_DS_ATTR_GIVEN_NAME "dsAttrTypeNative:givenName" // pszFirstName;
#define K_DS_ATTR_SN "dsAttrTypeNative:sn" // pszLastName;
#define K_DS_ATTR_AD_DOMAIN "dsAttrTypeNative:ADDomain" // pszADDomain;
#define K_DS_ATTR_KERBEROS_PRINCIPAL "dsAttrTypeNative:kerberosPrincipal" // pszKerberosPrincipal;
#define K_DS_ATTR_USER_PRINCIPAL_NAME "dsAttrTypeNative:userPrincipalName" // pszKerberosPrincipal;
#define K_DS_ATTR_EMAIL_ADDRESS "dsAttrTypeNative:mail" // pszEMailAddress;
#define K_DS_ATTR_MS_EXCH_HOME_SERVER_NAME "dsAttrTypeNative:msExchHomeServerName" // pszMSExchHomeServerName;
#define K_DS_ATTR_MS_EXCH_HOME_MDB "dsAttrTypeNative:homeMDB" // pszMSExchHomeMDB;
#define K_DS_ATTR_TELEPHONE_NUMBER "dsAttrTypeNative:telephoneNumber" // pszTelephoneNumber;
#define K_DS_ATTR_FACSIMILIE_TELEPHONE_NUMBER "dsAttrTypeNative:facsimilieTelephoneNumber" // pszFaxTelephoneNumber;
#define K_DS_ATTR_MOBILE "dsAttrTypeNative:mobile" // pszMobileTelephoneNumber;
#define K_DS_ATTR_STREET_ADDRESS "dsAttrTypeNative:streetAddress" // pszStreetAddress;
#define K_DS_ATTR_POST_OFFICE_BOX "dsAttrTypeNative:postOfficeBox" // pszPostOfficeBox;
#define K_DS_ATTR_CITY "dsAttrTypeNative:l" // pszCity;
#define K_DS_ATTR_STATE "dsAttrTypeNative:st" // pszState;
#define K_DS_ATTR_POSTAL_CODE "dsAttrTypeNative:postalCode" // pszPostalCode;
#define K_DS_ATTR_COUNTRY "dsAttrTypeNative:co" // pszCountry;
#define K_DS_ATTR_TITLE "dsAttrTypeNative:title" // Mac and AD - pszTitle;
#define K_DS_ATTR_COMPANY_MAC "dsAttrTypeStandard:Company" // Mac and AD - pszCompany;
#define K_DS_ATTR_COMPANY_AD "dsAttrTypeNative:company" // Mac and AD - pszCompany;
#define K_DS_ATTR_DEPARTMENT "dsAttrTypeNative:department" // Mac and AD - pszDepartment;
#define K_DS_ATTR_HOME_DIRECTORY "dsAttrTypeNative:homeDirectory" // Mac and AD - pszHomeDirectory;
#define K_DS_ATTR_HOME_DRIVE "dsAttrTypeNative:homeDrive" // Mac and AD - pszHomeDrive;
#define K_DS_ATTR_PWD_LAST_SET "dsAttrTypeNative:pwdLastSet" // Mac and AD - pszPasswordLastSet;
#define K_DS_ATTR_USER_ACCOUNT_CONTROL "dsAttrTypeNative:userAccountControl" // Mac and AD - pszUserAccountControl;
#define K_DS_ATTR_MAX_MINUTES_UNTIL_CHANGE_PASSWORD "dsAttrTypeNative:maxMinutesUntilChangePassword" // Mac - pszMaxMinutesUntilChangePassword;
#define K_DS_ATTR_MAX_PWD_AGE "dsAttrTypeNative:maxPwdAge" // AD - pszMaxMinutesUntilChangePassword;
#define K_DS_ATTR_MAX_FAILED_LOGIN_ATTEMPTS "dsAttrTypeNative:maxFailedLoginAttempts" // Mac - pszMaxFailedLoginAttempts;
#define K_DS_ATTR_LOCKOUT_THRESHHOLD "dsAttrTypeNative:lockoutThreshhold" // AD - pszMaxFailedLoginAttempts;
#define K_DS_ATTR_USING_HISTORY "dsAttrTypeNative:usingHistory" // Mac - pszAllowedPasswordHistory;
#define K_DS_ATTR_PWD_HISTORY_LENGTH "dsAttrTypeNative:pwdHistoryLength" // AD - pszAllowedPasswordHistory;
#define K_DS_ATTR_MIN_CHARS "dsAttrTypeNative:minChars" // Mac - pszMinCharsAllowedInPassword;
#define K_DS_ATTR_MIN_PWD_LENGTH "dsAttrTypeNative:minPwdLength" // AD - pszMinCharsAllowedInPassword;
#define K_DS_ATTR_MIN_MINUTES_UNTIL_CHANGE_PASSWORD "dsAttrTypeNative:minMinutesUntilChangePassword" // Mac - pszMinMinutesUntilChangePassword;
#define K_DS_ATTR_MIN_PWD_AGE "dsAttrTypeNative:minPwdAge" // AD - pszMinMinutesUntilChangePassword;
#define K_DS_ATTR_MAX_PWD_AGE_DAYS "dsAttrTypeNative:maxPwdAgeDays" // calculated from - pszMaxMinutesUntilChangePassword
#define K_DS_ATTR_MIN_PWD_AGE_DAYS "dsAttrTypeNative:minPwdAgeDays" // calculated from - pszMinMinutesUntilChangePassword
#define K_DS_ATTR_DAYS_TILL_PWD_EXPIRES "dsAttrTypeNative:daysTillPwdExpires" // calculated current time and pwd attrs

class LWIAttrLookup
{
public:

    typedef enum
    {
        idx_unknown,                         //  0
        idx_kDS1AttrDistinguishedName,       //  1
        idx_kDS1AttrGeneratedUID,            //  2
        idx_kDS1AttrNFSHomeDirectory,        //  3
        idx_kDS1AttrPassword,                //  4
        idx_kDS1AttrPasswordPlus,            //  5
        idx_kDS1AttrPasswordPolicyOptions,   //  6
        idx_kDS1AttrPrimaryGroupID,          //  7
        idx_kDS1AttrUniqueID,                //  8
        idx_kDS1AttrUserShell,               //  9
        _idx_unused_10,                      // 10 formerly idx_kDSNAttrAuthenticationAuthority
        idx_kDSNAttrGroupMembership,         // 11
        idx_kDSNAttrHomeDirectory,           // 12
        idx_kDSNAttrRecordName,              // 13
        idx_kDS1AttrPwdAgingPolicy,          // 14
        idx_kDS1AttrChange,                  // 15
        idx_kDS1AttrExpire,                  // 16
        idx_kDSNAttrMetaNodeLocation,        // 17
        idx_kDSNAttrGroupMembers,            // 18
        idx_kDS1AttrTimeToLive,              // 19
        idx_kDSAttributesAll,                // 20
        idx_kDSAttributesStandardAll,        // 21
        idx_kDSNAttrAuthMethod,              // 22
        idx_kDS1AttrReadOnlyNode,            // 23
        idx_kDSNAttrNodePath,                // 24
        idx_kDSNAttrRecordType,              // 25
        idx_kDSNAttrSubNodes,                // 26
        idx_kDS1AttrDataStamp,               // 27
        idx_K_DS_ATTR_TRUST_INFORMATION,     // 28
        idx_kDS1AttrMCXFlags,                // 29
        idx_kDS1AttrMCXSettings,             // 30
        idx_kDSNAttrMCXSettings,             // 31
        idx_kDS1AttrENetAddress,             // 32
        idx_kDSNAttrIPAddress,               // 33
        idx_kDS1AttrComment,                 // 34
        idx_kDSNAttrComputers,               // 35
        idx_kDSNAttrKeywords,                // 36
        idx_kDSNAttrOriginalHomeDirectory,   // 37
        idx_kDS1AttrOriginalNFSHomeDirectory,// 38
        idx_K_DS_ATTR_DISPLAY_NAME,          // 40
        idx_K_DS_ATTR_CN,                    // 41
        idx_K_DS_ATTR_NAME,                  // 42
        idx_kDS1AttrFirstName,               // 43
        idx_K_DS_ATTR_GIVEN_NAME,            // 44
        idx_kDS1AttrLastName,                // 45
        idx_K_DS_ATTR_SN,                    // 46
        idx_K_DS_ATTR_AD_DOMAIN,             // 47
        idx_K_DS_ATTR_KERBEROS_PRINCIPAL,    // 48
        idx_K_DS_ATTR_USER_PRINCIPAL_NAME,   // 49
        idx_K_DS_ATTR_EMAIL_ADDRESS,         // 50
        idx_K_DS_ATTR_MS_EXCH_HOME_SERVER_NAME,// 51
        idx_K_DS_ATTR_MS_EXCH_HOME_MDB,      // 52
        idx_K_DS_ATTR_TELEPHONE_NUMBER,      // 53
        idx_K_DS_ATTR_FACSIMILIE_TELEPHONE_NUMBER,// 54
        idx_K_DS_ATTR_MOBILE,                // 55
        idx_K_DS_ATTR_STREET_ADDRESS,        // 56
        idx_K_DS_ATTR_POST_OFFICE_BOX,       // 57
        idx_K_DS_ATTR_CITY,                  // 58
        idx_K_DS_ATTR_STATE,                 // 59
        idx_K_DS_ATTR_POSTAL_CODE,           // 60
        idx_K_DS_ATTR_COUNTRY,               // 61
        idx_K_DS_ATTR_TITLE,                 // 62
        idx_K_DS_ATTR_COMPANY_AD,            // 63
        idx_K_DS_ATTR_DEPARTMENT,            // 64
        idx_K_DS_ATTR_HOME_DIRECTORY,        // 65
        idx_K_DS_ATTR_HOME_DRIVE,            // 66
        idx_K_DS_ATTR_PWD_LAST_SET,          // 67
        idx_K_DS_ATTR_USER_ACCOUNT_CONTROL,  // 68
        idx_K_DS_ATTR_MAX_MINUTES_UNTIL_CHANGE_PASSWORD,// 69
        idx_K_DS_ATTR_MAX_PWD_AGE,           // 70
        idx_K_DS_ATTR_MAX_FAILED_LOGIN_ATTEMPTS,// 71
        idx_K_DS_ATTR_LOCKOUT_THRESHHOLD,    // 72
        idx_K_DS_ATTR_USING_HISTORY,         // 73
        idx_K_DS_ATTR_PWD_HISTORY_LENGTH,    // 74
        idx_K_DS_ATTR_MIN_CHARS,             // 75
        idx_K_DS_ATTR_MIN_PWD_LENGTH,        // 76
        idx_kDSNAttrCity,                    // 77
        idx_kDSNAttrState,                   // 78
        idx_kDSNAttrCountry,                 // 79
        idx_K_DS_ATTR_COMPANY_MAC,           // 80
        idx_kDSNAttrDepartment,              // 81
        idx_kDSNAttrEMailAddress,            // 82
        idx_kDSNAttrFaxNumber,               // 83
        idx_kDSNAttrJobTitle,                // 84
        idx_kDSNAttrMobileNumber,            // 85
        idx_kDSNAttrPhoneNumber,             // 86
        idx_kDSNAttrPostalCode,              // 87
        idx_kDSNAttrStreet,                  // 88
        idx_K_DS_ATTR_MIN_MINUTES_UNTIL_CHANGE_PASSWORD, // 89
        idx_K_DS_ATTR_MIN_PWD_AGE,           // 90
        idx_K_DS_ATTR_MAX_PWD_AGE_DAYS,      // 91
        idx_K_DS_ATTR_MIN_PWD_AGE_DAYS,      // 92
        idx_K_DS_ATTR_DAYS_TILL_PWD_EXPIRES, // 93
        idx_kDSNAttrSMBHome,                 // 94
        idx_sentinel                         // 95
    }  Index_t;
    

private:

    LWIAttrLookup();
    ~LWIAttrLookup();

    LWIAttrLookup(const LWIAttrLookup& other);
    LWIAttrLookup& operator=(const LWIAttrLookup& other);

public:

    static long Initialize();
    static void Cleanup();

    static long GetVector(tDataListPtr List, PLWIBITVECTOR* ppVector);
    static long GetVector(tDataNodePtr Node, PLWIBITVECTOR* ppVector);
    static long GetVector(const char* Item, PLWIBITVECTOR* ppVector);
    static Index_t GetIndex(const char* Item);

private:

    static CFMutableDictionaryRef _dictionary;
    static const char* _type;
};

#endif /* __LWIATTRLOOKUP_H__ */

