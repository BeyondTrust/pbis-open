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
 *        adldap_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AD LDAP helper functions (public header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSALDAP_MARSHAL_USER_H__
#define __LSALDAP_MARSHAL_USER_H__

#define LSA_AD_UF_DEFAULT            0x00000000
#define LSA_AD_UF_ACCOUNTDISABLE     0x00000002
#define LSA_AD_UF_LOCKOUT            0x00000010
#define LSA_AD_UF_CANT_CHANGE_PASSWD 0x00000040
#define LSA_AD_UF_DONT_EXPIRE_PASSWD 0x00010000
#define LSA_AD_UF_PASSWORD_EXPIRED   0x00800000

#define LSA_ACB_DISABLED                 0x00000001 /* 1 User account disabled */
#define LSA_ACB_HOMDIRREQ                0x00000002 /* 1 Home directory required */
#define LSA_ACB_PWNOTREQ                 0x00000004 /* 1 User password not required */
#define LSA_ACB_TEMPDUP                  0x00000008 /* 1 Temporary duplicate account */
#define LSA_ACB_NORMAL                   0x00000010 /* 1 Normal user account */
#define LSA_ACB_MNS                      0x00000020 /* 1 MNS logon user account */
#define LSA_ACB_DOMTRUST                 0x00000040 /* 1 Interdomain trust account */
#define LSA_ACB_WSTRUST                  0x00000080 /* 1 Workstation trust account */
#define LSA_ACB_SVRTRUST                 0x00000100 /* 1 Server trust account */
#define LSA_ACB_PWNOEXP                  0x00000200 /* 1 User password does not expire */
#define LSA_ACB_AUTOLOCK                 0x00000400 /* 1 Account auto locked */
#define LSA_ACB_ENC_TXT_PWD_ALLOWED      0x00000800 /* 1 Encryped text password is allowed */
#define LSA_ACB_SMARTCARD_REQUIRED       0x00001000 /* 1 Smart Card required */
#define LSA_ACB_TRUSTED_FOR_DELEGATION   0x00002000 /* 1 Trusted for Delegation */
#define LSA_ACB_NOT_DELEGATED            0x00004000 /* 1 Not delegated */
#define LSA_ACB_USE_DES_KEY_ONLY         0x00008000 /* 1 Use DES key only */
#define LSA_ACB_DONT_REQUIRE_PREAUTH     0x00010000 /* 1 Preauth not required */
#define LSA_ACB_PW_EXPIRED               0x00020000 /* 1 Password Expired */
#define LSA_ACB_NO_AUTH_DATA_REQD        0x00080000 /* 1 No authorization data required */

#endif //__LSALDAP_MARSHAL_USER_H__

