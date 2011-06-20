/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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

#include "../includes.h"


LONG
LWGetMacError(
    DWORD dwError
    )
{
    LONG macError = 0;

    switch (dwError)
    {
        case MAC_AD_ERROR_SUCCESS:
            macError = eDSNoErr;
            break;

        case EPERM:
            macError = eDSPermissionError;
            break;

        case ENOENT:
            macError = eDSRecordNotFound;
            break;

        case eDSPermissionError:
            macError = eDSPermissionError;
            break;

        case EACCES:
            macError = eDSNotAuthorized;
            break;

        case eDSNotAuthorized:
            macError = eDSNotAuthorized;
            break;

        case ENOMEM:
            macError = eMemoryAllocError;
            break;

        case eMemoryAllocError:
            macError = eMemoryAllocError;
            break;

        case EINVAL:
            macError = eParameterError;
            break;

        case MAC_AD_ERROR_NOT_IMPLEMENTED:
            macError = eNotYetImplemented;
            break;

        case eNotYetImplemented:
            macError = eNotYetImplemented;
            break;

        case MAC_AD_ERROR_INVALID_PARAMETER:
            macError = eParameterError;
            break;

        case eParameterError:
            macError = eParameterError;
            break;

        case MAC_AD_ERROR_NOT_SUPPORTED:
            macError = eDSAuthMethodNotSupported;
            break;

        case eDSAuthMethodNotSupported:
            macError = eDSAuthMethodNotSupported;
            break;

        case MAC_AD_ERROR_LOGON_FAILURE:
            macError = eDSAuthFailed;
            break;

        case eDSAuthFailed:
            macError = eDSAuthFailed;
            break;

        case MAC_AD_ERROR_INVALID_NAME:
            macError = eDSInvalidName;
            break;

        case MAC_AD_ERROR_UPN_NOT_FOUND:
            macError = eDSInvalidName;
            break;

        case MAC_AD_ERROR_NULL_PARAMETER:
            macError = eDSNullParameter;
            break;

        case eDSNullParameter:
            macError = eDSNullParameter;
            break;

        case MAC_AD_ERROR_INVALID_TAG:
            macError = eDSInvalidTag;
            break;

        case eDSInvalidTag:
            macError = eDSInvalidTag;
            break;

        case MAC_AD_ERROR_NO_SUCH_ATTRIBUTE:
            macError = eDSAttributeDoesNotExist;
            break;

        case eDSAttributeDoesNotExist:
            macError = eDSAttributeDoesNotExist;
            break;

        case MAC_AD_ERROR_INVALID_RECORD_TYPE:
            macError = eDSInvalidRecordType;
            break;

        case eDSInvalidRecordType:
            macError = eDSInvalidRecordType;
            break;

        case MAC_AD_ERROR_INVALID_ATTRIBUTE_TYPE:
            macError = eDSInvalidAttributeType;
            break;

        case eDSInvalidAttributeType:
            macError = eDSInvalidAttributeType;
            break;

        case MAC_AD_ERROR_INSUFFICIENT_BUFFER:
            macError = eDSBufferTooSmall;
            break;

        case eDSBufferTooSmall:
            macError = eDSBufferTooSmall;
            break;

        case MAC_AD_ERROR_IPC_FAILED:
            macError = eIPCSendError;
            break;

        case eIPCSendError:
            macError = eIPCSendError;
            break;

        case MAC_AD_ERROR_NO_PROC_STATUS:
            macError = ePlugInError;
            break;

        case ePlugInError:
            macError = ePlugInError;
            break;

        case MAC_AD_ERROR_KRB5_PASSWORD_MISMATCH:
            macError = eDSAuthBadPassword;
            break;

        case eDSAuthBadPassword:
            macError = eDSAuthBadPassword;
            break;

        case MAC_AD_ERROR_KRB5_PASSWORD_EXPIRED:
            macError = eDSAuthPasswordExpired;
            break;

        case eDSAuthAccountExpired:
            macError = eDSAuthAccountExpired;
            break;

        case MAC_AD_ERROR_KRB5_CLOCK_SKEW:
            macError = eDSAuthMasterUnreachable;
            break;

        case MAC_AD_ERROR_KRB5_ERROR:
            macError = eDSAuthMasterUnreachable;
            break;

        case MAC_AD_ERROR_GSS_API_FAILED:
            macError = eDSAuthMasterUnreachable;
            break;

        case eDSAuthMasterUnreachable:
            macError = eDSAuthMasterUnreachable;
            break;

        case MAC_AD_ERROR_NO_SUCH_POLICY:
            macError = eDSRecordNotFound;
            break;

        case eDSRecordNotFound:
            macError = eDSRecordNotFound;
            break;

        case MAC_AD_ERROR_LDAP_OPEN:
            macError = eDSBogusServer;
            break;

        case eDSBogusServer:
            macError = eDSBogusServer;
            break;

        case MAC_AD_ERROR_LDAP_SET_OPTION:
            macError = eParameterSendError;
            break;

        case eParameterSendError:
            macError = eParameterSendError;
            break;

        case MAC_AD_ERROR_LDAP_QUERY_FAILED:
            macError = eDSReceiveFailed;
            break;

        case eDSReceiveFailed:
            macError = eDSReceiveFailed;
            break;

        case MAC_AD_ERROR_COMMAND_FAILED:
            macError = eDSOperationFailed;
            break;

        case MAC_AD_ERROR_LOAD_LIBRARY_FAILED:
            macError = ePlugInFailedToInitialize;
            break;

        case MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED:
            macError = ePlugInFailedToInitialize;
            break;

        case eDSOperationFailed:
            macError = eDSOperationFailed;
            break;

        /* Begin LSASS error mappings */

        case LW_ERROR_NO_SUCH_USER:
            macError = eDSAuthUnknownUser;
            break;

        case LW_ERROR_DATA_ERROR:
        case LW_ERROR_NOT_IMPLEMENTED:
        case LW_ERROR_NO_CONTEXT_ITEM:
        case LW_ERROR_NO_SUCH_GROUP:
        case LW_ERROR_REGEX_COMPILE_FAILED:
        case LW_ERROR_NSS_EDIT_FAILED:
        case LW_ERROR_NO_HANDLER:
        case LW_ERROR_INTERNAL:
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_INVALID_DNS_RESPONSE:
        case LW_ERROR_DNS_RESOLUTION_FAILED:
        case LW_ERROR_FAILED_TIME_CONVERSION:
        case LW_ERROR_INVALID_SID:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_PASSWORD_MISMATCH:
            macError = eDSAuthBadPassword;
            break;

        case LW_ERROR_UNEXPECTED_DB_RESULT:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_PASSWORD_EXPIRED:
            macError = eDSAuthPasswordExpired;
            break;

        /* BUGBUG - Should there be an LSASS error for password must change? Or others? */
        //  macError = eDSAuthNewPasswordRequired;
        //  macError = eDSAuthPasswordTooShort;
        //  macError = eDSAuthPasswordTooLong;
        //  macError = eDSAuthPasswordNeedsLetter;
        //  macError = eDSAuthPasswordNeedsDigit;
        //  macError = eDSAuthNewPasswordRequired;
        //  macError = eDSAuthAccountInactive;

        case LW_ERROR_ACCOUNT_EXPIRED:
            macError = eDSAuthAccountExpired;
            break;

        case LW_ERROR_USER_EXISTS:
        case LW_ERROR_GROUP_EXISTS:
        case LW_ERROR_INVALID_GROUP_INFO_LEVEL:
        case LW_ERROR_INVALID_USER_INFO_LEVEL:
        case LW_ERROR_UNSUPPORTED_USER_LEVEL:
        case LW_ERROR_UNSUPPORTED_GROUP_LEVEL:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_INVALID_LOGIN_ID:
            macError = eDSAuthUnknownUser;
            break;

        case LW_ERROR_INVALID_GROUP_NAME:
        case LW_ERROR_NO_MORE_GROUPS:
        case LW_ERROR_NO_MORE_USERS:
        case LW_ERROR_FAILED_ADD_USER:
        case LW_ERROR_FAILED_ADD_GROUP:
        case LW_ERROR_INVALID_LSA_CONNECTION:
        case LW_ERROR_INVALID_AUTH_PROVIDER:
        case LW_ERROR_INVALID_PARAMETER:
        case LW_ERROR_LDAP_NO_PARENT_DN:
        case LW_ERROR_LDAP_ERROR:
        case LW_ERROR_NO_SUCH_DOMAIN:
        case LW_ERROR_LDAP_FAILED_GETDN:
        case LW_ERROR_DUPLICATE_DOMAINNAME:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_KRB5_CALL_FAILED:
        case LW_ERROR_GSS_CALL_FAILED:
            macError = eDSAuthFailed;
            break;

        case LW_ERROR_FAILED_FIND_DC:
        case LW_ERROR_NO_SUCH_CELL:
        case LW_ERROR_GROUP_IN_USE:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_PASSWORD_TOO_WEAK:
            macError = eDSAuthPasswordQualityCheckFailed;
            break;

        case LW_ERROR_INVALID_SID_REVISION:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_ACCOUNT_LOCKED:
        case LW_ERROR_FAILED_CREATE_HOMEDIR:
        case LW_ERROR_INVALID_HOMEDIR:
            macError = eDSAuthFailed;
            break;

        case LW_ERROR_ACCOUNT_DISABLED:
            macError = eDSAuthAccountDisabled;
            break;

        case LW_ERROR_USER_CANNOT_CHANGE_PASSWD:
        case LW_ERROR_LOAD_LIBRARY_FAILED:
        case LW_ERROR_LOOKUP_SYMBOL_FAILED:
        case LW_ERROR_INVALID_EVENTLOG:
        case LW_ERROR_INVALID_CONFIG:
        case LW_ERROR_UNEXPECTED_TOKEN:
        case LW_ERROR_LDAP_NO_RECORDS_FOUND:
        case LW_ERROR_DUPLICATE_USERNAME:
        case LW_ERROR_DUPLICATE_GROUPNAME:
        case LW_ERROR_DUPLICATE_CELLNAME:
        case LW_ERROR_STRING_CONV_FAILED:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_INVALID_PASSWORD:
            macError = eDSAuthBadPassword;
            break;

        case LW_ERROR_QUERY_CREATION_FAILED:
        case LW_ERROR_NO_SUCH_OBJECT:
        case LW_ERROR_DUPLICATE_USER_OR_GROUP:
        case LW_ERROR_INVALID_KRB5_CACHE_TYPE:
        case LW_ERROR_FAILED_TO_SET_TIME:
        case LW_ERROR_NO_NETBIOS_NAME:
        case LW_ERROR_INVALID_NETLOGON_RESPONSE:
        case LW_ERROR_INVALID_OBJECTGUID:
        case LW_ERROR_INVALID_DOMAIN:
        case LW_ERROR_NO_DEFAULT_REALM:
        case LW_ERROR_NOT_SUPPORTED:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_LOGON_FAILURE:
            macError = eDSAuthFailed;
            break;

        case LW_ERROR_NO_SITE_INFORMATION:
        case LW_ERROR_INVALID_LDAP_STRING:
        case LW_ERROR_INVALID_LDAP_ATTR_VALUE:
        case LW_ERROR_NULL_BUFFER:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_CLOCK_SKEW:
        case LW_ERROR_KRB5_NO_KEYS_FOUND:
            macError = eDSAuthFailed;
            break;

        case LW_ERROR_SERVICE_NOT_AVAILABLE:
        case LW_ERROR_INVALID_SERVICE_RESPONSE:
        case LW_ERROR_NSS_ERROR:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_AUTH_ERROR:
            macError = eDSAuthFailed;
            break;

        case LW_ERROR_INVALID_LDAP_DN:
        case LW_ERROR_NOT_MAPPED:
        case LW_ERROR_RPC_NETLOGON_FAILED:
        case LW_ERROR_ENUM_DOMAIN_TRUSTS_FAILED:
        case LW_ERROR_RPC_LSABINDING_FAILED:
        case LW_ERROR_RPC_OPENPOLICY_FAILED:
        case LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED:
        case LW_ERROR_RPC_SET_SESS_CREDS_FAILED:
        case LW_ERROR_RPC_REL_SESS_CREDS_FAILED:
        case LW_ERROR_RPC_CLOSEPOLICY_FAILED:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND:
            macError = eDSAuthUnknownUser;
            break;

        case LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES:
        case LW_ERROR_NO_TRUSTED_DOMAIN_FOUND:
        case LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS:
        case LW_ERROR_DCE_CALL_FAILED:
        case LW_ERROR_FAILED_TO_LOOKUP_DC:
        case LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL:
        case LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL:
            macError = eDSOperationFailed;
            break;

        case LW_ERROR_INVALID_USER_NAME:
            macError = eDSAuthUnknownUser;
            break;

        case LW_ERROR_INVALID_LOG_LEVEL:
        case LW_ERROR_INVALID_METRIC_TYPE:
        case LW_ERROR_INVALID_METRIC_PACK:
        case LW_ERROR_INVALID_METRIC_INFO_LEVEL:
        case LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK:
        case LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED:
        case LW_ERROR_LSA_SERVER_UNREACHABLE:
        case LW_ERROR_INVALID_NSS_ARTEFACT_TYPE:
        case LW_ERROR_INVALID_AGENT_VERSION:
            macError = eDSOperationFailed;
            break;
        /* End of LSASS error mappings */

        /* Begin LWNET error mappings */
        case ERROR_PATH_NOT_FOUND:
            macError = eDSOperationFailed;
            break;

        case ERROR_OUTOFMEMORY:
            macError = eMemoryAllocError;
            break;

        case DNS_ERROR_BAD_PACKET:
        case ERROR_CALL_NOT_IMPLEMENTED:
        case ERROR_BAD_FORMAT:
        case ERROR_DLL_INIT_FAILED:
        case ERROR_INTERNAL_ERROR:
        case ERROR_NOT_FOUND:
        case ERROR_INVALID_TIME:

        case ERROR_INVALID_PARAMETER:
            macError = eParameterError;
            break;

        case ERROR_NO_SUCH_DOMAIN:
        case NERR_DCNotFound:
        case NERR_SetupNotJoined:
        case ERROR_BAD_DLL_ENTRYPOINT:
        case ERROR_EVENTLOG_CANT_START:
        case ERROR_BAD_CONFIGURATION:
        case ERROR_ILLEGAL_CHARACTER:
        case ERROR_NOT_JOINED:
        case ERROR_WRITE_FAULT:
        case ERROR_SERVICE_DEPENDENCY_FAIL:
            macError = eDSOperationFailed;
            break;
        /* End of LWNET error mappings */

        default:
            LOG_ERROR("Unable to map system error (%d) to macError", dwError);
            macError = dwError;
            break;
    }

    return macError;
}

#define CASE_RETURN_STRING(token)               \
    case token:                                 \
    return #token

#define DEFAULT_RETURN_UNKNOWN_STRING()         \
    default:                                    \
    return "*UNKNOWN*"


