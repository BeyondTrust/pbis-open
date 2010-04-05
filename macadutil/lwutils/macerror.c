#include "includes.h"

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

        case LW_ERROR_INVALID_ACCOUNT:
            macError = eDSAuthUnknownUser;
            break;

        case LW_ERROR_INVALID_PASSWORD:
            macError = eDSAuthBadPassword;
            break;

        case LW_ERROR_QUERY_CREATION_FAILED:
        case LW_ERROR_NO_SUCH_OBJECT:
        case LW_ERROR_DUPLICATE_USER_OR_GROUP:
        case LW_ERROR_INVALID_KRB5_CACHE_TYPE:
        case LW_ERROR_NOT_JOINED_TO_AD:
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
            MAC_AD_LOG_ERROR("Unable to map system error (%d) to macError",
                             dwError);
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

PCSTR
MacErrorToString(LONG MacError)
{
    switch (MacError)
    {
        CASE_RETURN_STRING(MAC_AD_ERROR_SUCCESS);
        /* Begin System errors */
        CASE_RETURN_STRING(EPERM);
        CASE_RETURN_STRING(ENOENT);
        //CASE_RETURN_STRING(ESRCH);
        CASE_RETURN_STRING(EINTR);
        CASE_RETURN_STRING(EIO);
        CASE_RETURN_STRING(ENXIO);
        CASE_RETURN_STRING(E2BIG);
        CASE_RETURN_STRING(ENOEXEC);
        CASE_RETURN_STRING(EBADF);
        CASE_RETURN_STRING(ECHILD);
        //CASE_RETURN_STRING(EDEADLK);
        CASE_RETURN_STRING(ENOMEM);
        CASE_RETURN_STRING(EACCES);
        CASE_RETURN_STRING(EFAULT);
        CASE_RETURN_STRING(ENOTBLK);
        CASE_RETURN_STRING(EBUSY);
        CASE_RETURN_STRING(EEXIST);
        CASE_RETURN_STRING(EXDEV);
        CASE_RETURN_STRING(ENODEV);
        CASE_RETURN_STRING(ENOTDIR);
        CASE_RETURN_STRING(EISDIR);
        CASE_RETURN_STRING(EINVAL);
        CASE_RETURN_STRING(ENFILE);
        CASE_RETURN_STRING(ENOTTY);
        CASE_RETURN_STRING(ETXTBSY);
        CASE_RETURN_STRING(EFBIG);
        CASE_RETURN_STRING(ENOSPC);
        //CASE_RETURN_STRING(ESPIPE);
        CASE_RETURN_STRING(EROFS);
        CASE_RETURN_STRING(EMLINK);
        CASE_RETURN_STRING(EPIPE);
        CASE_RETURN_STRING(EDOM);
        CASE_RETURN_STRING(ERANGE);
        CASE_RETURN_STRING(EAGAIN);
        CASE_RETURN_STRING(EINPROGRESS);
        CASE_RETURN_STRING(EALREADY);
        CASE_RETURN_STRING(ENOTSOCK);
        CASE_RETURN_STRING(EDESTADDRREQ);
        CASE_RETURN_STRING(EMSGSIZE);
        CASE_RETURN_STRING(EPROTOTYPE);
        CASE_RETURN_STRING(ENOPROTOOPT);
        CASE_RETURN_STRING(EPROTONOSUPPORT);
        CASE_RETURN_STRING(ESOCKTNOSUPPORT);
        CASE_RETURN_STRING(ENOTSUP);
        CASE_RETURN_STRING(EPFNOSUPPORT);
        CASE_RETURN_STRING(EAFNOSUPPORT);
        CASE_RETURN_STRING(EADDRINUSE);
        CASE_RETURN_STRING(EADDRNOTAVAIL);
        CASE_RETURN_STRING(ENETDOWN);
        CASE_RETURN_STRING(ENETUNREACH);
        CASE_RETURN_STRING(ENETRESET);
        CASE_RETURN_STRING(ECONNABORTED);
        CASE_RETURN_STRING(ECONNRESET);
        CASE_RETURN_STRING(ENOBUFS);
        CASE_RETURN_STRING(EISCONN);
        CASE_RETURN_STRING(ENOTCONN);
        CASE_RETURN_STRING(ESHUTDOWN);
        CASE_RETURN_STRING(ETOOMANYREFS);
        CASE_RETURN_STRING(ETIMEDOUT);
        CASE_RETURN_STRING(ECONNREFUSED);
        CASE_RETURN_STRING(ELOOP);
        CASE_RETURN_STRING(ENAMETOOLONG);
        CASE_RETURN_STRING(EHOSTDOWN);
        CASE_RETURN_STRING(EHOSTUNREACH);
        CASE_RETURN_STRING(ENOTEMPTY);
        CASE_RETURN_STRING(EPROCLIM);
        CASE_RETURN_STRING(EUSERS);
        CASE_RETURN_STRING(EDQUOT);
        CASE_RETURN_STRING(ESTALE);
        CASE_RETURN_STRING(EREMOTE);
        CASE_RETURN_STRING(EBADRPC);
        CASE_RETURN_STRING(ERPCMISMATCH);
        CASE_RETURN_STRING(EPROGUNAVAIL);
        CASE_RETURN_STRING(EPROGMISMATCH);
        CASE_RETURN_STRING(EPROCUNAVAIL);
        CASE_RETURN_STRING(ENOLCK);
        CASE_RETURN_STRING(ENOSYS);
        CASE_RETURN_STRING(EFTYPE);
        CASE_RETURN_STRING(EAUTH);
        CASE_RETURN_STRING(ENEEDAUTH);
        CASE_RETURN_STRING(EPWROFF);
        CASE_RETURN_STRING(EDEVERR);
        CASE_RETURN_STRING(EOVERFLOW);
        CASE_RETURN_STRING(EBADEXEC);
        CASE_RETURN_STRING(EBADARCH);
        //CASE_RETURN_STRING(ESHLIBVERS);
        CASE_RETURN_STRING(EBADMACHO);
        CASE_RETURN_STRING(ECANCELED);
        CASE_RETURN_STRING(EIDRM);
        CASE_RETURN_STRING(ENOMSG);
        CASE_RETURN_STRING(EILSEQ);
        CASE_RETURN_STRING(ENOATTR);
        CASE_RETURN_STRING(EBADMSG);
        CASE_RETURN_STRING(EMULTIHOP);
        CASE_RETURN_STRING(ENODATA);
        CASE_RETURN_STRING(ENOLINK);
        CASE_RETURN_STRING(ENOSR);
        CASE_RETURN_STRING(ENOSTR);
        CASE_RETURN_STRING(EPROTO);
        CASE_RETURN_STRING(ETIME);
        /* End System Errors */
        /* Begin macadutil errors */
        CASE_RETURN_STRING(MAC_AD_ERROR_NOT_IMPLEMENTED);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_PARAMETER);
        CASE_RETURN_STRING(MAC_AD_ERROR_NOT_SUPPORTED);
        CASE_RETURN_STRING(MAC_AD_ERROR_LOGON_FAILURE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_NAME);
        CASE_RETURN_STRING(MAC_AD_ERROR_UPN_NOT_FOUND);
        CASE_RETURN_STRING(MAC_AD_ERROR_NULL_PARAMETER);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_TAG);
        CASE_RETURN_STRING(MAC_AD_ERROR_NO_SUCH_ATTRIBUTE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_RECORD_TYPE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_ATTRIBUTE_TYPE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INSUFFICIENT_BUFFER);
        CASE_RETURN_STRING(MAC_AD_ERROR_IPC_FAILED);
        CASE_RETURN_STRING(MAC_AD_ERROR_NO_PROC_STATUS);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_PASSWORD_MISMATCH);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_PASSWORD_EXPIRED);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_CLOCK_SKEW);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_ERROR);
        CASE_RETURN_STRING(MAC_AD_ERROR_GSS_API_FAILED);
        CASE_RETURN_STRING(MAC_AD_ERROR_NO_SUCH_POLICY);
        CASE_RETURN_STRING(MAC_AD_ERROR_LDAP_OPEN);
        CASE_RETURN_STRING(MAC_AD_ERROR_LDAP_SET_OPTION);
        CASE_RETURN_STRING(MAC_AD_ERROR_LDAP_QUERY_FAILED);
        /* End macadutil errors */
        /* Begin Mac Directory Service errors */
        CASE_RETURN_STRING(eDSOpenFailed);
        CASE_RETURN_STRING(eDSCloseFailed);
        CASE_RETURN_STRING(eDSOpenNodeFailed);
        CASE_RETURN_STRING(eDSBadDirRefences);
        CASE_RETURN_STRING(eDSNullRecordReference);
        CASE_RETURN_STRING(eDSMaxSessionsOpen);
        CASE_RETURN_STRING(eDSCannotAccessSession);
        CASE_RETURN_STRING(eDSDirSrvcNotOpened);
        CASE_RETURN_STRING(eDSNodeNotFound);
        CASE_RETURN_STRING(eDSUnknownNodeName);
        CASE_RETURN_STRING(eDSRegisterCustomFailed);
        CASE_RETURN_STRING(eDSGetCustomFailed);
        CASE_RETURN_STRING(eDSUnRegisterFailed);
        CASE_RETURN_STRING(eDSAllocationFailed);
        CASE_RETURN_STRING(eDSDeAllocateFailed);
        CASE_RETURN_STRING(eDSCustomBlockFailed);
        CASE_RETURN_STRING(eDSCustomUnblockFailed);
        CASE_RETURN_STRING(eDSCustomYieldFailed);
        CASE_RETURN_STRING(eDSCorruptBuffer);
        CASE_RETURN_STRING(eDSInvalidIndex);
        CASE_RETURN_STRING(eDSIndexOutOfRange);
        CASE_RETURN_STRING(eDSIndexNotFound);
        CASE_RETURN_STRING(eDSCorruptRecEntryData);
        CASE_RETURN_STRING(eDSRefSpaceFull);
        CASE_RETURN_STRING(eDSRefTableAllocError);
        CASE_RETURN_STRING(eDSInvalidReference);
        CASE_RETURN_STRING(eDSInvalidRefType);
        CASE_RETURN_STRING(eDSInvalidDirRef);
        CASE_RETURN_STRING(eDSInvalidNodeRef);
        CASE_RETURN_STRING(eDSInvalidRecordRef);
        CASE_RETURN_STRING(eDSInvalidAttrListRef);
        CASE_RETURN_STRING(eDSInvalidAttrValueRef);
        CASE_RETURN_STRING(eDSInvalidContinueData);
        CASE_RETURN_STRING(eDSInvalidBuffFormat);
        CASE_RETURN_STRING(eDSInvalidPatternMatchType);
        CASE_RETURN_STRING(eDSRefTableError);
        CASE_RETURN_STRING(eDSRefTableNilError);
        CASE_RETURN_STRING(eDSRefTableIndexOutOfBoundsError);
        CASE_RETURN_STRING(eDSRefTableEntryNilError);
        CASE_RETURN_STRING(eDSRefTableCSBPAllocError);
        CASE_RETURN_STRING(eDSRefTableFWAllocError);
        CASE_RETURN_STRING(eDSAuthFailed);
        CASE_RETURN_STRING(eDSAuthMethodNotSupported);
        CASE_RETURN_STRING(eDSAuthResponseBufTooSmall);
        CASE_RETURN_STRING(eDSAuthParameterError);
        CASE_RETURN_STRING(eDSAuthInBuffFormatError);
        CASE_RETURN_STRING(eDSAuthNoSuchEntity);
        CASE_RETURN_STRING(eDSAuthBadPassword);
        CASE_RETURN_STRING(eDSAuthContinueDataBad);
        CASE_RETURN_STRING(eDSAuthUnknownUser);
        CASE_RETURN_STRING(eDSAuthInvalidUserName);
        CASE_RETURN_STRING(eDSAuthCannotRecoverPasswd);
        CASE_RETURN_STRING(eDSAuthFailedClearTextOnly);
        CASE_RETURN_STRING(eDSAuthNoAuthServerFound);
        CASE_RETURN_STRING(eDSAuthServerError);
        CASE_RETURN_STRING(eDSInvalidContext);
        CASE_RETURN_STRING(eDSBadContextData);
        CASE_RETURN_STRING(eDSPermissionError);
        CASE_RETURN_STRING(eDSReadOnly);
        CASE_RETURN_STRING(eDSInvalidDomain);
        CASE_RETURN_STRING(eNetInfoError);
        CASE_RETURN_STRING(eDSInvalidRecordType);
        CASE_RETURN_STRING(eDSInvalidAttributeType);
        CASE_RETURN_STRING(eDSInvalidRecordName);
        CASE_RETURN_STRING(eDSAttributeNotFound);
        CASE_RETURN_STRING(eDSRecordAlreadyExists);
        CASE_RETURN_STRING(eDSRecordNotFound);
        CASE_RETURN_STRING(eDSAttributeDoesNotExist);
        CASE_RETURN_STRING(eDSNoStdMappingAvailable);
        CASE_RETURN_STRING(eDSInvalidNativeMapping);
        CASE_RETURN_STRING(eDSSchemaError);
        CASE_RETURN_STRING(eDSAttributeValueNotFound);
        CASE_RETURN_STRING(eDSVersionMismatch);
        CASE_RETURN_STRING(eDSPlugInConfigFileError);
        CASE_RETURN_STRING(eDSInvalidPlugInConfigData);
        CASE_RETURN_STRING(eDSAuthNewPasswordRequired);
        CASE_RETURN_STRING(eDSAuthPasswordExpired);
        CASE_RETURN_STRING(eDSAuthPasswordQualityCheckFailed);
        CASE_RETURN_STRING(eDSAuthAccountDisabled);
        CASE_RETURN_STRING(eDSAuthAccountExpired);
        CASE_RETURN_STRING(eDSAuthAccountInactive);
        CASE_RETURN_STRING(eDSAuthPasswordTooShort);
        CASE_RETURN_STRING(eDSAuthPasswordTooLong);
        CASE_RETURN_STRING(eDSAuthPasswordNeedsLetter);
        CASE_RETURN_STRING(eDSAuthPasswordNeedsDigit);
        CASE_RETURN_STRING(eDSAuthPasswordChangeTooSoon);
        CASE_RETURN_STRING(eDSAuthInvalidLogonHours);
        CASE_RETURN_STRING(eDSAuthInvalidComputer);
        CASE_RETURN_STRING(eDSAuthMasterUnreachable);
        CASE_RETURN_STRING(eDSNullParameter);
        CASE_RETURN_STRING(eDSNullDataBuff);
        CASE_RETURN_STRING(eDSNullNodeName);
        CASE_RETURN_STRING(eDSNullRecEntryPtr);
        CASE_RETURN_STRING(eDSNullRecName);
        CASE_RETURN_STRING(eDSNullRecNameList);
        CASE_RETURN_STRING(eDSNullRecType);
        CASE_RETURN_STRING(eDSNullRecTypeList);
        CASE_RETURN_STRING(eDSNullAttribute);
        CASE_RETURN_STRING(eDSNullAttributeAccess);
        CASE_RETURN_STRING(eDSNullAttributeValue);
        CASE_RETURN_STRING(eDSNullAttributeType);
        CASE_RETURN_STRING(eDSNullAttributeTypeList);
        CASE_RETURN_STRING(eDSNullAttributeControlPtr);
        CASE_RETURN_STRING(eDSNullAttributeRequestList);
        CASE_RETURN_STRING(eDSNullDataList);
        CASE_RETURN_STRING(eDSNullDirNodeTypeList);
        CASE_RETURN_STRING(eDSNullAutMethod);
        CASE_RETURN_STRING(eDSNullAuthStepData);
        CASE_RETURN_STRING(eDSNullAuthStepDataResp);
        CASE_RETURN_STRING(eDSNullNodeInfoTypeList);
        CASE_RETURN_STRING(eDSNullPatternMatch);
        CASE_RETURN_STRING(eDSNullNodeNamePattern);
        CASE_RETURN_STRING(eDSNullTargetArgument);
        CASE_RETURN_STRING(eDSEmptyParameter);
        CASE_RETURN_STRING(eDSEmptyBuffer);
        CASE_RETURN_STRING(eDSEmptyNodeName);
        CASE_RETURN_STRING(eDSEmptyRecordName);
        CASE_RETURN_STRING(eDSEmptyRecordNameList);
        CASE_RETURN_STRING(eDSEmptyRecordType);
        CASE_RETURN_STRING(eDSEmptyRecordTypeList);
        CASE_RETURN_STRING(eDSEmptyRecordEntry);
        CASE_RETURN_STRING(eDSEmptyPatternMatch);
        CASE_RETURN_STRING(eDSEmptyNodeNamePattern);
        CASE_RETURN_STRING(eDSEmptyAttribute);
        CASE_RETURN_STRING(eDSEmptyAttributeType);
        CASE_RETURN_STRING(eDSEmptyAttributeTypeList);
        CASE_RETURN_STRING(eDSEmptyAttributeValue);
        CASE_RETURN_STRING(eDSEmptyAttributeRequestList);
        CASE_RETURN_STRING(eDSEmptyDataList);
        CASE_RETURN_STRING(eDSEmptyNodeInfoTypeList);
        CASE_RETURN_STRING(eDSEmptyAuthMethod);
        CASE_RETURN_STRING(eDSEmptyAuthStepData);
        CASE_RETURN_STRING(eDSEmptyAuthStepDataResp);
        CASE_RETURN_STRING(eDSEmptyPattern2Match);
        CASE_RETURN_STRING(eDSBadDataNodeLength);
        CASE_RETURN_STRING(eDSBadDataNodeFormat);
        CASE_RETURN_STRING(eDSBadSourceDataNode);
        CASE_RETURN_STRING(eDSBadTargetDataNode);
        CASE_RETURN_STRING(eDSBufferTooSmall);
        CASE_RETURN_STRING(eDSUnknownMatchType);
        CASE_RETURN_STRING(eDSUnSupportedMatchType);
        CASE_RETURN_STRING(eDSInvalDataList);
        CASE_RETURN_STRING(eDSAttrListError);
        CASE_RETURN_STRING(eServerNotRunning);
        CASE_RETURN_STRING(eUnknownAPICall);
        CASE_RETURN_STRING(eUnknownServerError);
        CASE_RETURN_STRING(eUnknownPlugIn);
        CASE_RETURN_STRING(ePlugInDataError);
        CASE_RETURN_STRING(ePlugInNotFound);
        CASE_RETURN_STRING(ePlugInError);
        CASE_RETURN_STRING(ePlugInInitError);
        CASE_RETURN_STRING(ePlugInNotActive);
        CASE_RETURN_STRING(ePlugInFailedToInitialize);
        CASE_RETURN_STRING(ePlugInCallTimedOut);
        CASE_RETURN_STRING(eNoSearchNodesFound);
        CASE_RETURN_STRING(eSearchPathNotDefined);
        CASE_RETURN_STRING(eNotHandledByThisNode);
        CASE_RETURN_STRING(eIPCSendError);
        CASE_RETURN_STRING(eIPCReceiveError);
        CASE_RETURN_STRING(eServerReplyError);
        CASE_RETURN_STRING(eDSTCPSendError);
        CASE_RETURN_STRING(eDSTCPReceiveError);
        CASE_RETURN_STRING(eDSTCPVersionMismatch);
        CASE_RETURN_STRING(eDSIPUnreachable);
        CASE_RETURN_STRING(eDSUnknownHost);
        CASE_RETURN_STRING(ePluginHandlerNotLoaded);
        CASE_RETURN_STRING(eNoPluginsLoaded);
        CASE_RETURN_STRING(ePluginAlreadyLoaded);
        CASE_RETURN_STRING(ePluginVersionNotFound);
        CASE_RETURN_STRING(ePluginNameNotFound);
        CASE_RETURN_STRING(eNoPluginFactoriesFound);
        CASE_RETURN_STRING(ePluginConfigAvailNotFound);
        CASE_RETURN_STRING(ePluginConfigFileNotFound);
        CASE_RETURN_STRING(eCFMGetFileSysRepErr);
        CASE_RETURN_STRING(eCFPlugInGetBundleErr);
        CASE_RETURN_STRING(eCFBndleGetInfoDictErr);
        CASE_RETURN_STRING(eCFDictGetValueErr);
        CASE_RETURN_STRING(eDSServerTimeout);
        CASE_RETURN_STRING(eDSContinue);
        CASE_RETURN_STRING(eDSInvalidHandle);
        CASE_RETURN_STRING(eDSSendFailed);
        CASE_RETURN_STRING(eDSReceiveFailed);
        CASE_RETURN_STRING(eDSBadPacket);
        CASE_RETURN_STRING(eDSInvalidTag);
        CASE_RETURN_STRING(eDSInvalidSession);
        CASE_RETURN_STRING(eDSInvalidName);
        CASE_RETURN_STRING(eDSUserUnknown);
        CASE_RETURN_STRING(eDSUnrecoverablePassword);
        CASE_RETURN_STRING(eDSAuthenticationFailed);
        CASE_RETURN_STRING(eDSBogusServer);
        CASE_RETURN_STRING(eDSOperationFailed);
        CASE_RETURN_STRING(eDSNotAuthorized);
        CASE_RETURN_STRING(eDSNetInfoError);
        CASE_RETURN_STRING(eDSContactMaster);
        CASE_RETURN_STRING(eDSServiceUnavailable);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr1);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr2);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr3);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr4);
        CASE_RETURN_STRING(eParameterSendError);
        CASE_RETURN_STRING(eParameterReceiveError);
        CASE_RETURN_STRING(eServerSendError);
        CASE_RETURN_STRING(eServerReceiveError);
        CASE_RETURN_STRING(eMemoryError);
        CASE_RETURN_STRING(eMemoryAllocError);
        CASE_RETURN_STRING(eServerError);
        CASE_RETURN_STRING(eParameterError);
        CASE_RETURN_STRING(eDataReceiveErr_NoDirRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrListRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrValueListRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrEntry);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrValueEntry);
        CASE_RETURN_STRING(eDataReceiveErr_NoNodeCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecEntry);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecEntryCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecMatchCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoDataBuff);
        CASE_RETURN_STRING(eDataReceiveErr_NoContinueData);
        CASE_RETURN_STRING(eDataReceiveErr_NoNodeChangeToken);
        CASE_RETURN_STRING(eNoLongerSupported);
        CASE_RETURN_STRING(eUndefinedError);
        CASE_RETURN_STRING(eNotYetImplemented);
        CASE_RETURN_STRING(eDSLastValue);
        /* End Mac Directory Service errors */
        /* Begin LSASS errors */
        CASE_RETURN_STRING(LW_ERROR_INVALID_CACHE_PATH);
        CASE_RETURN_STRING(LW_ERROR_INVALID_CONFIG_PATH);
        CASE_RETURN_STRING(LW_ERROR_INVALID_PREFIX_PATH);
        CASE_RETURN_STRING(LW_ERROR_INSUFFICIENT_BUFFER);
        CASE_RETURN_STRING(LW_ERROR_OUT_OF_MEMORY);
        CASE_RETURN_STRING(LW_ERROR_INVALID_MESSAGE);
        CASE_RETURN_STRING(LW_ERROR_UNEXPECTED_MESSAGE);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_USER);
        CASE_RETURN_STRING(LW_ERROR_DATA_ERROR);
        CASE_RETURN_STRING(LW_ERROR_NOT_IMPLEMENTED);
        CASE_RETURN_STRING(LW_ERROR_NO_CONTEXT_ITEM);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_GROUP);
        CASE_RETURN_STRING(LW_ERROR_REGEX_COMPILE_FAILED);
        CASE_RETURN_STRING(LW_ERROR_NSS_EDIT_FAILED);
        CASE_RETURN_STRING(LW_ERROR_NO_HANDLER);
        CASE_RETURN_STRING(LW_ERROR_INTERNAL);
        CASE_RETURN_STRING(LW_ERROR_NOT_HANDLED);
        CASE_RETURN_STRING(LW_ERROR_INVALID_DNS_RESPONSE);
        CASE_RETURN_STRING(LW_ERROR_DNS_RESOLUTION_FAILED);
        CASE_RETURN_STRING(LW_ERROR_FAILED_TIME_CONVERSION);
        CASE_RETURN_STRING(LW_ERROR_INVALID_SID);
        CASE_RETURN_STRING(LW_ERROR_PASSWORD_MISMATCH);
        CASE_RETURN_STRING(LW_ERROR_UNEXPECTED_DB_RESULT);
        CASE_RETURN_STRING(LW_ERROR_PASSWORD_EXPIRED);
        CASE_RETURN_STRING(LW_ERROR_ACCOUNT_EXPIRED);
        CASE_RETURN_STRING(LW_ERROR_USER_EXISTS);
        CASE_RETURN_STRING(LW_ERROR_GROUP_EXISTS);
        CASE_RETURN_STRING(LW_ERROR_INVALID_GROUP_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_USER_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_UNSUPPORTED_USER_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_UNSUPPORTED_GROUP_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LOGIN_ID);
        CASE_RETURN_STRING(LW_ERROR_INVALID_HOMEDIR);
        CASE_RETURN_STRING(LW_ERROR_INVALID_GROUP_NAME);
        CASE_RETURN_STRING(LW_ERROR_NO_MORE_GROUPS);
        CASE_RETURN_STRING(LW_ERROR_NO_MORE_USERS);
        CASE_RETURN_STRING(LW_ERROR_FAILED_ADD_USER);
        CASE_RETURN_STRING(LW_ERROR_FAILED_ADD_GROUP);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LSA_CONNECTION);
        CASE_RETURN_STRING(LW_ERROR_INVALID_AUTH_PROVIDER);
        CASE_RETURN_STRING(LW_ERROR_INVALID_PARAMETER);
        CASE_RETURN_STRING(LW_ERROR_LDAP_NO_PARENT_DN);
        CASE_RETURN_STRING(LW_ERROR_LDAP_ERROR);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_DOMAIN);
        CASE_RETURN_STRING(LW_ERROR_LDAP_FAILED_GETDN);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_DOMAINNAME);
        CASE_RETURN_STRING(LW_ERROR_KRB5_CALL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_GSS_CALL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_FAILED_FIND_DC);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_CELL);
        CASE_RETURN_STRING(LW_ERROR_GROUP_IN_USE);
        CASE_RETURN_STRING(LW_ERROR_FAILED_CREATE_HOMEDIR);
        CASE_RETURN_STRING(LW_ERROR_PASSWORD_TOO_WEAK);
        CASE_RETURN_STRING(LW_ERROR_INVALID_SID_REVISION);
        CASE_RETURN_STRING(LW_ERROR_ACCOUNT_LOCKED);
        CASE_RETURN_STRING(LW_ERROR_ACCOUNT_DISABLED);
        CASE_RETURN_STRING(LW_ERROR_USER_CANNOT_CHANGE_PASSWD);
        CASE_RETURN_STRING(LW_ERROR_LOAD_LIBRARY_FAILED);
        CASE_RETURN_STRING(LW_ERROR_LOOKUP_SYMBOL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_INVALID_EVENTLOG);
        CASE_RETURN_STRING(LW_ERROR_INVALID_CONFIG);
        CASE_RETURN_STRING(LW_ERROR_UNEXPECTED_TOKEN);
        CASE_RETURN_STRING(LW_ERROR_LDAP_NO_RECORDS_FOUND);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_USERNAME);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_GROUPNAME);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_CELLNAME);
        CASE_RETURN_STRING(LW_ERROR_STRING_CONV_FAILED);
        CASE_RETURN_STRING(LW_ERROR_INVALID_ACCOUNT);
        CASE_RETURN_STRING(LW_ERROR_INVALID_PASSWORD);
        CASE_RETURN_STRING(LW_ERROR_QUERY_CREATION_FAILED);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_OBJECT);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_USER_OR_GROUP);
        CASE_RETURN_STRING(LW_ERROR_INVALID_KRB5_CACHE_TYPE);
        CASE_RETURN_STRING(LW_ERROR_NOT_JOINED_TO_AD);
        CASE_RETURN_STRING(LW_ERROR_FAILED_TO_SET_TIME);
        CASE_RETURN_STRING(LW_ERROR_NO_NETBIOS_NAME);
        CASE_RETURN_STRING(LW_ERROR_INVALID_NETLOGON_RESPONSE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_OBJECTGUID);
        CASE_RETURN_STRING(LW_ERROR_INVALID_DOMAIN);
        CASE_RETURN_STRING(LW_ERROR_NO_DEFAULT_REALM);
        CASE_RETURN_STRING(LW_ERROR_NOT_SUPPORTED);
        CASE_RETURN_STRING(LW_ERROR_LOGON_FAILURE);
        CASE_RETURN_STRING(LW_ERROR_NO_SITE_INFORMATION);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LDAP_STRING);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LDAP_ATTR_VALUE);
        CASE_RETURN_STRING(LW_ERROR_NULL_BUFFER);
        CASE_RETURN_STRING(LW_ERROR_CLOCK_SKEW);
        CASE_RETURN_STRING(LW_ERROR_KRB5_NO_KEYS_FOUND);
        CASE_RETURN_STRING(LW_ERROR_SERVICE_NOT_AVAILABLE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_SERVICE_RESPONSE);
        CASE_RETURN_STRING(LW_ERROR_NSS_ERROR);
        CASE_RETURN_STRING(LW_ERROR_AUTH_ERROR);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LDAP_DN);
        CASE_RETURN_STRING(LW_ERROR_NOT_MAPPED);
        CASE_RETURN_STRING(LW_ERROR_RPC_NETLOGON_FAILED);
        CASE_RETURN_STRING(LW_ERROR_ENUM_DOMAIN_TRUSTS_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSABINDING_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_OPENPOLICY_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_SET_SESS_CREDS_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_REL_SESS_CREDS_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_CLOSEPOLICY_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES);
        CASE_RETURN_STRING(LW_ERROR_NO_TRUSTED_DOMAIN_FOUND);
        CASE_RETURN_STRING(LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS);
        CASE_RETURN_STRING(LW_ERROR_DCE_CALL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_FAILED_TO_LOOKUP_DC);
        CASE_RETURN_STRING(LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_USER_NAME);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LOG_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_METRIC_TYPE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_METRIC_PACK);
        CASE_RETURN_STRING(LW_ERROR_INVALID_METRIC_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK);
        CASE_RETURN_STRING(LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED);
        CASE_RETURN_STRING(LW_ERROR_LSA_SERVER_UNREACHABLE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_NSS_ARTEFACT_TYPE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_AGENT_VERSION);
        /* End LSASS errors */
        /* Begin Netlogon errors */
        CASE_RETURN_STRING(DNS_ERROR_BAD_PACKET);
        CASE_RETURN_STRING(ERROR_BAD_CONFIGURATION);
        CASE_RETURN_STRING(ERROR_BAD_DLL_ENTRYPOINT);
        CASE_RETURN_STRING(ERROR_BAD_FORMAT);
        CASE_RETURN_STRING(ERROR_DLL_INIT_FAILED);
        CASE_RETURN_STRING(ERROR_EVENTLOG_CANT_START);
        CASE_RETURN_STRING(ERROR_ILLEGAL_CHARACTER);
        CASE_RETURN_STRING(ERROR_INTERNAL_ERROR);
        CASE_RETURN_STRING(ERROR_INVALID_PARAMETER);
        CASE_RETURN_STRING(ERROR_INVALID_TIME);
        CASE_RETURN_STRING(ERROR_NO_SUCH_DOMAIN);
        CASE_RETURN_STRING(ERROR_NOT_FOUND);
        CASE_RETURN_STRING(ERROR_NOT_JOINED);
        CASE_RETURN_STRING(ERROR_PATH_NOT_FOUND);
        CASE_RETURN_STRING(ERROR_SERVICE_DEPENDENCY_FAIL);
        CASE_RETURN_STRING(ERROR_WRITE_FAULT);
        CASE_RETURN_STRING(NERR_DCNotFound);
        /* End Netlogon errors */

        DEFAULT_RETURN_UNKNOWN_STRING();
    }
}
