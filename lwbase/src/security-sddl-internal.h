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
 *        security-sddl-internal.h
 *
 * Abstract:
 *
 *        SDDL Security Internal Types
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#ifndef SECURITYSDDLINTERNAL_H_
#define SECURITYSDDLINTERNAL_H_


//
// Well-known Sid string constants
//
#define SID_SECURITY_NULL_RID "S-1-0-0" // A group with no members. This is often used when a SID value is not known
#define SID_SECURITY_WORLD_RID "S-1-1-0" // A group that includes all users
#define SID_SECURITY_LOCAL_RID "S-1-2-0" // Users who log on to terminals locally (physically) connected to the system
#define SID_SECURITY_LOCAL_LOGON_RID "S-1-2-1"
#define SID_SECURITY_CREATOR_OWNER_RID "S-1-3-0" // A security identifier to be replaced by the security identifier of the user who created a new object
#define SID_SECURITY_CREATOR_GROUP_RID "S-1-3-1" // A security identifier to be replaced by the primary-group SID of the user who created a new object


#define SID_SECURITY_DIALUP_RID "S-1-5-1" // Users who log on to terminals using a dial-up modem. This is a group identifier
#define SID_SECURITY_NETWORK_RID "S-1-5-2" // Users who log on across a network
#define SID_SECURITY_BATCH_RID "S-1-5-3" // Users who log on using a batch queue facility
#define SID_SECURITY_INTERACTIVE_RID "S-1-5-4" // Users who log on for interactive operation
#define SID_SECURITY_SERVICE_RID "S-1-5-6" // Accounts authorized to log on as a service
#define SID_SECURITY_ANONYMOUS_LOGON_RID "S-1-5-7" // Anonymous logon, or null session logon
#define SID_SECURITY_PROXY_RID "S-1-5-8" // Proxy
#define SID_SECURITY_ENTERPRISE_CONTROLLERS_RID "S-1-5-9" // Enterprise controllers
#define SID_SECURITY_PRINCIPAL_SELF_RID "S-1-5-10" // The PRINCIPAL_SELF security identifier can be used in the ACL of a user or group object. During an access check, the system replaces the SID with the SID of the object. The PRINCIPAL_SELF SID is useful for specifying an inheritable ACE that applies to the user or group object that inherits the ACE
#define SID_SECURITY_AUTHENTICATED_USER_RID "S-1-5-11" // The authenticated users
#define SID_SECURITY_RESTRICTED_CODE_RID "S-1-5-12" // Restricted code
#define SID_SECURITY_TERMINAL_SERVER_RID "S-1-5-13" // Terminal Services
#define SID_SECURITY_LOCAL_SYSTEM_RID "S-1-5-18" // A special account used by the operating system
#define SID_SECURITY_NT_NON_UNIQUE "S-1-5-21" // SIDS are not unique
#define SID_SECURITY_BUILTIN_DOMAIN_RID "S-1-5-32" // The built-in system domain.


#define SDDL_ACCESS_LENGTH 57 // 28 (access masks) *2 + 1(\0)
#define SDDL_ACEFLAG_LENGTH 15 // 7 (ace flags) *2 +1 (\0)
#define SDDL_CONTROL_LENGTH  5  // "P" "AR" "AI"

#ifndef _DCE_IDL_
#include <lw/attrs.h>

LW_PCSTR
RtlpRidToAliasSidString(
    LW_IN ULONG rid
    );

ULONG
RtlpAliasSidStringToRid(
    LW_IN PCSTR pszSidString
    );

LW_PCSTR
RtlpAliasSidStringToSidString(
    LW_IN PCSTR pszAliasSidString
    );

LW_END_EXTERN_C

#endif /* _DCE_IDL_ */

#endif /* SECURITYSDDLINTERNAL_H_ */
