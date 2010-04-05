/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        error.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *
 *        Error Code Mapping API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static struct
{
    DWORD dwError;
    PCSTR pszMessage;
} gLwErrorMap[] =
{
    {
        LW_ERROR_SUCCESS,
        "No error"
    },
    {
        DNS_ERROR_BAD_PACKET,
        "A bad packet was received from a DNS server. Potentially the requested address does not exist."
    },
    {
        ERROR_ACCESS_DENIED,
        "Access is denied"
    },
    {
        ERROR_PASSWORD_RESTRICTION,
        "Password does not meet requirements"
    },
    {   LW_ERROR_ACCESS_DENIED,
        "Incorrect access attempt"
    },
    {
        ERROR_MEMBER_IN_ALIAS,
	"User is already in local group"
    },
    {
        ERROR_MEMBER_NOT_IN_ALIAS,
	"User is not a member of local group"
    },
    {
        ERROR_NO_SUCH_ALIAS,
	"No such local group"
    },
    {
        ERROR_ALIAS_EXISTS,
	"Local group exists"
    },
    {
        ERROR_MEMBERS_PRIMARY_GROUP,
	"Group is a primary group of one or more users"
    },
    {
        LW_ERROR_INVALID_CACHE_PATH,
        "An invalid cache path was specified"
    },
    {
        LW_ERROR_INVALID_CONFIG_PATH,
        "The path to the configuration file is invalid"
    },
    {
        LW_ERROR_INVALID_PREFIX_PATH,
        "The product installation folder could not be determined"
    },
    {
        LW_ERROR_INSUFFICIENT_BUFFER,
        "The provided buffer is insufficient"
    },
    {
        LW_ERROR_OUT_OF_MEMORY,
        "Out of memory"
    },
    {
        LW_ERROR_INVALID_MESSAGE,
        "The Inter Process message is invalid"
    },
    {
        LW_ERROR_UNEXPECTED_MESSAGE,
        "An unexpected Inter Process message was received"
    },
    {
        LW_ERROR_NO_SUCH_USER,
        "No such user"
    },
    {
        LW_ERROR_DATA_ERROR,
        "The cached data is incorrect"
    },
    {
        LW_ERROR_NOT_IMPLEMENTED,
        "The requested feature has not been implemented yet"
    },
    {
        LW_ERROR_NO_CONTEXT_ITEM,
        "The requested item was not found in context"
    },
    {
        LW_ERROR_NO_SUCH_GROUP,
        "No such group"
    },
    {
        LW_ERROR_REGEX_COMPILE_FAILED,
        "Failed to compile regular expression"
    },
    {
        LW_ERROR_NSS_EDIT_FAILED,
        "Failed to edit nsswitch configuration"
    },
    {
        LW_ERROR_NO_HANDLER,
        "No authentication Provider was found"
    },
    {
        LW_ERROR_INTERNAL,
        "Internal error"
    },
    {
        LW_ERROR_NOT_HANDLED,
        "The authentication request could not be handled"
    },
    {
        LW_ERROR_INVALID_DNS_RESPONSE,
        "Response from DNS is invalid"
    },
    {
        LW_ERROR_DNS_RESOLUTION_FAILED,
        "Failed to resolve query using DNS"
    },
    {
        LW_ERROR_FAILED_TIME_CONVERSION,
        "Failed to convert the time"
    },
    {
        LW_ERROR_INVALID_SID,
        "The security descriptor (SID) is invalid"
    },
    {
        LW_ERROR_PASSWORD_MISMATCH,
        "The password is incorrect for the given username"
    },
    {
        LW_ERROR_UNEXPECTED_DB_RESULT,
        "Unexpected cached data found"
    },
    {
        LW_ERROR_PASSWORD_EXPIRED,
        "Password expired"
    },
    {
        LW_ERROR_ACCOUNT_EXPIRED,
        "Account expired"
    },
    {
        LW_ERROR_USER_EXISTS,
        "A user by this name already exists"
    },
    {
        LW_ERROR_GROUP_EXISTS,
        "A group by this name already exists"
    },
    {
        LW_ERROR_INVALID_GROUP_INFO_LEVEL,
        "An invalid group info level was specified"
    },
    {
        LW_ERROR_INVALID_USER_INFO_LEVEL,
        "An invalid user info level was specified"
    },
    {
        LW_ERROR_UNSUPPORTED_USER_LEVEL,
        "This interface does not support the user info level specified"
    },
    {
        LW_ERROR_UNSUPPORTED_GROUP_LEVEL,
        "This interface does not support the group info level specified"
    },
    {
        LW_ERROR_INVALID_LOGIN_ID,
        "The login id is invalid"
    },
    {
        LW_ERROR_INVALID_HOMEDIR,
        "The home directory is invalid"
    },
    {
        LW_ERROR_INVALID_GROUP_NAME,
        "The group name is invalid"
    },
    {
        LW_ERROR_NO_MORE_GROUPS,
        "No more groups"
    },
    {
        LW_ERROR_NO_MORE_USERS,
        "No more users"
    },
    {
        LW_ERROR_FAILED_ADD_USER,
        "Failed to add user"
    },
    {
        LW_ERROR_FAILED_ADD_GROUP,
        "Failed to add group"
    },
    {
        LW_ERROR_INVALID_LSA_CONNECTION,
        "The connection to the authentication service is invalid"
    },
    {
        LW_ERROR_INVALID_AUTH_PROVIDER,
        "The authentication provider is invalid"
    },
    {
        LW_ERROR_INVALID_PARAMETER,
        "Invalid parameter"
    },
    {
        LW_ERROR_LDAP_NO_PARENT_DN,
        "No distinguished name found in LDAP for parent of this object"
    },
    {
        LW_ERROR_LDAP_ERROR,
        "An Error was encountered when negotiating with LDAP"
    },
    {
        LW_ERROR_NO_SUCH_DOMAIN,
        "Unknown Active Directory domain"
    },
    {
        LW_ERROR_LDAP_FAILED_GETDN,
        "Failed to find distinguished name using LDAP"
    },
    {
        LW_ERROR_DUPLICATE_DOMAINNAME,
        "A duplicate Active Directory domain name was found"
    },
    {
        LW_ERROR_KRB5_CALL_FAILED,
        "The call to Kerberos 5 failed"
    },
    {
        LW_ERROR_GSS_CALL_FAILED,
        "The GSS call failed"
    },
    {
        LW_ERROR_FAILED_FIND_DC,
        "Failed to find the domain controller"
    },
    {
        LW_ERROR_NO_SUCH_CELL,
        "Failed to find the Cell in Active Direcotry"
    },
    {
        LW_ERROR_GROUP_IN_USE,
        "The specified group is currently being used"
    },
    {
        LW_ERROR_FAILED_CREATE_HOMEDIR,
        "Failed to create home directory for user"
    },
    {
        LW_ERROR_PASSWORD_TOO_WEAK,
        "The specified password is not strong enough"
    },
    {
        LW_ERROR_INVALID_SID_REVISION,
        "The security descriptor (SID) has an invalid revision"
    },
    {
        LW_ERROR_ACCOUNT_LOCKED,
        "The user account is locked"
    },
    {
        LW_ERROR_ACCOUNT_DISABLED,
        "The user account is disabled"
    },
    {
        LW_ERROR_USER_CANNOT_CHANGE_PASSWD,
        "The user is not allowed to change his/her password"
    },
    {
        LW_ERROR_LOAD_LIBRARY_FAILED,
        "Failed to dynamically load a library"
    },
    {
        LW_ERROR_LOOKUP_SYMBOL_FAILED,
        "Failed to lookup a symbol in a dynamic library"
    },
    {
        LW_ERROR_INVALID_EVENTLOG,
        "The Eventlog interface is invalid"
    },
    {
        LW_ERROR_INVALID_CONFIG,
        "The specified configuration (file) is invalid"
    },
    {
        LW_ERROR_UNEXPECTED_TOKEN,
        "An unexpected token was encountered in the configuration"
    },
    {
        LW_ERROR_LDAP_NO_RECORDS_FOUND,
        "No records were found in the cache"
    },
    {
        LW_ERROR_DUPLICATE_USERNAME,
        "A duplicate user record was found"
    },
    {
        LW_ERROR_DUPLICATE_GROUPNAME,
        "A duplicate group record was found"
    },
    {
        LW_ERROR_DUPLICATE_CELLNAME,
        "A duplicate cell was found"
    },
    {
        LW_ERROR_STRING_CONV_FAILED,
        "Failed to convert string format (wide/ansi)"
    },
    {
        LW_ERROR_INVALID_ACCOUNT,
        "The user account is invalid"
    },
    {
        LW_ERROR_INVALID_PASSWORD,
        "The password is invalid"
    },
    {
        LW_ERROR_QUERY_CREATION_FAILED,
        "Failed to create query to examine cache"
    },
    {
        LW_ERROR_NO_SUCH_OBJECT,
        "No such user, group or domain object"
    },
    {
        LW_ERROR_DUPLICATE_USER_OR_GROUP,
        "A duplicate user or group was found"
    },
    {
        LW_ERROR_INVALID_KRB5_CACHE_TYPE,
        "An invalid kerberos 5 cache type was specified"
    },
    {
        LW_ERROR_NOT_JOINED_TO_AD,
        "This machine is not currently joined to Active Directory"
    },
    {
        LW_ERROR_FAILED_TO_SET_TIME,
        "The system time could not be set"
    },
    {
        LW_ERROR_NO_NETBIOS_NAME,
        "Failed to find NetBIOS name for the domain"
    },
    {
        LW_ERROR_INVALID_NETLOGON_RESPONSE,
        "The Netlogon response buffer is invalid"
    },
    {
        LW_ERROR_INVALID_OBJECTGUID,
        "The specified Globally Unique Identifier (GUID) is invalid"
    },
    {
        LW_ERROR_INVALID_DOMAIN,
        "The domain name is invalid"
    },
    {
        LW_ERROR_NO_DEFAULT_REALM,
        "The kerberos default realm is not set"
    },
    {
        LW_ERROR_NOT_SUPPORTED,
        "The request is not supported"
    },
    {
        LW_ERROR_LOGON_FAILURE,
        "The logon request failed"
    },
    {
        LW_ERROR_NO_SITE_INFORMATION,
        "No site information was found for the active directory domain"
    },
    {
        LW_ERROR_INVALID_LDAP_STRING,
        "The LDAP string value is invalid"
    },
    {
        LW_ERROR_INVALID_LDAP_ATTR_VALUE,
        "The LDAP attribute value is NULL or invalid"
    },
    {
        LW_ERROR_NULL_BUFFER,
        "An invalid buffer was provided"
    },
    {
        LW_ERROR_CLOCK_SKEW,
        "Clock skew detected with active directory server"
    },
    {
        LW_ERROR_KRB5_NO_KEYS_FOUND,
        "No kerberos keys found"
    },
    {
        LW_ERROR_SERVICE_NOT_AVAILABLE,
        "The authentication service is not available"
    },
    {
        LW_ERROR_INVALID_SERVICE_RESPONSE,
        "Invalid response from the authentication service"
    },
    {
        LW_ERROR_NSS_ERROR,
        "Name Service Switch Error"
    },
    {
        LW_ERROR_AUTH_ERROR,
        "Authentication Error"
    },
    {
        LW_ERROR_INVALID_LDAP_DN,
        "Invalid Ldap distinguished name (DN)"
    },
    {
        LW_ERROR_NOT_MAPPED,
        "Account Name or SID not mapped"
    },
    {
        LW_ERROR_RPC_NETLOGON_FAILED,
        "Calling librpc/NetLogon API failed"
    },
    {
        LW_ERROR_ENUM_DOMAIN_TRUSTS_FAILED,
        "Enumerating domain trusts failed"
    },
    {
        LW_ERROR_RPC_LSABINDING_FAILED,
        "Calling librpc/Lsa Binding failed"
    },
    {
        LW_ERROR_RPC_OPENPOLICY_FAILED,
        "Calling librpc/Lsa Open Policy failed"
    },
    {
        LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED,
        "Calling librpc/Lsa LookupName2 failed"
    },
    {
        LW_ERROR_RPC_SET_SESS_CREDS_FAILED,
        "RPC Set Session Creds failed"
    },
    {
        LW_ERROR_RPC_REL_SESS_CREDS_FAILED,
        "RPC Release Session Creds failed"
    },
    {
        LW_ERROR_RPC_CLOSEPOLICY_FAILED,
        "Calling librpc/Lsa Close Handle/policy failed"
    },
    {
        LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND,
        "No user/group was found using RPC lookup name to objectSid"
    },
    {
        LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES,
        "Abnormal duplicates were found using RPC lookup name to objectSid"
    },
    {
        LW_ERROR_NO_TRUSTED_DOMAIN_FOUND,
        "No such trusted domain found"
    },
    {
        LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS,
        "Trusted domain is executing in an incompatiable modes"
    },
    {
        LW_ERROR_DCE_CALL_FAILED,
        "A call to DCE/RPC failed"
    },
    {
        LW_ERROR_FAILED_TO_LOOKUP_DC,
        "Failed to lookup the domain controller for given domain"
    },
    {
        LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL,
        "An invalid nss artefact info level was specified"
    },
    {
        LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL,
        "The NSS artefact info level specified is not supported"
    },
    {
        LW_ERROR_INVALID_USER_NAME,
        "An invalid user name was specified"
    },
    {
        LW_ERROR_INVALID_LOG_LEVEL,
        "An invalid log level was specified"
    },
    {
        LW_ERROR_INVALID_METRIC_TYPE,
        "An invalid metric type was specified"
    },
    {
        LW_ERROR_INVALID_METRIC_PACK,
        "An invalid metric pack was specified"
    },
    {
        LW_ERROR_INVALID_METRIC_INFO_LEVEL,
        "An invalid info level was specified when querying metrics"
    },
    {
        LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK,
        "The system hostname configuration is incorrect"
    },
    {
        LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED,
        "Could not find DirectoryService cache utility to call -flushcache with"
    },
    {
        LW_ERROR_LSA_SERVER_UNREACHABLE,
        "The LSASS server is not responding."
    },
    {
        LW_ERROR_INVALID_NSS_ARTEFACT_TYPE,
        "An invalid NSS Artefact type was specified"
    },
    {
        LW_ERROR_INVALID_AGENT_VERSION,
        "The LSASS Server version is invalid"
    },
    {
        LW_ERROR_DOMAIN_IS_OFFLINE,
        "The domain is offline"
    },
    {
        LW_ERROR_INVALID_HOMEDIR_TEMPLATE,
        "An invalid home directory template was specified"
    },
    {
        LW_ERROR_RPC_PARSE_SID_STRING,
        "Failed to use NetAPI to parse SID string"
    },
    {
        LW_ERROR_RPC_LSA_LOOKUPSIDS_FAILED,
        "Failed to use NetAPI to lookup names for SIDs"
    },
    {
        LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND,
        "Failed to find any names for SIDs using NetAPI"
    },
    {
        LW_ERROR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES,
        "Found duplicates using RPC Call to lookup SIDs"
    },
    {
        LW_ERROR_PASSWORD_RESTRICTION,
        "Password does not meet requirements"
    },
    {
        LW_ERROR_OBJECT_NOT_ENABLED,
        "The user/group is not enabled in the cell"
    },
    {
        LW_ERROR_NO_MORE_NSS_ARTEFACTS,
        "No more NSS Artefacts"
    },
    {
        LW_ERROR_INVALID_NSS_MAP_NAME,
        "An invalid name was specified for the NIS map"
    },
    {
        LW_ERROR_INVALID_NSS_KEY_NAME,
        "An invalid name was specified for the NIS key"
    },
    {
        LW_ERROR_NO_SUCH_NSS_KEY,
        "No such NIS Key is defined"
    },
    {
        LW_ERROR_NO_SUCH_NSS_MAP,
        "No such NIS Map is defined"
    },
    {
        LW_ERROR_RPC_ERROR,
        "An Error was encountered when negotiating with RPC"
    },
    {
        LW_ERROR_LDAP_SERVER_UNAVAILABLE,
        "The LDAP server is unavailable"
    },
    {
        LW_ERROR_CREATE_KEY_FAILED,
        "Could not create random key"
    },
    {
        LW_ERROR_CANNOT_DETECT_USER_PROCESSES,
        "Cannot detect user processes"
    },
    {
        LW_ERROR_TRACE_NOT_INITIALIZED,
        "Tracing has not been initialized"
    },
    {
        LW_ERROR_NO_SUCH_TRACE_FLAG,
        "The trace flag is not defined"
    },
    {
        LW_ERROR_DCERPC_ERROR,
        "DCE-RPC Error"
    },
    {
        LW_ERROR_INVALID_RPC_SERVER,
        "Invalid RPC Server"
    },
    {
        LW_ERROR_RPC_SERVER_REGISTRATION_ERROR,
        "RPC Server Registration error"
    },
    {
        LW_ERROR_RPC_SERVER_RUNTIME_ERROR,
        "RPC Server runtime error"
    },
    {
        LW_ERROR_DOMAIN_IN_USE,
        "The domain is in use"
    },
    {
        LW_ERROR_SAM_DATABASE_ERROR,
        "SAM database error"
    },
    {
        LW_ERROR_SAM_INIT_ERROR,
        "Error when initializing SAM subsystem"
    },
    {
        LW_ERROR_OBJECT_IN_USE,
        "The object is in use"
    },
    {
        LW_ERROR_NO_SUCH_ATTRIBUTE,
        "No such attribute was found"
    },
    {
        LW_ERROR_GET_DC_NAME_FAILED,
        "Get domain controller name failed"
    },
    {
        LW_ERROR_INVALID_ATTRIBUTE_VALUE,
        "An invalid attribute value was specified"
    },
    {
        LW_ERROR_NO_ATTRIBUTE_VALUE,
        "No attribute value was found"
    },
    {
        LW_ERROR_MEMBER_IN_LOCAL_GROUP,
        "Member is in local group"
    },
    {
        LW_ERROR_MEMBER_NOT_IN_LOCAL_GROUP,
        "No such member in local group"
    },
    {
        LW_ERROR_INVALID_SERVICE_TRANSITION,
        "The operation is invalid from the service's present state"
    },
    {
        LW_ERROR_SERVICE_DEPENDENCY_UNMET,
        "The service cannot be started because another service it depends on is not running"
    },
    {
        LW_ERROR_SERVICE_UNRESPONSIVE,
        "The service is not responding to requests"
    },
    {
        LW_ERROR_NO_SUCH_SERVICE,
        "No service with the specified name exists"
    },
    {
        LW_ERROR_DEPENDENT_SERVICE_STILL_RUNNING,
        "The service cannot be stopped because another service that depends on it is still running"
    }
};

