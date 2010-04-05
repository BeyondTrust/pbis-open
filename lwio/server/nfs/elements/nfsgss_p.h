/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nfsgss_p.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        GSS Support (Private Header)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#ifndef __NFSGSS_P_H__
#define __NFSGSS_P_H__

#define BAIL_ON_SEC_ERROR(ulMajorStatus) \
    if ((ulMajorStatus != GSS_S_COMPLETE)\
            && (ulMajorStatus != GSS_S_CONTINUE_NEEDED)) {\
        goto sec_error; \
    }

#define BAIL_ON_KRB_ERROR(ctx, ret)                                   \
    if (ret) {                                                        \
        if (ctx)  {                                                   \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);    \
            if (pszKrb5Error) {                                       \
                LWIO_LOG_ERROR("KRB5 Error at %s:%d: %s",              \
                        __FILE__,                                     \
                        __LINE__,                                     \
                        pszKrb5Error);                                \
                krb5_free_error_message(ctx, pszKrb5Error);           \
            }                                                         \
        } else {                                                      \
            LWIO_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",            \
                    __FILE__,                                         \
                    __LINE__,                                         \
                    ret);                                             \
        }                                                             \
        if (ret == KRB5KDC_ERR_KEY_EXP) {                             \
            ntStatus = LWIO_ERROR_PASSWORD_EXPIRED;                    \
        } else if (ret == KRB5_LIBOS_BADPWDMATCH) {                   \
            ntStatus = LWIO_ERROR_PASSWORD_MISMATCH;                   \
        } else if (ret == KRB5KRB_AP_ERR_SKEW) {                      \
            ntStatus = LWIO_ERROR_CLOCK_SKEW;                          \
        } else if (ret == ENOENT) {                                   \
            ntStatus = LWIO_ERROR_KRB5_NO_KEYS_FOUND;                  \
        } else {                                                      \
            ntStatus = LWIO_ERROR_KRB5_CALL_FAILED;                    \
        }                                                             \
        goto error;                                                   \
    }

#define NFS_PRINCIPAL       "not_defined_in_RFC4178@please_ignore"
#define NFS_KRB5_CACHE_PATH "MEMORY:lwio_krb5_cc"

typedef struct _NFS_KRB5_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    LONG             refcount;

    PSTR            pszCachePath;
    PSTR            pszMachinePrincipal;
    time_t          ticketExpiryTime;

} NFS_KRB5_CONTEXT, *PNFS_KRB5_CONTEXT;

typedef enum
{
    NFS_GSS_CONTEXT_STATE_INITIAL = 0,
    NFS_GSS_CONTEXT_STATE_HINTS,
    NFS_GSS_CONTEXT_STATE_NEGOTIATE,
    NFS_GSS_CONTEXT_STATE_COMPLETE
} NFS_GSS_CONTEXT_STATE;

typedef struct _NFS_GSS_NEGOTIATE_CONTEXT
{
    NFS_GSS_CONTEXT_STATE state;

    gss_ctx_id_t* pGssContext;

} NFS_GSS_NEGOTIATE_CONTEXT, *PNFS_GSS_NEGOTIATE_CONTEXT;

static
NTSTATUS
NfsGssNewContext(
    PNFS_HOST_INFO     pHostinfo,
    PNFS_KRB5_CONTEXT* ppContext
    );

static
NTSTATUS
NfsGssInitNegotiate(
    PNFS_KRB5_CONTEXT          pGssContext,
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate,
    PBYTE                      pSecurityInputBlob,
    ULONG                      ulSecurityInputBlobLen,
    PBYTE*                     ppSecurityOutputBlob,
    ULONG*                     pulSecurityOutputBloblen
    );

static
NTSTATUS
NfsGssContinueNegotiate(
    PNFS_KRB5_CONTEXT          pGssContext,
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate,
    PBYTE                      pSecurityInputBlob,
    ULONG                      ulSecurityInputBlobLen,
    PBYTE*                     ppSecurityOutputBlob,
    ULONG*                     pulSecurityOutputBloblen
    );

static
VOID
NfsGssFreeContext(
    PNFS_KRB5_CONTEXT pContext
    );

static
void
nfs_display_status(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    );

static
void
nfs_display_status_1(
    PCSTR     pszId,
    OM_uint32 code,
    int       type
    );

static
NTSTATUS
NfsGssRenew(
   PNFS_KRB5_CONTEXT pContext
   );

static
NTSTATUS
NfsGetTGTFromKeytab(
    PCSTR   pszUserName,
    PCSTR   pszPassword,
    PCSTR   pszCachePath,
    time_t* pGoodUntilTime
    );

static
NTSTATUS
NfsDestroyKrb5Cache(
    PCSTR pszCachePath
    );

static
NTSTATUS
NfsSetDefaultKrb5CachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    );

#endif /* __NFSGSS_P_H__ */
