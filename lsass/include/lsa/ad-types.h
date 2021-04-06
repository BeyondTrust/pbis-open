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
 *     ad-types.h
 *
 * Abstract:
 *
 *     BeyondTrust Security and Authentication Subsystem (LSASS)
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
#define LSA_NET_JOIN_DOMAIN_NO_PRE_COMPUTER_ACCOUNT      (0x00000200)

// The following settings ARE implemented
#define LSA_NET_LEAVE_DOMAIN_ACCT_DELETE       (0x00000001)
#define LSA_NET_LEAVE_DOMAIN_LICENSE_RELEASE   (0x00000200)

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
