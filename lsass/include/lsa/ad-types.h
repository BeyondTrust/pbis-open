/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *     ad-types.h
 *
 * Abstract:
 *
 *     Likewise Security and Authentication Subsystem (LSASS)
 *
 *     Types used by AD provider APIs
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LSA_AD_TYPES_H__
#define __LSA_AD_TYPES_H__

/**
 * @ingroup ad
 */

/*@{*/

/**
 * @brief Domain join flags
 *
 * Encodes additional options when joining a domain
 */
typedef DWORD LSA_NET_JOIN_FLAGS;
typedef DWORD LSA_USER_ACCOUNT_CONTROL_FLAGS;
typedef DWORD *PLSA_NET_JOIN_FLAGS;

/**
 * @brief Do not synchronize time with DC
 *
 * Ordinarily, the AD provider first synchronizes the time
 * with the domain controller as this is necessary for Kerberos
 * to function.  This flag disables this behavior.
 * @hideinitializer
 */
#define LSA_NET_JOIN_DOMAIN_NOTIMESYNC         (0x00000001)
/**
 * @brief Join multiple domains
 *
 * Attempting to join another domain while already joined
 * usually leaves the current domain.  If this flags is specified,
 * both will be joined simultaneously.
 * @hideinitializer
 */
#define LSA_NET_JOIN_DOMAIN_MULTIPLE           (0x00000002)
// The following settings are not implemented
#define LSA_NET_JOIN_DOMAIN_ACCT_CREATE        (0x00000004)
#define LSA_NET_JOIN_DOMAIN_JOIN_IF_JOINED     (0x00000008)
#define LSA_NET_JOIN_DOMAIN_JOIN_UNSECURE      (0x00000010)
#define LSA_NET_JOIN_DOMAIN_MACHINE_PWD_PASSED (0x00000020)
#define LSA_NET_JOIN_DOMAIN_DEFER_SPN_SET      (0x00000040)
#define LSA_NET_JOIN_DOMAIN_JOIN_WITH_NEW_NAME (0x00000080)
#define LSA_NET_JOIN_DOMAIN_JOIN_READONLY      (0x00000100)

// The following setting is not implemented
#define LSA_NET_LEAVE_DOMAIN_ACCT_DELETE       (0x00000001)

/* LDAP account flags - local copies of UF_* flags
   from NetAPI */
// User Account Control Flags
#define LSAJOIN_SCRIPT                          (0x00000001)
#define LSAJOIN_ACCOUNTDISABLE                  (0x00000002)
#define LSAJOIN_HOMEDIR_REQUIRED                (0x00000008)
#define LSAJOIN_LOCKOUT                         (0x00000010)
#define LSAJOIN_PASSWD_NOTREQD                  (0x00000020)
#define LSAJOIN_PASSWD_CANT_CHANGE              (0x00000040)
#define LSAJOIN_ENCRYPTED_TEXT_PASSWORD_ALLOWED (0x00000080)
#define LSAJOIN_TEMP_DUPLICATE_ACCOUNT          (0x00000100)
#define LSAJOIN_NORMAL_ACCOUNT                  (0x00000200)
#define LSAJOIN_INTERDOMAIN_TRUST_ACCOUNT       (0x00000800)
#define LSAJOIN_WORKSTATION_TRUST_ACCOUNT       (0x00001000)
#define LSAJOIN_SERVER_TRUST_ACCOUNT            (0x00002000)
// N/A                                          (0x00004000)
// N/A                                          (0x00008000)
#define LSAJOIN_DONT_EXPIRE_PASSWD              (0x00010000)
#define LSAJOIN_MNS_LOGON_ACCOUNT               (0x00020000)
#define LSAJOIN_SMARTCARD_REQUIRED              (0x00040000)
#define LSAJOIN_TRUSTED_FOR_DELEGATION          (0x00080000)
#define LSAJOIN_NOT_DELEGATED                   (0x00100000)
#define LSAJOIN_USE_DES_KEY_ONLY                (0x00200000)
#define LSAJOIN_DONT_REQUIRE_PREAUTH            (0x00400000)
#define LSAJOIN_PASSWORD_EXPIRED                (0x00800000)
#define LSAJOIN_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION (0x01000000) 
/*@}*/

#endif /* __LSA_AD_TYPES_H__ */