size_t
LwGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t sErrIndex = 0;
    size_t sRequiredLen = 0;

    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }

    for (sErrIndex = 0; sErrIndex < sizeof(gLwErrorMap)/sizeof(gLwErrorMap[0]); sErrIndex++)
    {
        if (gLwErrorMap[sErrIndex].dwError == dwError)
        {
            sRequiredLen = strlen(gLwErrorMap[sErrIndex].pszMessage) + 1;
            if (stBufSize >= sRequiredLen)
            {
                strcpy(pszBuffer, gLwErrorMap[sErrIndex].pszMessage);
            }
            return sRequiredLen;
        }
    }

    sRequiredLen = strlen("Unknown error") + 1;
    if (stBufSize >= sRequiredLen)
    {
        strcpy(pszBuffer, "Unknown error");
    }
    return sRequiredLen;
}

DWORD
LwMapErrnoToLwError(
    DWORD dwErrno
    )
{
    switch(dwErrno)
    {
        case 0:
            return LW_ERROR_SUCCESS;
        case EPERM:
            return ERROR_ACCESS_DENIED;
        case ENOENT:
            return ERROR_FILE_NOT_FOUND;
        case ESRCH:
            return LW_ERROR_NO_SUCH_PROCESS;
        case EINTR:
            return LW_ERROR_INTERRUPTED;
        case EIO:
            return LW_ERROR_GENERIC_IO;
        case ENXIO:
            return LW_ERROR_ERRNO_ENXIO;
        case E2BIG:
            return LW_ERROR_ERRNO_E2BIG;
        case ENOEXEC:
            return LW_ERROR_ERRNO_ENOEXEC;
        case EBADF:
            return LW_ERROR_INVALID_HANDLE;
        case ECHILD:
            return LW_ERROR_ERRNO_ECHILD;
        case EAGAIN:
#if EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
            return LW_ERROR_ERRNO_EAGAIN;
        case ENOMEM:
            return LW_ERROR_OUT_OF_MEMORY;
        case EACCES:
            return LW_ERROR_ACCESS_DENIED;
        case EFAULT:
            return LW_ERROR_ERRNO_EFAULT;
        case ENOTBLK:
            return LW_ERROR_ERRNO_ENOTBLK;
        case EBUSY:
            return LW_ERROR_ERRNO_EBUSY;
        case EEXIST:
            return LW_ERROR_ERRNO_EEXIST;
        case EXDEV:
            return LW_ERROR_ERRNO_EXDEV;
        case ENODEV:
            return LW_ERROR_ERRNO_ENODEV;
        case ENOTDIR:
            return LW_ERROR_ERRNO_ENOTDIR;
        case EISDIR:
            return LW_ERROR_ERRNO_EISDIR;
        case EINVAL:
            return LW_ERROR_INVALID_PARAMETER;
        case ENFILE:
            return LW_ERROR_ERRNO_ENFILE;
        case EMFILE:
            return LW_ERROR_ERRNO_EMFILE;
        case ENOTTY:
            return LW_ERROR_ERRNO_ENOTTY;
        case ETXTBSY:
            return LW_ERROR_ERRNO_ETXTBSY;
        case EFBIG:
            return LW_ERROR_ERRNO_EFBIG;
        case ENOSPC:
            return LW_ERROR_ERRNO_ENOSPC;
        case ESPIPE:
            return LW_ERROR_ERRNO_ESPIPE;
        case EROFS:
            return LW_ERROR_ERRNO_EROFS;
        case EMLINK:
            return LW_ERROR_ERRNO_EMLINK;
        case EPIPE:
            return LW_ERROR_ERRNO_EPIPE;
        case EDOM:
            return LW_ERROR_ERRNO_EDOM;
        case ERANGE:
            return LW_ERROR_ERRNO_ERANGE;
#if defined(EDEADLOCK) && EDEADLOCK != EDEADLK
        case EDEADLOCK:
#endif
        case EDEADLK:
            return LW_ERROR_ERRNO_EDEADLOCK;
        case ENAMETOOLONG:
            return LW_ERROR_ERRNO_ENAMETOOLONG;
        case ENOLCK:
            return LW_ERROR_ERRNO_ENOLCK;
        case ENOSYS:
            return LW_ERROR_NOT_SUPPORTED;
#if ENOTEMPTY != EEXIST
        case ENOTEMPTY:
            return LW_ERROR_ERRNO_ENOTEMPTY;
#endif
        case ELOOP:
            return LW_ERROR_ERRNO_ELOOP;
        case ENOMSG:
            return LW_ERROR_ERRNO_ENOMSG;
        case EIDRM:
            return LW_ERROR_ERRNO_EIDRM;
#ifdef ECHRNG
        case ECHRNG:
            return LW_ERROR_ERRNO_ECHRNG;
#endif
#ifdef EL2NSYNC
        case EL2NSYNC:
            return LW_ERROR_ERRNO_EL2NSYNC;
#endif
#ifdef EL3HLT
        case EL3HLT:
            return LW_ERROR_ERRNO_EL3HLT;
#endif
#ifdef EL3RST
        case EL3RST:
            return LW_ERROR_ERRNO_EL3RST;
#endif
#ifdef ELNRNG
        case ELNRNG:
            return LW_ERROR_ERRNO_ELNRNG;
#endif
#ifdef EUNATCH
        case EUNATCH:
            return LW_ERROR_ERRNO_EUNATCH;
#endif
#ifdef ENOCSI
        case ENOCSI:
            return LW_ERROR_ERRNO_ENOCSI;
#endif
#ifdef EL2HLT
        case EL2HLT:
            return LW_ERROR_ERRNO_EL2HLT;
#endif
#ifdef EBADE
        case EBADE:
            return LW_ERROR_ERRNO_EBADE;
#endif
#ifdef EBADR
        case EBADR:
            return LW_ERROR_ERRNO_EBADR;
#endif
#ifdef EXFULL
        case EXFULL:
            return LW_ERROR_ERRNO_EXFULL;
#endif
#ifdef ENOANO
        case ENOANO:
            return LW_ERROR_ERRNO_ENOANO;
#endif
#ifdef EBADRQC
        case EBADRQC:
            return LW_ERROR_ERRNO_EBADRQC;
#endif
#ifdef EBADSLT
        case EBADSLT:
            return LW_ERROR_ERRNO_EBADSLT;
#endif
#ifdef EBFONT
        case EBFONT:
            return LW_ERROR_ERRNO_EBFONT;
#endif
#ifdef ENOSTR
        case ENOSTR:
            return LW_ERROR_ERRNO_ENOSTR;
#endif
#ifdef ENODATA
        case ENODATA:
            return LW_ERROR_ERRNO_ENODATA;
#endif
#ifdef ETIME
        case ETIME:
            return LW_ERROR_ERRNO_ETIME;
#endif
#ifdef ENOSR
        case ENOSR:
            return LW_ERROR_ERRNO_ENOSR;
#endif
#ifdef ENONET
        case ENONET:
            return LW_ERROR_ERRNO_ENONET;
#endif
#ifdef ENOPKG
        case ENOPKG:
            return LW_ERROR_ERRNO_ENOPKG;
#endif
        case EREMOTE:
            return LW_ERROR_ERRNO_EREMOTE;
        case ENOLINK:
            return LW_ERROR_ERRNO_ENOLINK;
#ifdef EADV
        case EADV:
            return LW_ERROR_ERRNO_EADV;
#endif
#ifdef ESRMNT
        case ESRMNT:
            return LW_ERROR_ERRNO_ESRMNT;
#endif
#ifdef ECOMM
        case ECOMM:
            return LW_ERROR_ERRNO_ECOMM;
#endif
        case EPROTO:
            return LW_ERROR_ERRNO_EPROTO;
        case EMULTIHOP:
            return LW_ERROR_ERRNO_EMULTIHOP;
#ifdef EDOTDOT
        case EDOTDOT:
            return LW_ERROR_ERRNO_EDOTDOT;
#endif
        case EBADMSG:
            return LW_ERROR_ERRNO_EBADMSG;
        case EOVERFLOW:
            return LW_ERROR_ERRNO_EOVERFLOW;
#ifdef ENOTUNIQ
        case ENOTUNIQ:
            return LW_ERROR_ERRNO_ENOTUNIQ;
#endif
#ifdef EBADFD
        case EBADFD:
            return LW_ERROR_ERRNO_EBADFD;
#endif
#ifdef EREMCHG
        case EREMCHG:
            return LW_ERROR_ERRNO_EREMCHG;
#endif
#ifdef ELIBACC
        case ELIBACC:
            return LW_ERROR_ERRNO_ELIBACC;
#endif
#ifdef ELIBBAD
        case ELIBBAD:
            return LW_ERROR_ERRNO_ELIBBAD;
#endif
#ifdef ELIBSCN
        case ELIBSCN:
            return LW_ERROR_ERRNO_ELIBSCN;
#endif
#ifdef ELIBMAX
        case ELIBMAX:
            return LW_ERROR_ERRNO_ELIBMAX;
#endif
#ifdef ELIBEXEC
        case ELIBEXEC:
            return LW_ERROR_ERRNO_ELIBEXEC;
#endif
        case EILSEQ:
            return LW_ERROR_ERRNO_EILSEQ;
#ifdef ERESTART
        case ERESTART:
            return LW_ERROR_ERRNO_ERESTART;
#endif
#ifdef ESTRPIPE
        case ESTRPIPE:
            return LW_ERROR_ERRNO_ESTRPIPE;
#endif
        case EUSERS:
            return LW_ERROR_ERRNO_EUSERS;
        case ENOTSOCK:
            return LW_ERROR_ERRNO_ENOTSOCK;
        case EDESTADDRREQ:
            return LW_ERROR_ERRNO_EDESTADDRREQ;
        case EMSGSIZE:
            return LW_ERROR_ERRNO_EMSGSIZE;
        case EPROTOTYPE:
            return LW_ERROR_ERRNO_EPROTOTYPE;
        case ENOPROTOOPT:
            return LW_ERROR_ERRNO_ENOPROTOOPT;
        case EPROTONOSUPPORT:
            return LW_ERROR_ERRNO_EPROTONOSUPPORT;
        case ESOCKTNOSUPPORT:
            return LW_ERROR_ERRNO_ESOCKTNOSUPPORT;
        case EOPNOTSUPP:
            return LW_ERROR_ERRNO_EOPNOTSUPP;
        case EPFNOSUPPORT:
            return LW_ERROR_ERRNO_EPFNOSUPPORT;
        case EAFNOSUPPORT:
            return LW_ERROR_ERRNO_EAFNOSUPPORT;
        case EADDRINUSE:
            return LW_ERROR_ERRNO_EADDRINUSE;
        case EADDRNOTAVAIL:
            return LW_ERROR_ERRNO_EADDRNOTAVAIL;
        case ENETDOWN:
            return LW_ERROR_ERRNO_ENETDOWN;
        case ENETUNREACH:
            return LW_ERROR_ERRNO_ENETUNREACH;
        case ENETRESET:
            return LW_ERROR_ERRNO_ENETRESET;
        case ECONNABORTED:
            return LW_ERROR_ERRNO_ECONNABORTED;
        case ECONNRESET:
            return LW_ERROR_ERRNO_ECONNRESET;
        case ENOBUFS:
            return LW_ERROR_ERRNO_ENOBUFS;
        case EISCONN:
            return LW_ERROR_ERRNO_EISCONN;
        case ENOTCONN:
            return LW_ERROR_ERRNO_ENOTCONN;
        case ESHUTDOWN:
            return LW_ERROR_ERRNO_ESHUTDOWN;
        case ETOOMANYREFS:
            return LW_ERROR_ERRNO_ETOOMANYREFS;
        case ETIMEDOUT:
            return LW_ERROR_ERRNO_ETIMEDOUT;
        case ECONNREFUSED:
            return LW_ERROR_ERRNO_ECONNREFUSED;
        case EHOSTDOWN:
            return LW_ERROR_ERRNO_EHOSTDOWN;
        case EHOSTUNREACH:
            return LW_ERROR_ERRNO_EHOSTUNREACH;
        case EALREADY:
            return LW_ERROR_ERRNO_EALREADY;
        case EINPROGRESS:
            return LW_ERROR_ERRNO_EINPROGRESS;
        case ESTALE:
            return LW_ERROR_ERRNO_ESTALE;
#ifdef EUCLEAN
        case EUCLEAN:
            return LW_ERROR_ERRNO_EUCLEAN;
#endif
#ifdef ENOTNAM
        case ENOTNAM:
            return LW_ERROR_ERRNO_ENOTNAM;
#endif
#ifdef ENAVAIL
        case ENAVAIL:
            return LW_ERROR_ERRNO_ENAVAIL;
#endif
#ifdef EISNAM
        case EISNAM:
            return LW_ERROR_ERRNO_EISNAM;
#endif
#ifdef EREMOTEIO
        case EREMOTEIO:
            return LW_ERROR_ERRNO_EREMOTEIO;
#endif
        case EDQUOT:
            return LW_ERROR_ERRNO_EDQUOT;
#ifdef ENOMEDIUM
        case ENOMEDIUM:
            return LW_ERROR_ERRNO_ENOMEDIUM;
#endif
#ifdef EMEDIUMTYPE
        case EMEDIUMTYPE:
            return LW_ERROR_ERRNO_EMEDIUMTYPE;
#endif
        case ECANCELED:
            return LW_ERROR_ERRNO_ECANCELED;
        default:
            LW_LOG_ERROR("Unable to map errno %d", dwErrno);
            return LW_ERROR_UNKNOWN;
    }
}

