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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dnsdefines.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __DNS_DEFINES_H__
#define __DNS_DEFINES_H__

#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) -1)
#endif

#define DNS_TCP        1

#define DNS_UDP        2

#define DNS_OPCODE_UPDATE        1

// Use this for Win2K
#define DNS_GSS_ALGORITHM "gss.microsoft.com"
// Use this for WinXP and Win2003
//#define DNS_GSS_ALGORITHM "gss-tsig"

#ifndef CT_FIELD_OFFSET

#define CT_FIELD_OFFSET(Type, Field) \
    ((size_t)(&(((Type*)(0))->Field)))

#endif

#ifndef MAX_DNS_UDP_BUFFER_SIZE
#define MAX_DNS_UDP_BUFFER_SIZE 512
#endif

#ifndef DNS_MAX
#define DNS_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef DNS_MIN
#define DNS_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif


/*
 * Logging
 */
extern pthread_mutex_t       gLogLock;
extern LWDNSLogLevel         gLWDNSMaxLogLevel;
extern PFN_LWDNS_LOG_MESSAGE gpfnLWDNSLogger;

#define LWDNS_LOCK_LOGGER    pthread_mutex_lock(&gLogLock)
#define LWDNS_UNLOCK_LOGGER  pthread_mutex_unlock(&gLogLock)

