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
// SDDL Component tags
//
#define SDDL_OWNER                          "O"       // Owner tag
#define SDDL_GROUP                          "G"       // Group tag
#define SDDL_DACL                           "D"       // DACL tag
#define SDDL_SACL                           "S"       // SACL tag

//
// SDDL Security descriptor controls
// dacl_flags and sacl_flags
//
#define SDDL_PROTECTED                      "P"       // DACL or SACL Protected
#define SDDL_AUTO_INHERIT_REQ               "AR"      // Auto inherit request
#define SDDL_AUTO_INHERITED                 "AI"      // DACL/SACL are auto inherited

#define SDDL_CONTROL_SIZE                    2

//
// SDDL Ace types
//
#define SDDL_ACCESS_ALLOWED                 "A"       // Access allowed
#define SDDL_ACCESS_DENIED                  "D"       // Access denied
#define SDDL_OBJECT_ACCESS_ALLOWED          "OA"      // Object access allowed
#define SDDL_OBJECT_ACCESS_DENIED           "OD"      // Object access denied
#define SDDL_AUDIT                          "AU"      // Audit
#define SDDL_ALARM                          "AL"      // Alarm
#define SDDL_OBJECT_AUDIT                   "OU"      // Object audit
#define SDDL_OBJECT_ALARM                   "OL"      // Object alarm

//
// SDDL Ace flags
//
#define SDDL_CONTAINER_INHERIT              "CI"      // Container inherit
#define SDDL_OBJECT_INHERIT                 "OI"      // Object inherit
#define SDDL_NO_PROPAGATE                   "NP"      // Inherit no propagate
#define SDDL_INHERIT_ONLY                   "IO"      // Inherit only
#define SDDL_INHERITED                      "ID"      // Inherited
#define SDDL_AUDIT_SUCCESS                  "SA"      // Audit success
#define SDDL_AUDIT_FAILURE                  "FA"      // Audit failure

#define SDDL_ACEFLAG_SIZE                    2


//
// SDDL Rights
//
#define SDDL_READ_PROPERTY                  "RP"
#define SDDL_WRITE_PROPERTY                 "WP"
#define SDDL_CREATE_CHILD                   "CC"
#define SDDL_DELETE_CHILD                   "DC"
#define SDDL_LIST_CHILDREN                  "LC"
#define SDDL_SELF_WRITE                     "SW"
#define SDDL_LIST_OBJECT                    "LO"
#define SDDL_DELETE_TREE                    "DT"
#define SDDL_CONTROL_ACCESS                 "CR"
#define SDDL_READ_CONTROL                   "RC"
#define SDDL_WRITE_DAC                      "WD"
#define SDDL_WRITE_OWNER                    "WO"
#define SDDL_STANDARD_DELETE                "SD"
#define SDDL_GENERIC_ALL                    "GA"
#define SDDL_GENERIC_READ                   "GR"
#define SDDL_GENERIC_WRITE                  "GW"
#define SDDL_GENERIC_EXECUTE                "GX"
#define SDDL_FILE_ALL                       "FA"
#define SDDL_FILE_READ                      "FR"
#define SDDL_FILE_WRITE                     "FW"
#define SDDL_FILE_EXECUTE                   "FX"
#define SDDL_KEY_ALL                        "KA"
#define SDDL_KEY_READ                       "KR"
#define SDDL_KEY_WRITE                      "KW"
#define SDDL_KEY_EXECUTE                    "KX"

#define SDDL_RIGHT_SIZE                      2

//
// SDDL User alias max size
//      - currently, upto two supported eg. "DA"
//      - modify this if more WCHARs need to be there in future e.g. "DAX"
//

#define SDDL_ALIAS_SIZE                     2