LW_DWORD
LwMapHErrnoToLwError(
    LW_IN LW_DWORD dwHErrno
    )
{
    switch(dwHErrno)
    {
        case 0:
            return LW_ERROR_SUCCESS;
        case HOST_NOT_FOUND:
            return WSAHOST_NOT_FOUND;
        case NO_DATA:
#if defined(NO_ADDRESS) && NO_DATA != NO_ADDRESS
        case NO_ADDRESS:
#endif
            return WSANO_DATA;
        case NO_RECOVERY:
            return WSANO_RECOVERY;
        case TRY_AGAIN:
            return WSATRY_AGAIN;
        default:
            LW_LOG_ERROR("Unable to map h_errno %d", dwHErrno);
            return LW_ERROR_UNKNOWN;
    }
}


DWORD
LwMapLdapErrorToLwError(
    DWORD dwErr
    )
{
    switch(dwErr)
    {
        case LDAP_SUCCESS:
            return LW_ERROR_SUCCESS;
        case LDAP_SERVER_DOWN:
            return LW_ERROR_LDAP_SERVER_DOWN;
        case LDAP_LOCAL_ERROR:
            return LW_ERROR_LDAP_LOCAL_ERROR;
        case LDAP_ENCODING_ERROR:
            return LW_ERROR_LDAP_ENCODING_ERROR;
        case LDAP_DECODING_ERROR:
            return LW_ERROR_LDAP_DECODING_ERROR;
        case LDAP_TIMEOUT:
            return LW_ERROR_LDAP_TIMEOUT;
        case LDAP_AUTH_UNKNOWN:
            return LW_ERROR_LDAP_AUTH_UNKNOWN;
        case LDAP_FILTER_ERROR:
            return LW_ERROR_LDAP_FILTER_ERROR;
        case LDAP_USER_CANCELLED:
            return LW_ERROR_LDAP_USER_CANCELLED;
        case LDAP_PARAM_ERROR:
            return LW_ERROR_LDAP_PARAM_ERROR;
        case LDAP_NO_MEMORY:
            return LW_ERROR_LDAP_NO_MEMORY;
        case LDAP_CONNECT_ERROR:
            return LW_ERROR_LDAP_CONNECT_ERROR;
        case LDAP_NOT_SUPPORTED:
            return LW_ERROR_LDAP_NOT_SUPPORTED;
        case LDAP_CONTROL_NOT_FOUND:
            return LW_ERROR_LDAP_CONTROL_NOT_FOUND;
        case LDAP_NO_RESULTS_RETURNED:
            return LW_ERROR_LDAP_NO_RESULTS_RETURNED;
        case LDAP_MORE_RESULTS_TO_RETURN:
            return LW_ERROR_LDAP_MORE_RESULTS_TO_RETURN;
        case LDAP_CLIENT_LOOP:
            return LW_ERROR_LDAP_CLIENT_LOOP;
        case LDAP_REFERRAL_LIMIT_EXCEEDED:
            return LW_ERROR_LDAP_REFERRAL_LIMIT_EXCEEDED;
        case LDAP_OPERATIONS_ERROR:
            return LW_ERROR_LDAP_OPERATIONS_ERROR;
        case LDAP_PROTOCOL_ERROR:
            return LW_ERROR_LDAP_PROTOCOL_ERROR;
        case LDAP_TIMELIMIT_EXCEEDED:
            return LW_ERROR_LDAP_TIMELIMIT_EXCEEDED;
        case LDAP_SIZELIMIT_EXCEEDED:
            return LW_ERROR_LDAP_SIZELIMIT_EXCEEDED;
        case LDAP_COMPARE_FALSE:
            return LW_ERROR_LDAP_COMPARE_FALSE;
        case LDAP_COMPARE_TRUE:
            return LW_ERROR_LDAP_COMPARE_TRUE;
        case LDAP_STRONG_AUTH_NOT_SUPPORTED:
            return LW_ERROR_LDAP_STRONG_AUTH_NOT_SUPPORTED;
        case LDAP_STRONG_AUTH_REQUIRED:
            return LW_ERROR_LDAP_STRONG_AUTH_REQUIRED;
        case LDAP_PARTIAL_RESULTS:
            return LW_ERROR_LDAP_PARTIAL_RESULTS;
        case LDAP_NO_SUCH_ATTRIBUTE:
            return LW_ERROR_LDAP_NO_SUCH_ATTRIBUTE;
        case LDAP_UNDEFINED_TYPE:
            return LW_ERROR_LDAP_UNDEFINED_TYPE;
        case LDAP_INAPPROPRIATE_MATCHING:
            return LW_ERROR_LDAP_INAPPROPRIATE_MATCHING;
        case LDAP_CONSTRAINT_VIOLATION:
            return LW_ERROR_LDAP_CONSTRAINT_VIOLATION;
        case LDAP_TYPE_OR_VALUE_EXISTS:
            return LW_ERROR_LDAP_TYPE_OR_VALUE_EXISTS;
        case LDAP_INVALID_SYNTAX:
            return LW_ERROR_LDAP_INVALID_SYNTAX;
        case LDAP_NO_SUCH_OBJECT:
            return LW_ERROR_LDAP_NO_SUCH_OBJECT;
        case LDAP_ALIAS_PROBLEM:
            return LW_ERROR_LDAP_ALIAS_PROBLEM;
        case LDAP_INVALID_DN_SYNTAX:
            return LW_ERROR_LDAP_INVALID_DN_SYNTAX;
        case LDAP_IS_LEAF:
            return LW_ERROR_LDAP_IS_LEAF;
        case LDAP_ALIAS_DEREF_PROBLEM:
            return LW_ERROR_LDAP_ALIAS_DEREF_PROBLEM;
        case LDAP_REFERRAL:
            return LW_ERROR_LDAP_REFERRAL;
        case LDAP_ADMINLIMIT_EXCEEDED:
            return LW_ERROR_LDAP_ADMINLIMIT_EXCEEDED;
        case LDAP_UNAVAILABLE_CRITICAL_EXTENSION:
            return LW_ERROR_LDAP_UNAVAILABLE_CRITICAL_EXTENSION;
        case LDAP_CONFIDENTIALITY_REQUIRED:
            return LW_ERROR_LDAP_CONFIDENTIALITY_REQUIRED;
        case LDAP_SASL_BIND_IN_PROGRESS:
            return LW_ERROR_LDAP_SASL_BIND_IN_PROGRESS;
        case LDAP_X_PROXY_AUTHZ_FAILURE:
            return LW_ERROR_LDAP_X_PROXY_AUTHZ_FAILURE;
        case LDAP_INAPPROPRIATE_AUTH:
            return LW_ERROR_LDAP_INAPPROPRIATE_AUTH;
        case LDAP_INVALID_CREDENTIALS:
            return LW_ERROR_LDAP_INVALID_CREDENTIALS;
        case LDAP_INSUFFICIENT_ACCESS:
            return LW_ERROR_LDAP_INSUFFICIENT_ACCESS;
        case LDAP_BUSY:
            return LW_ERROR_LDAP_BUSY;
        case LDAP_UNAVAILABLE:
            return LW_ERROR_LDAP_UNAVAILABLE;
        case LDAP_UNWILLING_TO_PERFORM:
            return LW_ERROR_LDAP_UNWILLING_TO_PERFORM;
        case LDAP_LOOP_DETECT:
            return LW_ERROR_LDAP_LOOP_DETECT;
        case LDAP_NAMING_VIOLATION:
            return LW_ERROR_LDAP_NAMING_VIOLATION;
        case LDAP_OBJECT_CLASS_VIOLATION:
            return LW_ERROR_LDAP_OBJECT_CLASS_VIOLATION;
        case LDAP_NOT_ALLOWED_ON_NONLEAF:
            return LW_ERROR_LDAP_NOT_ALLOWED_ON_NONLEAF;
        case LDAP_NOT_ALLOWED_ON_RDN:
            return LW_ERROR_LDAP_NOT_ALLOWED_ON_RDN;
        case LDAP_ALREADY_EXISTS:
            return LW_ERROR_LDAP_ALREADY_EXISTS;
        case LDAP_NO_OBJECT_CLASS_MODS:
            return LW_ERROR_LDAP_NO_OBJECT_CLASS_MODS;
        case LDAP_RESULTS_TOO_LARGE:
            return LW_ERROR_LDAP_RESULTS_TOO_LARGE;
        case LDAP_AFFECTS_MULTIPLE_DSAS:
            return LW_ERROR_LDAP_AFFECTS_MULTIPLE_DSAS;
        case LDAP_CUP_RESOURCES_EXHAUSTED:
            return LW_ERROR_LDAP_CUP_RESOURCES_EXHAUSTED;
        case LDAP_CUP_SECURITY_VIOLATION:
            return LW_ERROR_LDAP_CUP_SECURITY_VIOLATION;
        case LDAP_CUP_INVALID_DATA:
            return LW_ERROR_LDAP_CUP_INVALID_DATA;
        case LDAP_CUP_UNSUPPORTED_SCHEME:
            return LW_ERROR_LDAP_CUP_UNSUPPORTED_SCHEME;
        case LDAP_CUP_RELOAD_REQUIRED:
            return LW_ERROR_LDAP_CUP_RELOAD_REQUIRED;
        case LDAP_CANCELLED:
            return LW_ERROR_LDAP_CANCELLED;
        case LDAP_NO_SUCH_OPERATION:
            return LW_ERROR_LDAP_NO_SUCH_OPERATION;
        case LDAP_TOO_LATE:
            return LW_ERROR_LDAP_TOO_LATE;
        case LDAP_CANNOT_CANCEL:
            return LW_ERROR_LDAP_CANNOT_CANCEL;
        case LDAP_ASSERTION_FAILED:
            return LW_ERROR_LDAP_ASSERTION_FAILED;
        default:
            LW_LOG_ERROR("Unable to map ldap error %d", dwErr);
            return LW_ERROR_UNKNOWN;
    }
}

