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

size_t
LwGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t sRequiredLen = 0;
    PCSTR pszDesc = LwWin32ExtErrorToDescription(dwError);

    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }

    if (!pszDesc)
    {
        pszDesc = "Unknown error";
    }

    sRequiredLen = strlen(pszDesc) + 1;
    if (stBufSize >= sRequiredLen)
    {
        strcpy(pszBuffer, pszDesc);
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
            LW_RTL_LOG_ERROR("Unable to map errno %d", dwErrno);
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
            LW_RTL_LOG_ERROR("Unable to map h_errno %d", dwHErrno);
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
            LW_RTL_LOG_ERROR("Unable to map ldap error %d", dwErr);
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
            LW_RTL_LOG_ERROR("Unable to map lwmsg status %d", status);
            return LW_ERROR_INTERNAL;
    }
}