//
// SDDL User aliases
//
#define SDDL_DOMAIN_ADMINISTRATORS          "DA"      // Domain admins
#define SDDL_DOMAIN_GUESTS                  "DG"      // Domain guests
#define SDDL_DOMAIN_USERS                   "DU"      // Domain users
#define SDDL_ENTERPRISE_DOMAIN_CONTROLLERS  "ED"      // Enterprise domain controllers
#define SDDL_DOMAIN_DOMAIN_CONTROLLERS      "DD"      // Domain domain controllers
#define SDDL_DOMAIN_COMPUTERS               "DC"      // Domain computers
#define SDDL_BUILTIN_ADMINISTRATORS         "BA"      // Builtin (local ) administrators
#define SDDL_BUILTIN_GUESTS                 "BG"      // Builtin (local ) guests
#define SDDL_BUILTIN_USERS                  "BU"      // Builtin (local ) users
#define SDDL_LOCAL_ADMIN                    "LA"      // Local administrator account
#define SDDL_LOCAL_GUEST                    "LG"      // Local group account
#define SDDL_ACCOUNT_OPERATORS              "AO"      // Account operators
#define SDDL_BACKUP_OPERATORS               "BO"      // Backup operators
#define SDDL_PRINTER_OPERATORS              "PO"      // Printer operators
#define SDDL_SERVER_OPERATORS               "SO"      // Server operators
#define SDDL_AUTHENTICATED_USERS            "AU"      // Authenticated users
#define SDDL_PERSONAL_SELF                  "PS"      // Personal self
#define SDDL_CREATOR_OWNER                  "CO"      // Creator owner
#define SDDL_CREATOR_GROUP                  "CG"      // Creator group
#define SDDL_LOCAL_SYSTEM                   "SY"      // Local system
#define SDDL_POWER_USERS                    "PU"      // Power users
#define SDDL_EVERYONE                       "WD"      // Everyone ( World )
#define SDDL_REPLICATOR                     "RE"      // Replicator
#define SDDL_INTERACTIVE                    "IU"      // Interactive logon user
#define SDDL_NETWORK                        "NU"      // Nework logon user
#define SDDL_SERVICE                        "SU"      // Service logon user
#define SDDL_RESTRICTED_CODE                "RC"      // Restricted code
#define SDDL_ANONYMOUS                      "AN"      // Anonymous Logon
#define SDDL_SCHEMA_ADMINISTRATORS          "SA"      // Schema Administrators
#define SDDL_CERT_SERV_ADMINISTRATORS       "CA"      // Certificate Server Administrators
#define SDDL_RAS_SERVERS                    "RS"      // RAS servers group
#define SDDL_ENTERPRISE_ADMINS              "EA"      // Enterprise administrators
#define SDDL_GROUP_POLICY_ADMINS            "PA"      // Group Policy administrators
#define SDDL_ALIAS_PREW2KCOMPACC            "RU"      // alias to allow previous windows 2000
#define SDDL_LOCAL_SERVICE                  "LS"      // Local service account (for services)
#define SDDL_NETWORK_SERVICE                "NS"      // Network service account (for services)
#define SDDL_REMOTE_DESKTOP                 "RD"      // Remote desktop users (for terminal server)
#define SDDL_NETWORK_CONFIGURATION_OPS      "NO"      // Network configuration operators ( to manage configuration of networking features)
#define SDDL_PERFMON_USERS                  "MU"      // Performance Monitor Users
#define SDDL_PERFLOG_USERS                  "LU"      // Performance Log Users


//
// SDDL SID strings for the integrity levels
//
#define SDDL_ML_LOW                          "LW"     // Low integrity level
#define SDDL_ML_MEDIUM                       "ME"     // Medium integrity level
#define SDDL_ML_HIGH                         "HI"     // High integrity level
#define SDDL_ML_SYSTEM                       "SI"     // System integrity level

//
// SDDL strings for the mandatory label policy flags that are in the access mask
//
#define SDDL_NO_WRITE_UP                     "NW"
#define SDDL_NO_READ_UP                      "NR"
#define SDDL_NO_EXECUTE_UP                   "NX"

//
// SDDL Seperators - character version
//
#define SDDL_SEPERATOR_C                     ';'
#define SDDL_DELIMINATOR_C                   ':'
#define SDDL_ACE_BEGIN_C                    '('
#define SDDL_ACE_END_C                       ')'
#define SDDL_SECTION_DELIMINATOR_C           '\n'

//
// SDDL Seperators - string version
//
#define SDDL_SEPERATOR_S                     ";"
#define SDDL_DELIMINATOR_S                   ":"
#define SDDL_ACE_BEGIN_S                     "("
#define SDDL_ACE_END_S                       ")"


#define SDDL_ACE_PART_NUM                     6 // ace_type;ace_flags;rights;guid;inherit_guid;account_sid



