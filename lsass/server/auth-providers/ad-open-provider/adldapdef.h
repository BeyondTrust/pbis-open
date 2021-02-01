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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        adldapdef.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        AD LDAP definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *
 */

#ifndef __ADLDAPDEF_H__
#define __ADLDAPDEF_H__

#define AD_LDAP_NAME_TAG         "name"
#define AD_LDAP_SAM_NAME_TAG     "sAMAccountName"
#define AD_LDAP_DISPLAY_NAME_TAG "displayName"
#define AD_LDAP_PASSWD_TAG       "unixUserPassword"
#define AD_LDAP_UID_TAG          "uidNumber"
#define AD_LDAP_ALIAS_TAG        "uid"
#define AD_LDAP_GID_TAG          "gidNumber"
#define AD_LDAP_HOMEDIR_TAG      "unixHomeDirectory"
#define AD_LDAP_GECOS_TAG        "gecos"
#define AD_LDAP_SHELL_TAG        "loginShell"
#define AD_LDAP_MEMBER_TAG       "member"
#define AD_LDAP_MEMBEROF_TAG     "memberOf"
#define AD_LDAP_SEC_DESC_TAG     "nTSecurityDescriptor"
#define AD_LDAP_KEYWORDS_TAG     "keywords"
#define AD_LDAP_DESCRIPTION_TAG  "description"
#define AD_LDAP_OBJECTSID_TAG    "objectSid"
#define AD_LDAP_UPN_TAG          "userPrincipalName"
#define AD_LDAP_PRIMEGID_TAG     "primaryGroupID"
#define AD_LDAP_USER_CTRL_TAG    "userAccountControl"
#define AD_LDAP_PWD_LASTSET_TAG  "pwdLastSet"
#define AD_LDAP_ACCOUT_EXP_TAG   "accountExpires"
#define AD_LDAP_MAX_PWDAGE_TAG   "maxPwdAge"
#define AD_LDAP_DESCRIP_TAG      "description"
#define AD_LDAP_GROUP_TYPE       "groupType"
#define AD_LDAP_OBJECTCLASS_TAG  "objectClass"
#define AD_LDAP_CONFNAM_CTXT_TAG "configurationNamingContext"
#define AD_LDAP_NETBIOS_TAG      "netBIOSName"
#define AD_LDAP_DN_TAG           "distinguishedName"
#define AD_LDAP_DISPLAY_NAME_TAG "displayName"
#define AD_LDAP_WINDOWSHOMEFOLDER_TAG   "homeDirectory"
#define AD_LDAP_LOCALWINDOWSHOMEFOLDER_TAG   "unixWindowsHomeDirectory"

// Pseudo-only Attributes
#define AD_LDAP_BACKLINK_PSEUDO_TAG   "backLink"

#endif /* __ADLDAPDEF_H__ */

