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
 * Module Name:
 *
 *        sddl.h
 *
 * Abstract:
 *
 *        SDDL Types
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#ifndef __LWBASE_SDDL_H__
#define __LWBASE_SDDL_H__

#ifdef _DCE_IDL_

cpp_quote("#include <lw/security-types.h>")
cpp_quote("#if 0")

#endif

#include <lw/types.h>
#include <lw/attrs.h>

//
// SDDL Version information
//
#define SDDL_REVISION_1     1
#define SDDL_REVISION       SDDL_REVISION_1

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
#define SDDL_SECTION_DELIMINATOR_S           "\n"


#define SDDL_ACE_PART_NUM                     6 // ace_type;ace_flags;rights;guid;inherit_guid;account_sid


#endif  // endif __SDDL_H__