#define _LWDNS_LOG_MESSAGE(Level, Format, ...) \
    DNSLogMessage(gpfnLWDNSLogger, Level, Format, ## __VA_ARGS__)


#define _LWDNS_LOG_WITH_THREAD(Level, Format, ...)	\
  _LWDNS_LOG_MESSAGE(Level,				\
                     "0x%lx:" Format,			\
                     (unsigned long)pthread_self(),	\
                     ## __VA_ARGS__)

#define _LWDNS_LOG_WITH_DEBUG(Level, Format, ...) \
    _LWDNS_LOG_WITH_THREAD(Level,                 \
                         "[%s() %s:%d] " Format,  \
                         __FUNCTION__,            \
                         __FILE__,                \
                         __LINE__,                \
                         ## __VA_ARGS__)

#define _LWDNS_LOG_IF(Level, Format, ...)                   \
    do {                                                    \
        LWDNS_LOCK_LOGGER;                                  \
        if (gpfnLWDNSLogger && (gLWDNSMaxLogLevel >= (Level)))     \
        {                                                   \
            if (gLWDNSMaxLogLevel >= LWDNS_LOG_LEVEL_DEBUG) \
            {                                               \
                _LWDNS_LOG_WITH_DEBUG(Level, Format, ## __VA_ARGS__); \
            }                                               \
            else                                            \
            {                                               \
                _LWDNS_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
            }                                               \
        }                                                   \
        LWDNS_UNLOCK_LOGGER;                                \
    } while (0)

#define LWDNS_LOG_ALWAYS(szFmt, ...) \
    _LWDNS_LOG_IF(LWDNS_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define LWDNS_LOG_ERROR(szFmt, ...) \
    _LWDNS_LOG_IF(LWDNS_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define LWDNS_LOG_WARNING(szFmt, ...) \
    _LWDNS_LOG_IF(LWDNS_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define LWDNS_LOG_INFO(szFmt, ...) \
    _LWDNS_LOG_IF(LWDNS_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define LWDNS_LOG_VERBOSE(szFmt, ...) \
    _LWDNS_LOG_IF(LWDNS_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define LWDNS_LOG_DEBUG(szFmt, ...) \
    _LWDNS_LOG_IF(LWDNS_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)

#define BAIL_ON_LWDNS_ERROR(dwError) \
    if (dwError){ \
        goto error; \
    }

#define BAIL_ON_HERRNO_ERROR(dwError) \
    if (dwError) \
    { \
        dwError = DNSMapHerrno(dwError); \
        goto error; \
    }

#ifdef WIN32
#define BAIL_ON_SEC_ERROR(dwMajorStatus) \
    if ((dwMajorStatus!= SEC_E_OK)\
            && (dwMajorStatus != SEC_I_CONTINUE_NEEDED)) {\
        goto sec_error; \
    }

#else

#define BAIL_ON_SEC_ERROR(dwMajorStatus) \
    if ((dwMajorStatus!= GSS_S_COMPLETE)\
            && (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {\
        goto sec_error; \
    }

#endif

#define BAIL_ON_LWDNS_KRB_ERROR(ctx, ret)                               \
    if (ret) {                                                          \
        if (ctx)  {                                                     \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);      \
            if (pszKrb5Error) {                                         \
                LWDNS_LOG_ERROR("KRB5 Error at %s:%d: %s",              \
                        __FILE__,                                       \
                        __LINE__,                                       \
                        pszKrb5Error);                                  \
                krb5_free_error_message(ctx, pszKrb5Error);             \
            }                                                           \
        } else {                                                        \
            LWDNS_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",            \
                    __FILE__,                                           \
                    __LINE__,                                           \
                    ret);                                               \
        }                                                               \
        if (ret == KRB5KDC_ERR_KEY_EXP) {                               \
            dwError = LWDNS_ERROR_PASSWORD_EXPIRED;                     \
        } else if (ret == KRB5_LIBOS_BADPWDMATCH) {                     \
            dwError = LWDNS_ERROR_PASSWORD_MISMATCH;                    \
        } else if (ret == KRB5KRB_AP_ERR_SKEW) {                        \
            dwError = LWDNS_ERROR_CLOCK_SKEW;                           \
        } else if (ret == ENOENT) {                                     \
            dwError = LWDNS_ERROR_KRB5_NO_KEYS_FOUND;                   \
        } else {                                                        \
            dwError = LWDNS_ERROR_KRB5_CALL_FAILED;                     \
        }                                                               \
        goto error;                                                     \
    }

#ifndef IsNullOrEmptyString

#define IsNullOrEmptyString(str) \
    (!(str) || !*(str))

#define IsEmptyString(str) (!*(str))

#endif

#define LWDNS_SAFE_FREE_STRING(str) \
    do {                            \
        if (str) {                  \
            DNSFreeString(str);     \
            (str) = NULL;           \
        }                           \
    } while(0)

#define LWDNS_SAFE_FREE_MEMORY(mem) \
        do {                        \
           if (mem) {               \
              DNSFreeMemory(mem);   \
              (mem) = NULL;         \
           }                        \
        } while(0);

/* DNS Class Types */

#define DNS_CLASS_IN        1

#define DNS_CLASS_ANY        255

#define DNS_CLASS_NONE        254

/* DNS RR Types */
#define DNS_RR_A            1


#define DNS_TCP_PORT        53
#define DNS_UDP_PORT        53


#define QTYPE_A               1// 1 a host address

#define QTYPE_NS              2 // 2 an authoritative name server

#define QTYPE_MD              3 // a mail destination (Obsolete - use MX)

#define QTYPE_CNAME              5 // CNAME

#define QTYPE_SOA              6 // SOA

#define QTYPE_PTR              12 // PTR

#define QTYPE_ANY              255

#define    QTYPE_TKEY              249

#define QTYPE_TSIG              250

/*
MF              4 a mail forwarder (Obsolete - use MX)

CNAME           5 the canonical name for an alias

SOA             6 marks the start of a zone of authority

MB              7 a mailbox domain name (EXPERIMENTAL)

MG              8 a mail group member (EXPERIMENTAL)

MR              9 a mail rename domain name (EXPERIMENTAL)

NULL            10 a null RR (EXPERIMENTAL)

WKS             11 a well known service description

PTR             12 a domain name pointer

HINFO           13 host information

MINFO           14 mailbox or mail list information

MX              15 mail exchange

TXT             16 text strings
*/

#define QR_QUERY     0x0000
#define QR_RESPONSE     0x0001

#define OPCODE_QUERY 0x00
#define OPCODE_IQUERY    0x01
#define OPCODE_STATUS    0x02

#define AA                1
#define RECURSION_DESIRED    0x01

#define RCODE_NOERROR                0               //No error condition

#define RCODE_FORMATERROR            1

#define RCODE_SERVER_FAILURE         2

#define RCODE_NAME_ERROR             3

#define RCODE_NOTIMPLEMENTED         4

#define RCODE_REFUSED                5


#define SENDBUFFER_SIZE                65536

#define RECVBUFFER_SIZE                65536



#define DNS_ONE_DAY_IN_SECS   86400
#define DNS_ONE_HOUR_IN_SECS  3600
#define DNS_TEN_HOURS_IN_SECS (10 * DNS_ONE_HOUR_IN_SECS)


#ifndef WIN32
#define SOCKET_ERROR             -1
#define INVALID_SOCKET            -1
#ifndef TRUE
#define TRUE                 1
#endif
#ifndef FALSE
#define FALSE                 0
#endif


#endif


#define  DNS_NO_ERROR                0

#define  DNS_FORMAT_ERROR            1

#define  DNS_SERVER_FAILURE            2

#define  DNS_NAME_ERROR                3

#define  DNS_NOT_IMPLEMENTED        4

#define  DNS_REFUSED                5

#endif /* __DNS_DEFINES_H__ */