DWORD
LwMapLwmsgStatusToLwError(
    LWMsgStatus status
    )
{
    switch (status)
    {
        case LWMSG_STATUS_SUCCESS:
            return LW_ERROR_SUCCESS;
        case LWMSG_STATUS_ERROR:
            return LW_ERROR_INTERNAL;
        case LWMSG_STATUS_MEMORY:
        case LWMSG_STATUS_RESOURCE_LIMIT:
            return LW_ERROR_OUT_OF_MEMORY;
        case LWMSG_STATUS_MALFORMED:
        case LWMSG_STATUS_OVERFLOW:
        case LWMSG_STATUS_UNDERFLOW:
        case LWMSG_STATUS_EOF:
            return LW_ERROR_INVALID_MESSAGE;
        case LWMSG_STATUS_INVALID_PARAMETER:
            return LW_ERROR_INVALID_PARAMETER;
        case LWMSG_STATUS_INVALID_STATE:
            return LW_ERROR_INVALID_PARAMETER;
        case LWMSG_STATUS_UNIMPLEMENTED:
            return LW_ERROR_NOT_IMPLEMENTED;
        case LWMSG_STATUS_SYSTEM:
            return LW_ERROR_INTERNAL;
        case LWMSG_STATUS_SECURITY:
            return LW_ERROR_ACCESS_DENIED;
        case LWMSG_STATUS_CANCELLED:
            return LW_ERROR_INTERRUPTED;
        case LWMSG_STATUS_FILE_NOT_FOUND:
            return ERROR_FILE_NOT_FOUND;
        case LWMSG_STATUS_CONNECTION_REFUSED:
            return LW_ERROR_ERRNO_ECONNREFUSED;
        case LWMSG_STATUS_PEER_RESET:
            return LW_ERROR_ERRNO_ECONNRESET;
        case LWMSG_STATUS_PEER_ABORT:
            return LW_ERROR_ERRNO_ECONNABORTED;
        case LWMSG_STATUS_PEER_CLOSE:
        case LWMSG_STATUS_SESSION_LOST:
            return LW_ERROR_ERRNO_EPIPE;
        case LWMSG_STATUS_INVALID_HANDLE:
            return ERROR_INVALID_HANDLE;
        default:
            LW_LOG_ERROR("Unable to map lwmsg status %d", status);
            return LW_ERROR_INTERNAL;
    }
}
