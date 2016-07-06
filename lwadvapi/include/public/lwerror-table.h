/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 *
 *   NT status and Win32 error code mapping table
 *
 *   Some of these definitions were written from scratch.
 * 
 *   NT status codes are unsigned 32-bit hex values
 *
 *   Win32 error codes are unsigned 32-bit decimal values
 */

/* Win32 codes*/
#ifdef __ERROR_WIN32__

#include <lw/winerror.h>

#endif

/* Equivalence table*/
#ifdef __ERROR_XMACRO__
#define S STATUS_CODE

// List of lw extended errors
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_CACHE_PATH, -1, "An invalid cache path was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_CONFIG_PATH, -1, "The path to the configuration file is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_PREFIX_PATH, -1, "The product installation folder could not be determined" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INSUFFICIENT_BUFFER, -1, "The provided buffer is insufficient" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_OUT_OF_MEMORY, -1, "Out of memory" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_MESSAGE, -1, "The Inter Process message is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNEXPECTED_MESSAGE, -1, "An unexpected Inter Process message was received" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_USER, -1, "No such user" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DATA_ERROR, -1, "The cached data is incorrect" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NOT_IMPLEMENTED, -1, "The requested feature has not been implemented yet" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_CONTEXT_ITEM, -1, "The requested item was not found in context" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_GROUP, -1, "No such group" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_REGEX_COMPILE_FAILED, -1, "Failed to compile regular expression" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NSS_EDIT_FAILED, -1, "Failed to edit nsswitch configuration" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_HANDLER, -1, "No authentication Provider was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INTERNAL, -1, "Internal error" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NOT_HANDLED, -1, "The authentication request could not be handled" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_DNS_RESPONSE, -1, "Response from DNS is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DNS_RESOLUTION_FAILED, -1, "Failed to resolve query using DNS" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_TIME_CONVERSION, -1, "Failed to convert the time" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_SID, -1, "The security identifier (SID) is invalid" )
// Triggered by passing a bad password for a valid username to domainjoin
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_PASSWORD_MISMATCH, -1, "The password is incorrect for the given account" )
// Triggered by passing a bad password for a valid username to domainjoin
S ( LW_ERROR_LDAP_CONSTRAINT_VIOLATION, LW_ERROR_PASSWORD_MISMATCH, -1, "Constraint?" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNEXPECTED_DB_RESULT, -1, "Unexpected cached data found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_PASSWORD_EXPIRED, -1, "Password expired" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ACCOUNT_EXPIRED, -1, "Account expired" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_USER_EXISTS, -1, "A user by this name already exists" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_GROUP_EXISTS, -1, "A group by this name already exists" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_GROUP_INFO_LEVEL, -1, "An invalid group info level was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_USER_INFO_LEVEL, -1, "An invalid user info level was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNSUPPORTED_USER_LEVEL, -1, "This interface does not support the user info level specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNSUPPORTED_GROUP_LEVEL, -1, "This interface does not support the group info level specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_LOGIN_ID, -1, "The login id is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_HOMEDIR, -1, "The home directory is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_GROUP_NAME, -1, "The group name is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_MORE_GROUPS, -1, "No more groups" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_MORE_USERS, -1, "No more users" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_ADD_USER, -1, "Failed to add user" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_ADD_GROUP, -1, "Failed to add group" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_LSA_CONNECTION, -1, "The connection to the authentication service is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_AUTH_PROVIDER, -1, "The authentication provider is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_PARAMETER, -1, "Invalid parameter" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_PARENT_DN, -1, "No distinguished name found in LDAP for parent of this object" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_ERROR, -1, "An Error was encountered when negotiating with LDAP" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_DOMAIN, -1, "Unknown Active Directory domain" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_FAILED_GETDN, -1, "Failed to find distinguished name using LDAP" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DUPLICATE_DOMAINNAME, -1, "A duplicate Active Directory domain name was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_KRB5_CALL_FAILED, -1, "The call to Kerberos 5 failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_GSS_CALL_FAILED, -1, "The GSS call failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_FIND_DC, -1, "Failed to find the domain controller" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_CELL, -1, "Failed to find the Cell in Active Directory" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_GROUP_IN_USE, -1, "The specified group is currently being used" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_CREATE_HOMEDIR, -1, "Failed to create home directory for user" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_PASSWORD_TOO_WEAK, -1, "The specified password is not strong enough" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_SID_REVISION, -1, "The security descriptor (SID) has an invalid revision" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ACCOUNT_LOCKED, -1, "The user account is locked" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ACCOUNT_DISABLED, -1, "The account is disabled" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_USER_CANNOT_CHANGE_PASSWD, -1, "The user is not allowed to change his/her password" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LOAD_LIBRARY_FAILED, -1, "Failed to dynamically load a library" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LOOKUP_SYMBOL_FAILED, -1, "Failed to lookup a symbol in a dynamic library" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_EVENTLOG, -1, "The Eventlog interface is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_CONFIG, -1, "The specified configuration (file) is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNEXPECTED_TOKEN, -1, "An unexpected token was encountered in the configuration" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_RECORDS_FOUND, -1, "No records were found in the cache" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DUPLICATE_USERNAME, -1, "A duplicate user record was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DUPLICATE_GROUPNAME, -1, "A duplicate group record was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DUPLICATE_CELLNAME, -1, "A duplicate cell was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_ACCOUNT, -1, "The user account is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_STRING_CONV_FAILED, -1, "Failed to convert string format (wide/ansi)" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_PASSWORD, -1, "The password is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_QUERY_CREATION_FAILED, -1, "Failed to create query to examine cache" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_OBJECT, -1, "No such user, group or domain object" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DUPLICATE_USER_OR_GROUP, -1, "A duplicate user or group was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NOT_JOINED_TO_AD, -1, "This machine is not currently joined to Active Directory" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_KRB5_CACHE_TYPE, -1, "An invalid kerberos 5 cache type was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_TO_SET_TIME, -1, "The system time could not be set" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_NETBIOS_NAME, -1, "Failed to find NetBIOS name for the domain" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_NETLOGON_RESPONSE, -1, "The Netlogon response buffer is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_OBJECTGUID, -1, "The specified Globally Unique Identifier (GUID) is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_DOMAIN, -1, "The domain name is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_DEFAULT_REALM, -1, "The kerberos default realm is not set" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NOT_SUPPORTED, -1, "The request is not supported" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LOGON_FAILURE, -1, "The logon request failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SITE_INFORMATION, -1, "No site information was found for the active directory domain" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_LDAP_STRING, -1, "The LDAP string value is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_LDAP_ATTR_VALUE, -1, "The LDAP attribute value is NULL or invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NULL_BUFFER, -1, "An invalid buffer was provided" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_CLOCK_SKEW, -1, "Clock skew detected with active directory server" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_KRB5_NO_KEYS_FOUND, -1, "No kerberos keys found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_SERVICE_NOT_AVAILABLE, -1, "The authentication service is not available" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_SERVICE_RESPONSE, -1, "Invalid response from the authentication service" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NSS_ERROR, -1, "Name Service Switch Error" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_AUTH_ERROR, -1, "Authentication Error" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_LDAP_DN, -1, "Invalid Ldap distinguished name (DN)" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NOT_MAPPED, -1, "Account Name or SID not mapped" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_NETLOGON_FAILED, -1, "Calling librpc/NetLogon API failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ENUM_DOMAIN_TRUSTS_FAILED, -1, "Enumerating domain trusts failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_LSABINDING_FAILED, -1, "Calling librpc/Lsa Binding failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_OPENPOLICY_FAILED, -1, "Calling librpc/Lsa Open Policy failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED, -1, "Calling librpc/Lsa LookupName2 failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_SET_SESS_CREDS_FAILED, -1, "RPC Set Session Creds failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_REL_SESS_CREDS_FAILED, -1, "RPC Release Session Creds failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_CLOSEPOLICY_FAILED, -1, "Calling librpc/Lsa Close Handle/policy failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND, -1, "No user/group was found using RPC lookup name to objectSid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES, -1, "Abnormal duplicates were found using RPC lookup name to objectSid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_TRUSTED_DOMAIN_FOUND, -1, "No such trusted domain found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS, -1, "Trusted domain is executing in an incompatiable modes" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DCE_CALL_FAILED, -1, "A call to DCE/RPC failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_TO_LOOKUP_DC, -1, "Failed to lookup the domain controller for given domain" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL, -1, "An invalid nss artefact info level was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL, -1, "The NSS artefact info level specified is not supported" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_USER_NAME, -1, "An invalid user name was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_LOG_LEVEL, -1, "An invalid log level was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_METRIC_TYPE, -1, "An invalid metric type was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_METRIC_PACK, -1, "An invalid metric pack was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_METRIC_INFO_LEVEL, -1, "An invalid info level was specified when querying metrics" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK, -1, "The system hostname configuration is incorrect" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED, -1, "Could not find DirectoryService cache utility to call -flushcache with" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LSA_SERVER_UNREACHABLE, -1, "The LSASS server is not responding." )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_NSS_ARTEFACT_TYPE, -1, "An invalid NSS Artefact type was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_AGENT_VERSION, -1, "The LSASS Server version is invalid" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DOMAIN_IS_OFFLINE, -1, "The domain is offline" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_HOMEDIR_TEMPLATE, -1, "An invalid home directory template was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_PARSE_SID_STRING, -1, "Failed to use NetAPI to parse SID string" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_LSA_LOOKUPSIDS_FAILED, -1, "Failed to use NetAPI to lookup names for SIDs" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND, -1, "Failed to find any names for SIDs using NetAPI" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES, -1, "Found duplicates using RPC Call to lookup SIDs" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_PASSWORD_RESTRICTION, -1, "Password does not meet requirements" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_OBJECT_NOT_ENABLED, -1, "The user/group is not enabled in the cell" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_MORE_NSS_ARTEFACTS, -1, "No more NSS Artefacts" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_NSS_MAP_NAME, -1, "An invalid name was specified for the NIS map" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_NSS_KEY_NAME, -1, "An invalid name was specified for the NIS key" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_NSS_KEY, -1, "No such NIS Key is defined" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_NSS_MAP, -1, "No such NIS Map is defined" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_ERROR, -1, "An Error was encountered when negotiating with RPC" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_SERVER_UNAVAILABLE, -1, "The LDAP server is unavailable" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_CREATE_KEY_FAILED, -1, "Could not create random key" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_CANNOT_DETECT_USER_PROCESSES, -1, "Cannot detect user processes" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_TRACE_NOT_INITIALIZED, -1, "Tracing has not been initialized" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_TRACE_FLAG, -1, "The trace flag is not defined" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DCERPC_ERROR, -1, "DCE-RPC Error" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_RPC_SERVER, -1, "Invalid RPC Server" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_SERVER_REGISTRATION_ERROR, -1, "RPC Server Registration error" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_RPC_SERVER_RUNTIME_ERROR, -1, "RPC Server runtime error" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DOMAIN_IN_USE, -1, "The domain is in use" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_SAM_DATABASE_ERROR, -1, "SAM database error" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_SAM_INIT_ERROR, -1, "Error when initializing SAM subsystem" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_OBJECT_IN_USE, -1, "The object is in use" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_ATTRIBUTE, -1, "No such attribute was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_GET_DC_NAME_FAILED, -1, "Get domain controller name failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_ATTRIBUTE_VALUE, -1, "An invalid attribute value was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_ATTRIBUTE_VALUE, -1, "No attribute value was found" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_MEMBER_IN_LOCAL_GROUP, -1, "Member is in local group" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_MEMBER_NOT_IN_LOCAL_GROUP, -1, "No such member in local group" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_GROUP, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_WARNING_CONTINUE_NEEDED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ACCESS_DENIED, -1, "Incorrect access attempt" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_PROCESS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INTERRUPTED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_GENERIC_IO, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_HANDLE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENXIO, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_E2BIG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOEXEC, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ECHILD, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EAGAIN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EFAULT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTBLK, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBUSY, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EEXIST, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EXDEV, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENODEV, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTDIR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EISDIR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENFILE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EMFILE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTTY, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ETXTBSY, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EFBIG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOSPC, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ESPIPE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EROFS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EMLINK, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EPIPE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EDOM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ERANGE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNKNOWN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENAMETOOLONG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOLCK, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTEMPTY, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ELOOP, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOMSG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EIDRM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ECHRNG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EL2NSYNC, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EL3HLT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EL3RST, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ELNRNG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EUNATCH, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOCSI, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EL2HLT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBADE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBADR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EXFULL, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOANO, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBADRQC, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBADSLT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EDEADLOCK, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBFONT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOSTR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENODATA, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ETIME, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOSR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENONET, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOPKG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EREMOTE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOLINK, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EADV, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ESRMNT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ECOMM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EPROTO, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EMULTIHOP, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EDOTDOT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBADMSG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EOVERFLOW, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTUNIQ, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EBADFD, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EREMCHG, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ELIBACC, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ELIBBAD, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ELIBSCN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ELIBMAX, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ELIBEXEC, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EILSEQ, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ERESTART, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ESTRPIPE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EUSERS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTSOCK, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EDESTADDRREQ, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EMSGSIZE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EPROTOTYPE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOPROTOOPT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EPROTONOSUPPORT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ESOCKTNOSUPPORT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EOPNOTSUPP, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EPFNOSUPPORT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EAFNOSUPPORT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EADDRINUSE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EADDRNOTAVAIL, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENETDOWN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENETUNREACH, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENETRESET, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ECONNABORTED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ECONNRESET, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOBUFS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EISCONN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTCONN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ESHUTDOWN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ETOOMANYREFS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ETIMEDOUT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ECONNREFUSED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EHOSTDOWN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EHOSTUNREACH, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EALREADY, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EINPROGRESS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ESTALE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EUCLEAN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOTNAM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENAVAIL, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EISNAM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EREMOTEIO, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EDQUOT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ENOMEDIUM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_EMEDIUMTYPE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_ERRNO_ECANCELED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_SERVER_DOWN, -1, "The DC closed an LDAP connection in the middle of a query" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_LOCAL_ERROR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_ENCODING_ERROR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_DECODING_ERROR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_TIMEOUT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_AUTH_UNKNOWN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_FILTER_ERROR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_USER_CANCELLED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_PARAM_ERROR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_MEMORY, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CONNECT_ERROR, -1, "The DC sent a notice of disconnect to a LDAP connection during the query." )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NOT_SUPPORTED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CONTROL_NOT_FOUND, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_RESULTS_RETURNED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_MORE_RESULTS_TO_RETURN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CLIENT_LOOP, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_REFERRAL_LIMIT_EXCEEDED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_OPERATIONS_ERROR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_PROTOCOL_ERROR, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_TIMELIMIT_EXCEEDED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_SIZELIMIT_EXCEEDED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_COMPARE_FALSE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_COMPARE_TRUE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_STRONG_AUTH_NOT_SUPPORTED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_STRONG_AUTH_REQUIRED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_PARTIAL_RESULTS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_SUCH_ATTRIBUTE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_UNDEFINED_TYPE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_INAPPROPRIATE_MATCHING, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CONSTRAINT_VIOLATION, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_TYPE_OR_VALUE_EXISTS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_SUCH_OBJECT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_ALIAS_PROBLEM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_INVALID_DN_SYNTAX, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_IS_LEAF, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_ALIAS_DEREF_PROBLEM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_REFERRAL, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_ADMINLIMIT_EXCEEDED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_UNAVAILABLE_CRITICAL_EXTENSION, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CONFIDENTIALITY_REQUIRED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_SASL_BIND_IN_PROGRESS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_X_PROXY_AUTHZ_FAILURE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_INAPPROPRIATE_AUTH, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_INVALID_CREDENTIALS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_INSUFFICIENT_ACCESS, -1, "Insufficient access to perform LDAP operation" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_BUSY, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_UNAVAILABLE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_UNWILLING_TO_PERFORM, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_LOOP_DETECT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NAMING_VIOLATION, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_OBJECT_CLASS_VIOLATION, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NOT_ALLOWED_ON_NONLEAF, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NOT_ALLOWED_ON_RDN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_ALREADY_EXISTS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_OBJECT_CLASS_MODS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_RESULTS_TOO_LARGE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_AFFECTS_MULTIPLE_DSAS, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CUP_RESOURCES_EXHAUSTED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CUP_SECURITY_VIOLATION, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CUP_INVALID_DATA, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CUP_UNSUPPORTED_SCHEME, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CUP_RELOAD_REQUIRED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CANCELLED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_NO_SUCH_OPERATION, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_TOO_LATE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_CANNOT_CANCEL, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_ASSERTION_FAILED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_LDAP_INVALID_SYNTAX, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_BAD_MECH, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_BAD_NAMETYPE, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_BAD_NAME, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_CONTEXT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_CREDENTIAL, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_CONTEXT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_CRED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_TOKEN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNSUPPORTED_SUBPROTO, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UNSUPPORTED_CRYPTO_OP, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_TEST_FAILED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_CSV_BAD_FORMAT, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_CSV_NO_SUCH_FIELD, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_SERVICE_TRANSITION, -1, "The operation is invalid from the service's present state" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_SERVICE_DEPENDENCY_UNMET, -1, "The service cannot be started because another service it depends on is not running" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_SERVICE_UNRESPONSIVE, -1, "The service is not responding to requests" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_NO_SUCH_SERVICE, -1, "No service with the specified name exists" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DEPENDENT_SERVICE_STILL_RUNNING, -1, "The service cannot be stopped because another service that depends on it is still running" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_INVALID_OU, -1, "An invalid Organizational Unit was specified" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UID_TOO_LOW, -1, "The specified user ID too low" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_UID_TOO_HIGH, -1, "The specified user ID too high" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_GID_TOO_LOW, -1, "The specified group ID too low" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_GID_TOO_HIGH, -1, "The specified group ID too high" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_AUTOENROLL_FAILED, -1, "Certificate auto-enrollment failed with a general SOAP fault" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_AUTOENROLL_HTTP_REQUEST_FAILED, -1, "Certificate auto-enrollment HTTP request failed" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_AUTOENROLL_SUBJECT_NAME_REQUIRED, -1, "A subject name must be provided for the specified certificate template" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DOMAINJOIN_LEAVE_LICENSE_NOT_RELEASED , -1, "The domain leave completed but the license could not be released" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_DOMAINJOIN_LEAVE_MACHINE_ACCT_DELETE_FAILED_INSUFFICIENT_ACCESS , -1, "The computer account could not be deleted, the specified user does not have sufficient rights to delete the object" )


#include "lwerror-table-krb5.h"

#undef S
#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
