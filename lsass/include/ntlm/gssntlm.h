/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        gssntlm.h
 *
 * Abstract:
 *
 *
 * Authors:
 *
 */

#ifndef __GSSNTLM_H__
#define __GSSNTLM_H__

#include <gssapi/gssapi.h>
#include <gssapi/gssapi_ext.h>
#include <gssapi/gssapi_krb5.h>

//******************************************************************************
//
// S T R U C T S
//

//******************************************************************************
//
// D E F I N E S
//

/* 1.3.6.1.4.1.311.2.2.10 */
#define GSS_MECH_NTLM       "\x2b\x06\x01\x04\x01\x82\x37\x02\x02\x0a"
#define GSS_MECH_NTLM_LEN   10

/* 1.3.6.1.4.1.27433.3.1 */
#define GSS_CRED_OPT_PW     "\x2b\x06\x01\x04\x01\x81\xd6\x29\x03\x01"
#define GSS_CRED_OPT_PW_LEN 10

/* 1.3.6.1.4.1.27433.3.2 */
#define GSS_CRED_OPT_DOMAIN "\x2b\x06\x01\x04\x01\x81\xd6\x29\x03\x02"
#define GSS_CRED_OPT_DOMAIN_LEN 10

#define GSS_C_QOP_DUMMY_SIG 1

//******************************************************************************
//
// E X T E R N S
//
extern gss_OID gGssNtlmOid;

//******************************************************************************
//
// P R O T O T Y P E S
//

OM_uint32
ntlm_gss_acquire_cred(
    OM_uint32* pMinorStatus,
    const gss_name_t pDesiredName,
    OM_uint32 nTimeReq,
    const gss_OID_set pDesiredMechs,
    gss_cred_usage_t CredUsage,
    gss_cred_id_t* pOutputCredHandle,
    gss_OID_set* pActualMechs,
    OM_uint32 *pTimeRec
    );

OM_uint32
ntlm_gss_release_cred(
    OM_uint32* pMinorStatus,
    gss_cred_id_t* pCredHandle
    );

OM_uint32
ntlm_gss_release_oid(
    OM_uint32* MinorStatus,
    gss_OID *pOid
    );

OM_uint32
ntlm_gss_init_sec_context(
    OM_uint32* pMinorStatus,
    const gss_cred_id_t InitiatorCredHandle,
    gss_ctx_id_t* pContextHandle,
    const gss_name_t pTargetName,
    const gss_OID pMechType,
    OM_uint32 nReqFlags,
    OM_uint32 nTimeReq,
    const gss_channel_bindings_t pInputChanBindings,
    const gss_buffer_t pInputToken,
    gss_OID* pActualMechType,
    gss_buffer_t pOutputToken,
    OM_uint32* pRetFlags,
    OM_uint32* pTimeRec
    );

OM_uint32
ntlm_gss_accept_sec_context(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t *pContextHandle,
    const gss_cred_id_t AcceptorCredHandle,
    const gss_buffer_t pInputTokenBuffer,
    const gss_channel_bindings_t pInputChanBindings,
    gss_name_t* pSrcName,
    gss_OID* pMechType,
    gss_buffer_t pOutputToken,
    OM_uint32* pRetFlags,
    OM_uint32* pTimeRec,
    gss_cred_id_t* pDelegatedCredHandle
    );

OM_uint32
ntlm_gss_delete_sec_context(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t* pContextHandle,
    gss_buffer_t OutputToken
    );

OM_uint32
ntlm_gss_display_name(
    OM_uint32* pMinorStatus,
    gss_name_t pGssName,
    gss_buffer_t pOutputName,
    gss_OID* ppNameType
    );

OM_uint32
ntlm_gss_get_mic(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_qop_t Qop,
    gss_buffer_t Message,
    gss_buffer_t Token
    );

OM_uint32
ntlm_gss_verify_mic(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_buffer_t Message,
    gss_buffer_t Token,
    gss_qop_t* pQop
    );

OM_uint32
ntlm_gss_wrap(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    INT nEncrypt,
    gss_qop_t Qop,
    gss_buffer_t InputMessage,
    PINT ActualQop,
    gss_buffer_t OutputMessage
    );

OM_uint32
ntlm_gss_wrap_iov(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    INT nEncrypt,
    gss_qop_t Qop,
    PINT pEncrypted,
    gss_iov_buffer_desc* pBuffers,
    int cBuffers
    );

OM_uint32
ntlm_gss_wrap_iov_length(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    INT nEncrypt,
    gss_qop_t Qop,
    PINT pEncrypted,
    gss_iov_buffer_desc* pBuffers,
    int cBuffers
    );

OM_uint32
ntlm_gss_unwrap(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_buffer_t InputMessage,
    gss_buffer_t OutMessage,
    PINT pEncrypted,
    gss_qop_t* pQop
    );

OM_uint32
ntlm_gss_unwrap_iov(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    PINT pEncrypted,
    gss_qop_t* pQop,
    gss_iov_buffer_desc* pBuffers,
    int cBuffers
    );

OM_uint32
ntlm_gss_display_status(
    OM_uint32* pMinorStatus,
    OM_uint32 dwConvertStatus,
    INT dwStatusType,
    gss_OID pMechType,
    OM_uint32* pdwContinueNeeded,
    gss_buffer_t pMsg
    );

OM_uint32
ntlm_gss_compare_oid(
    OM_uint32* pMinorStatus,
    const gss_OID a,
    const gss_OID b,
    BOOLEAN *bIsEqual
    );

OM_uint32
ntlm_gss_import_name(
    OM_uint32* pMinorStatus,
    const gss_buffer_t InputNameBuffer,
    const gss_OID InputNameType,
    gss_name_t* pOutputName
    );

OM_uint32
ntlm_gss_release_name(
    OM_uint32* pMinorStatus,
    gss_name_t* pName
    );

OM_uint32
ntlm_gss_inquire_cred(
    OM_uint32* pMinorStatus,
    const gss_cred_id_t CredHandle,
    gss_name_t* pName,
    OM_uint32* pLifeTime,
    gss_cred_usage_t* pCredUsage,
    gss_OID_set* pMechs
    );

OM_uint32
ntlm_gss_export_sec_context(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t* pContextHandle,
    gss_buffer_t InterprocessToken
    );

OM_uint32
ntlm_gss_import_sec_context(
    OM_uint32 * pMinorStatus,
    gss_buffer_t InterprocessToken,
    gss_ctx_id_t* pContextHandle
    );

OM_uint32
ntlm_gss_inquire_context(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_name_t* ppSourceName,
    gss_name_t* pTargetName,
    OM_uint32* pLifeTime,
    gss_OID* pMechType,
    OM_uint32* pCtxtFlags,
    PINT pLocal,
    PINT pOpen
    );

OM_uint32
ntlm_gss_inquire_sec_context_by_oid(
    OM_uint32* pMinorStatus,
    const gss_ctx_id_t GssCtxtHandle,
    const gss_OID Attrib,
    gss_buffer_set_t* ppBufferSet
    );

OM_uint32
ntlm_gssspi_set_cred_option(
    OM_uint32* pMinorStatus,
    gss_cred_id_t GssCredHandle,
    const gss_OID Option,
    const gss_buffer_t Buffer
    );

OM_uint32
ntlm_gss_get_name_attribute(
    OM_uint32* pMinorStatus,
    gss_name_t pName,
    gss_buffer_t pAttr,
    int* pAuthenticate,
    int* pComplete,
    gss_buffer_t pValue,
    gss_buffer_t pDisplayValue,
    int* pMore
    );

#endif /* __GSSNTLM_H__ */