//
// Well-known Sid string constants
//
#define SECURITY_NULL_PREFIX "S-1-0-0" // A group with no members. This is often used when a SID value is not known
#define SECURITY_WORLD_PREFIX "S-1-1-0" // A group that includes all users
#define SECURITY_LOCAL_PREFIX "S-1-2-0" // Users who log on to terminals locally (physically) connected to the system
#define SECURITY_LOCAL_LOGON_PREFIX "S-1-2-1"
#define SECURITY_CREATOR_OWNER_PREFIX "S-1-3-0" // A security identifier to be replaced by the security identifier of the user who created a new object
#define SECURITY_CREATOR_GROUP_PREFIX "S-1-3-1" // A security identifier to be replaced by the primary-group SID of the user who created a new object


#define SECURITY_DIALUP_PREFIX "S-1-5-1" // Users who log on to terminals using a dial-up modem. This is a group identifier
#define SECURITY_NETWORK_PREFIX "S-1-5-2" // Users who log on across a network
#define SECURITY_BATCH_PREFIX "S-1-5-3" // Users who log on using a batch queue facility
#define SECURITY_INTERACTIVE_PREFIX "S-1-5-4" // Users who log on for interactive operation
#define SECURITY_SERVICE_PREFIX "S-1-5-6" // Accounts authorized to log on as a service
#define SECURITY_ANONYMOUS_LOGON_PREFIX "S-1-5-7" // Anonymous logon, or null session logon
#define SECURITY_PROXY_PREFIX "S-1-5-8" // Proxy
#define SECURITY_ENTERPRISE_CONTROLLERS_PREFIX "S-1-5-9" // Enterprise controllers
#define SECURITY_PRINCIPAL_SELF_PREFIX "S-1-5-10" // The PRINCIPAL_SELF security identifier can be used in the ACL of a user or group object. During an access check, the system replaces the SID with the SID of the object. The PRINCIPAL_SELF SID is useful for specifying an inheritable ACE that applies to the user or group object that inherits the ACE
#define SECURITY_AUTHENTICATED_USER_PREFIX "S-1-5-11" // The authenticated users
#define SECURITY_RESTRICTED_CODE_PREFIX "S-1-5-12" // Restricted code
#define SECURITY_TERMINAL_SERVER_PREFIX "S-1-5-13" // Terminal Services
#define SECURITY_LOCAL_SYSTEM_PREFIX "S-1-5-18" // A special account used by the operating system
#define SECURITY_NT_NON_PREFIX "S-1-5-21" // SIDS are not unique
#define SECURITY_BUILTIN_DOMAIN_PREFIX "S-1-5-32" // The built-in system domain.

#define SECURITY_BUILTIN_DOMAIN_PREFIX_HYPHEN "S-1-5-32-" // The built-in system domain.
#define SECURITY_XSTRING(val) SECURITY_STRING(val)
#define SECURITY_STRING(val) #val

#define SECURITY_BUILTIN_DOMAIN_ADMIN  SECURITY_BUILTIN_DOMAIN_PREFIX_HYPHEN \
                                       SECURITY_XSTRING(DOMAIN_ALIAS_RID_ADMINS) // "S-1-5-32-544"
#define SECURITY_BUILTIN_DOMAIN_GUESTS SECURITY_BUILTIN_DOMAIN_PREFIX_HYPHEN \
                                       SECURITY_XSTRING(DOMAIN_ALIAS_RID_GUESTS) // "S-1-5-32-546"
#define SECURITY_BUILTIN_DOMAIN_USERS  SECURITY_BUILTIN_DOMAIN_PREFIX_HYPHEN \
                                       SECURITY_XSTRING(DOMAIN_ALIAS_RID_USERS)  // "S-1-5-32-545"


#define SDDL_ACCESS_LENGTH 57 // 28 (access masks) *2 + 1(\0)
#define SDDL_ACEFLAG_LENGTH 15 // 7 (ace flags) *2 +1 (\0)
#define SDDL_CONTROL_LENGTH  5  // "P" "AR" "AI"

#define ACCESS_UNKNOWN_ACE_TYPE                 20 // Unknown/unsupported ACE type

#include <lw/types.h>
#include <lw/attrs.h>


LW_PCSTR
RtlpRidToSddl(
    LW_IN ULONG rid
    );

ULONG
RtlpSddlToRid(
    LW_IN PCSTR pszSidString
    );

LW_PCSTR
RtlpSddlToSidString(
    LW_IN PCSTR pszAliasSidString
    );

LW_END_EXTERN_C


#endif /* SECURITYSDDLINTERNAL_H_ */
