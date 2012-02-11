/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        adldapdef.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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

