/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Likewise Software.  All rights reserved.
 *
 * Module Name:
 *
 *        gssntlm.c
 *
 * Abstract:
 *
 *        GSS wrapper functions for NTLM implementation.
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include <ntlm/sspintlm.h>
#include <ntlm/gssntlm.h>
#include <lwerror.h>
#include <lwmem.h>
#include <lwdef.h>
#include <stdarg.h>
#include <lwstr.h>
#include <string.h>
#include <assert.h>
#include <lwsecurityidentifier.h>
#include <lsautils.h>
#include <errno.h>

UINT32 ntlm_gss_duplicate_oid(
    UINT32 MinorStatus,
    const gss_OID_desc *const Oid,
    gss_OID *NewOid
    );

gss_OID_desc gGssNtlmOidDesc = {
    .length = GSS_MECH_NTLM_LEN,
    .elements = GSS_MECH_NTLM
    };

gss_OID gGssNtlmOid = &gGssNtlmOidDesc;

gss_OID_desc gGssCredOptionPasswordOidDesc = {
    .length = GSS_CRED_OPT_PW_LEN,
    .elements = GSS_CRED_OPT_PW
    };
gss_OID gGssCredOptionPasswordOid = &gGssCredOptionPasswordOidDesc;

gss_OID_desc gGssCredOptionDomainOidDesc = {
    .length = GSS_CRED_OPT_DOMAIN_LEN,
    .elements = GSS_CRED_OPT_DOMAIN
    };
gss_OID gGssCredOptionDomainOid = &gGssCredOptionDomainOidDesc;

//
// Since there is no GSSAPI mech plugin header, this must be kept in sync with
// gss_mechanism in krb5/src/lib/gssapi/mglueP.h.
//

typedef struct _GSS_MECH_CONFIG
{
    gss_OID_desc MechType;
    PVOID pContext;

    OM_uint32
    (*gss_acquire_cred)(
        OM_uint32*,
        gss_name_t,
        OM_uint32,
        gss_OID_set,
        INT,
        gss_cred_id_t*,
        gss_OID_set*,
        OM_uint32*
        );

    OM_uint32
    (*gss_release_cred)(
        OM_uint32*,
        gss_cred_id_t*
        );

    OM_uint32
    (*gss_init_sec_context)(
        OM_uint32*,
        gss_cred_id_t,
        gss_ctx_id_t*,
        gss_name_t,
        gss_OID,
        OM_uint32,
        OM_uint32,
        gss_channel_bindings_t,
        gss_buffer_t,
        gss_OID*,
        gss_buffer_t,
        OM_uint32*,
        OM_uint32*
        );

    OM_uint32
    (*gss_accept_sec_context)(
        OM_uint32*,
        gss_ctx_id_t*,
        gss_cred_id_t,
        gss_buffer_t,
        gss_channel_bindings_t,
        gss_name_t*,
        gss_OID*,
        gss_buffer_t,
        OM_uint32*,
        OM_uint32*,
        gss_cred_id_t*
        );

    OM_uint32
    (*gss_process_context_token)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_delete_sec_context)(
        OM_uint32*,
        gss_ctx_id_t*,
        gss_buffer_t
        );

    OM_uint32
    (*gss_context_time)(
        OM_uint32*,
        gss_ctx_id_t,
        OM_uint32*
        );

    OM_uint32
    (*gss_get_mic)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_qop_t,
        gss_buffer_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_verify_mic)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t,
        gss_buffer_t,
        gss_qop_t*
        );

    OM_uint32
    (*gss_wrap)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        gss_buffer_t,
        PINT,
        gss_buffer_t
        );

    OM_uint32
    (*gss_unwrap)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t,
        gss_buffer_t,
        PINT,
        gss_qop_t*
        );

    OM_uint32
    (*gss_display_status)(
        OM_uint32*,
        OM_uint32,
        INT,
        gss_OID,
        OM_uint32*,
        gss_buffer_t
        );

    OM_uint32
    (*gss_indicate_mechs)(
        OM_uint32*,
        gss_OID_set*
        );

    OM_uint32
    (*gss_compare_name)(
        OM_uint32*,
        gss_name_t,
        gss_name_t,
        PINT);

    OM_uint32
    (*gss_display_name)(
        OM_uint32*,
        gss_name_t,
        gss_buffer_t,
        gss_OID*
        );

    OM_uint32
    (*gss_import_name)(
        OM_uint32*,
        gss_buffer_t,
        gss_OID,
        gss_name_t*
        );

    OM_uint32
    (*gss_release_name)(
        OM_uint32*,
        gss_name_t*);

    OM_uint32
    (*gss_inquire_cred)(
        OM_uint32*,
        gss_cred_id_t,
        gss_name_t*,
        OM_uint32*,
        PINT,
        gss_OID_set*
        );

    OM_uint32
    (*gss_add_cred)(
        OM_uint32*,
        gss_cred_id_t,
        gss_name_t,
        gss_OID,
        gss_cred_usage_t,
        OM_uint32,
        OM_uint32,
        gss_cred_id_t*,
        gss_OID_set*,
        OM_uint32*,
        OM_uint32*
        );

    OM_uint32
    (*gss_export_sec_context)(
        OM_uint32*,
        gss_ctx_id_t*,
        gss_buffer_t
        );

    OM_uint32
    (*gss_import_sec_context)(
        OM_uint32*,
        gss_buffer_t,
        gss_ctx_id_t*
        );

    OM_uint32
    (*gss_inquire_cred_by_mech)(
        OM_uint32*,
        gss_cred_id_t,
        gss_OID,
        gss_name_t*,
        OM_uint32*,
        OM_uint32*,
        gss_cred_usage_t*
        );

    OM_uint32
    (*gss_inquire_names_for_mech)(
        OM_uint32,
        gss_OID,
        gss_OID_set*
        );

    OM_uint32
    (*gss_inquire_context)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_name_t*,
        gss_name_t*,
        OM_uint32*,
        gss_OID*,
        OM_uint32*,
        PINT,
        PINT
        );

    OM_uint32
    (*gss_internal_release_oid)(
        OM_uint32*,
        gss_OID*
        );

    OM_uint32
    (*gss_wrap_size_limit)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        OM_uint32,
        OM_uint32*
        );

    OM_uint32        (KRB5_CALLCONV *gss_localname)
        (
                    OM_uint32 *,        /* minor */
                    const gss_name_t,   /* name */
                    gss_const_OID,      /* mech_type */
                    gss_buffer_t /* localname */
            );
        OM_uint32               (KRB5_CALLCONV *gssspi_authorize_localname)
        (
                    OM_uint32 *,        /* minor_status */
                    const gss_name_t,   /* pname */
                    gss_const_buffer_t, /* local user */
                    gss_const_OID       /* local nametype */
        );


    OM_uint32
    (*gss_export_name)(
        OM_uint32*,
        const gss_name_t,
        gss_buffer_t
        );

        OM_uint32       (KRB5_CALLCONV *gss_duplicate_name)
        (
                    OM_uint32*,         /* minor_status */
                    const gss_name_t,   /* input_name */
                    gss_name_t *        /* output_name */
        /* */);

    OM_uint32
    (*gss_store_cred)(
        OM_uint32*,
        const gss_cred_id_t,
        gss_cred_usage_t,
        const gss_OID,
        OM_uint32,
        OM_uint32,
        gss_OID_set*,
        gss_cred_usage_t*
        );

    OM_uint32
    (*gss_inquire_sec_context_by_oid)(
        OM_uint32*,
        const gss_ctx_id_t,
        const gss_OID,
        gss_buffer_set_t*
        );

    OM_uint32
    (*gss_inquire_cred_by_oid)(
        OM_uint32*,
        const gss_cred_id_t,
        const gss_OID,
        gss_buffer_set_t*
        );

    OM_uint32
    (*gss_set_sec_context_option)(
        OM_uint32*,
        gss_ctx_id_t*,
        const gss_OID,
        const gss_buffer_t
        );

    OM_uint32
    (*gssspi_set_cred_option)(
        OM_uint32*,
        gss_cred_id_t,
        const gss_OID,
        const gss_buffer_t
        );

    OM_uint32
    (*gssspi_mech_invoke)(
        OM_uint32*,
        const gss_OID,
        const gss_OID,
        gss_buffer_t
        );

    OM_uint32
    (*gss_wrap_aead)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        gss_buffer_t,
        gss_buffer_t,
        PINT,
        gss_buffer_t
        );

    OM_uint32
    (*gss_unwrap_aead)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t,
        gss_buffer_t,
        gss_buffer_t,
        PINT,
        gss_qop_t*);

    OM_uint32
    (*gss_wrap_iov)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        PINT,
        gss_iov_buffer_desc*,
        INT
        );

    OM_uint32
    (*gss_unwrap_iov)(
        OM_uint32*,
        gss_ctx_id_t,
        PINT,
        gss_qop_t*,
        gss_iov_buffer_desc*,
        INT
        );

    OM_uint32
    (*gss_wrap_iov_length)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        PINT,
        gss_iov_buffer_desc*,
        INT
        );

    OM_uint32
    (*gss_complete_auth_token)(
        OM_uint32*,
        const gss_ctx_id_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_acquire_cred_impersonate_name)(
        OM_uint32 *,
        const gss_cred_id_t,
        const gss_name_t,
        OM_uint32,
        const gss_OID_set,
        gss_cred_usage_t,
        gss_cred_id_t *,
        gss_OID_set *,
        OM_uint32 *
        );

    OM_uint32
    (*gss_add_cred_impersonate_name)(
        OM_uint32 *,
        gss_cred_id_t,
        const gss_cred_id_t,
        const gss_name_t,
        const gss_OID,
        gss_cred_usage_t,
        OM_uint32,
        OM_uint32,
        gss_cred_id_t *,
        gss_OID_set *,
        OM_uint32 *,
        OM_uint32 *
        );

    OM_uint32
    (*gss_display_name_ext)(
        OM_uint32 *,
        gss_name_t,
        gss_OID,
        gss_buffer_t
        );

    OM_uint32
    (*gss_inquire_name)(
        OM_uint32 *,
        gss_name_t,
        int *,
        gss_OID *,
        gss_buffer_set_t *
        );
    
    OM_uint32
    (*gss_get_name_attribute)(
        OM_uint32 *,
        gss_name_t,
        gss_buffer_t,
        int *,
        int *,
        gss_buffer_t,
        gss_buffer_t,
        int *
        );

    OM_uint32
    (*gss_set_name_attribute)(
        OM_uint32 *,
        gss_name_t,
        int,
        gss_buffer_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_delete_name_attribute)(
        OM_uint32 *,
        gss_name_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_export_name_composite)(
        OM_uint32 *,
        gss_name_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_map_name_to_any)(
        OM_uint32 *,
        gss_name_t,
        int,
        gss_buffer_t,
        gss_any_t *
        );

    OM_uint32
    (*gss_release_any_name_mapping)(
        OM_uint32 *,
        gss_name_t,
        gss_buffer_t,
        gss_any_t *
        );    


        OM_uint32       (KRB5_CALLCONV *gss_pseudo_random)
        (
            OM_uint32 *,                /* minor_status */
            gss_ctx_id_t,               /* context */
            int,                        /* prf_key */
            const gss_buffer_t,         /* prf_in */
            ssize_t,                    /* desired_output_len */
            gss_buffer_t                /* prf_out */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gss_set_neg_mechs)
        (
            OM_uint32 *,                /* minor_status */
            gss_cred_id_t,              /* cred_handle */
            const gss_OID_set           /* mech_set */
        /* */);
        OM_uint32       (KRB5_CALLCONV *gss_inquire_saslname_for_mech)
        (
            OM_uint32 *,                /* minor_status */
            const gss_OID,              /* desired_mech */
            gss_buffer_t,               /* sasl_mech_name */
            gss_buffer_t,               /* mech_name */
            gss_buffer_t                /* mech_description */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gss_inquire_mech_for_saslname)
        (
            OM_uint32 *,                /* minor_status */
            const gss_buffer_t,         /* sasl_mech_name */
            gss_OID *                   /* mech_type */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gss_inquire_attrs_for_mech)
        (
            OM_uint32 *,                /* minor_status */
            gss_const_OID,              /* mech */
            gss_OID_set *,              /* mech_attrs */
            gss_OID_set *               /* known_mech_attrs */
        /* */);

        /* Credential store extensions */

        OM_uint32       (KRB5_CALLCONV *gss_acquire_cred_from)
        (
            OM_uint32 *,                /* minor_status */
            gss_name_t,                 /* desired_name */
            OM_uint32,                  /* time_req */
            gss_OID_set,                /* desired_mechs */
            gss_cred_usage_t,           /* cred_usage */
            gss_const_key_value_set_t,  /* cred_store */
            gss_cred_id_t *,            /* output_cred_handle */
            gss_OID_set *,              /* actual_mechs */
            OM_uint32 *                 /* time_rec */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gss_store_cred_into)
        (
            OM_uint32 *,                /* minor_status */
            gss_cred_id_t,              /* input_cred_handle */
            gss_cred_usage_t,           /* input_usage */
            gss_OID,                    /* desired_mech */
            OM_uint32,                  /* overwrite_cred */
            OM_uint32,                  /* default_cred */
            gss_const_key_value_set_t,  /* cred_store */
            gss_OID_set *,              /* elements_stored */
            gss_cred_usage_t *          /* cred_usage_stored */
        /* */);

       OM_uint32       (KRB5_CALLCONV *gssspi_acquire_cred_with_password)
        (
            OM_uint32 *,                /* minor_status */
            const gss_name_t,           /* desired_name */
            const gss_buffer_t,  /* password */
            OM_uint32,                  /* time_req */
            const gss_OID_set,          /* desired_mechs */
            int,                        /* cred_usage */
            gss_cred_id_t *,            /* output_cred_handle */
            gss_OID_set *,              /* actual_mechs */
            OM_uint32 *                 /* time_rec */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gss_export_cred)
        (
            OM_uint32 *,                /* minor_status */
            gss_cred_id_t,              /* cred_handle */
            gss_buffer_t                /* token */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gss_import_cred)
        (
                OM_uint32 *,            /* minor_status */
                gss_buffer_t,           /* token */
                gss_cred_id_t *         /* cred_handle */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gssspi_import_sec_context_by_mech)
        (
            OM_uint32 *,                /* minor_status */
            gss_OID,                    /* desired_mech */
            gss_buffer_t,               /* interprocess_token */
            gss_ctx_id_t *              /* context_handle */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gssspi_import_name_by_mech)
        (
            OM_uint32 *,                /* minor_status */
            gss_OID,                    /* mech_type */
            gss_buffer_t,               /* input_name_buffer */
            gss_OID,                    /* input_name_type */
            gss_name_t*                 /* output_name */
        /* */);

        OM_uint32       (KRB5_CALLCONV *gssspi_import_cred_by_mech)
        (
            OM_uint32 *,                /* minor_status */
            gss_OID,                    /* mech_type */
            gss_buffer_t,               /* token */
            gss_cred_id_t *             /* cred_handle */
        /* */);

} GSS_MECH_CONFIG, *PGSS_MECH_CONFIG;

typedef struct _NTLM_GSS_NAME
{
    // This always points to a static global structure
    gss_OID NameType;
    PSTR pszName;
    NTLM_CONTEXT_HANDLE hContext;
} NTLM_GSS_NAME, *PNTLM_GSS_NAME;

typedef struct _NTLM_GSS_CREDS
{
    PSTR pszUserName;
    DWORD fCredentialUse;
    TimeStamp tsExpiry;

    NTLM_CRED_HANDLE CredHandle;
} NTLM_GSS_CREDS, *PNTLM_GSS_CREDS;

//
// Prototypes
//

PGSS_MECH_CONFIG
gss_mech_initialize(
    void
    );

//
// Globals
//

static GSS_MECH_CONFIG gNtlmMech =
{
    {GSS_MECH_NTLM_LEN, GSS_MECH_NTLM},
    NULL,

    ntlm_gss_acquire_cred,
    ntlm_gss_release_cred,
    ntlm_gss_init_sec_context,
    ntlm_gss_accept_sec_context,
    NULL, //ntlm_gss_process_context_token,
    ntlm_gss_delete_sec_context,
    NULL, //ntlm_gss_context_time,
    ntlm_gss_get_mic,
    ntlm_gss_verify_mic,
    ntlm_gss_wrap,
    ntlm_gss_unwrap,
    ntlm_gss_display_status,
    NULL, //ntlm_gss_indicate_mechs,
    NULL, //ntlm_gss_compare_name,
    ntlm_gss_display_name,
    ntlm_gss_import_name,
    ntlm_gss_release_name,
    ntlm_gss_inquire_cred,
    NULL, //ntlm_gss_add_cred,
    ntlm_gss_export_sec_context,
    ntlm_gss_import_sec_context,
    NULL, //ntlm_gss_inquire_cred_by_mech,
    NULL, //ntlm_gss_inquire_names_for_mech,
    ntlm_gss_inquire_context,
    ntlm_gss_release_oid,
    NULL, //ntlm_gss_wrap_size_limit,
    NULL, // gss_localname
    NULL, // gssspi_authorize_localname
    NULL, // ntlm_gss_export_name,
    NULL, // gss_duplicate_name
    NULL, // ntlm_gss_store_cred,
    ntlm_gss_inquire_sec_context_by_oid,
    NULL, //ntlm_gss_inquire_cred_by_oid,
    NULL, //ntlm_gss_set_sec_context_option,
    ntlm_gssspi_set_cred_option,
    NULL, //ntlm_gssspi_mech_invoke,
    NULL, //ntlm_gss_wrap_aead,
    NULL, //ntlm_gss_unwrap_aead,
    ntlm_gss_wrap_iov,
    ntlm_gss_unwrap_iov,
    ntlm_gss_wrap_iov_length,
    NULL, //ntlm_gss_complete_auth_token,
    NULL, // gss_acquire_cred_impersonate_name
    NULL, // gss_add_cred_impersonate_name
    NULL, // gss_display_name_ext
    NULL, // gss_inquire_name
    .gss_get_name_attribute = ntlm_gss_get_name_attribute,
    NULL, // gss_set_name_attribute
    NULL, // gss_delete_name_attribute
    NULL, // gss_export_name_composite
    NULL, // gss_map_name_to_any
    NULL, // gss_release_any_name_mapping
    NULL, // gss_pseudo_random
    NULL, // gss_set_neg_mechs
    NULL, // gss_inquire_saslname_for_mech
    NULL, // gss_inquire_mech_for_saslname
    NULL, // gss_inquire_attrs_for_mech
    NULL, // gss_acquire_cred_from
    NULL, // gss_store_cred_into
    NULL, // gssspi_acquire_cred_with_password
    NULL, // gss_export_cred
    NULL, // gss_import_cred
    NULL, // gssspi_import_sec_context_by_mech
    NULL, // gssspi_import_name_by_mech
    NULL, // gssspi_import_cred_by_mech
};

//
// Function Definitions
//


OM_uint32
ntlm_gss_acquire_cred(
    OM_uint32* pMinorStatus,
    const gss_name_t hDesiredName,
    OM_uint32 nTimeReq,
    const gss_OID_set pDesiredMechs,
    gss_cred_usage_t CredUsage,
    gss_cred_id_t* pOutputCredHandle,
    gss_OID_set* pActualMechs,
    OM_uint32 *pTimeRec
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_GSS_CREDS pCreds = NULL;
    DWORD fCredentialUse = 0;
    PNTLM_GSS_NAME pDesiredName = (PNTLM_GSS_NAME)hDesiredName;
    BOOLEAN bIsUserName = TRUE;
    PCSTR pszName = NULL;
    OM_uint32 timeRec = (OM_uint32)-1;

    if (pActualMechs)
    {
        *pActualMechs = NULL;
    }

    if (pTimeRec)
    {
        *pTimeRec = 0;
    }

    switch(CredUsage)
    {
    case GSS_C_ACCEPT:
        fCredentialUse = NTLM_CRED_INBOUND;
        break;
    case GSS_C_INITIATE:
        fCredentialUse = NTLM_CRED_OUTBOUND;
        break;
    default:
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (pDesiredName)
    {
        MajorStatus = ntlm_gss_compare_oid(
                &MinorStatus,
                pDesiredName->NameType,
                GSS_C_NT_USER_NAME,
                &bIsUserName);
        BAIL_ON_LSA_ERROR(MinorStatus);

        if (!bIsUserName)
        {
            MajorStatus = GSS_S_BAD_NAMETYPE;
            MinorStatus = LW_ERROR_BAD_NAMETYPE;
            BAIL_ON_LSA_ERROR(MinorStatus);
        }

        pszName = pDesiredName->pszName;
    }


    MinorStatus = LwAllocateMemory(
        sizeof(*pCreds),
        OUT_PPVOID(&pCreds));
    BAIL_ON_LSA_ERROR(MinorStatus);

    MinorStatus = LwStrDupOrNull(
        pszName,
        &pCreds->pszUserName);
    BAIL_ON_LSA_ERROR(MinorStatus);

    pCreds->fCredentialUse = fCredentialUse;

    MinorStatus = NtlmClientAcquireCredentialsHandle(
        pCreds->pszUserName,
        "NTLM",
        pCreds->fCredentialUse,
        NULL,
        NULL,
        &pCreds->CredHandle,
        &pCreds->tsExpiry
        );
    if (MinorStatus == LW_ERROR_NO_CRED)
    {
        // Return a valid credentials handle anyway. Hopefully the caller will
        // call gssspi_set_cred_option for the credentials to be fully
        // initialized.
        MinorStatus = 0;
    }
    else
    {
        BAIL_ON_LSA_ERROR(MinorStatus);
        timeRec = pCreds->tsExpiry;
    }

    *pOutputCredHandle = (gss_cred_id_t)pCreds;

cleanup:
    *pMinorStatus = MinorStatus;

    if (pActualMechs)
    {
        *pActualMechs = NULL;
    }

    if (pTimeRec)
    {
        *pTimeRec = timeRec;
    }

    return MajorStatus;

error:
    *pOutputCredHandle = NULL;
    ntlm_gss_release_cred(
            NULL,
            (gss_cred_id_t*)&pCreds);

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    if (pTimeRec)
    {
        *pTimeRec = 0;
    }

    goto cleanup;
}

OM_uint32
ntlm_gss_release_cred(
    OM_uint32* pMinorStatus,
    gss_cred_id_t* pCredHandle
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_GSS_CREDS pCreds = NULL;

    if (!pCredHandle)
    {
        MajorStatus = GSS_S_NO_CRED;
        MinorStatus = LW_ERROR_NO_CRED;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    pCreds = (PNTLM_GSS_CREDS)*pCredHandle;

    if (!pCreds)
    {
        MajorStatus = GSS_S_NO_CRED;
        MinorStatus = LW_ERROR_NO_CRED;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }


    MinorStatus = NtlmClientFreeCredentialsHandle(
        &pCreds->CredHandle
        );
    // Ignore this error until all the data is freed. Be careful not to
    // overwrite MinorStatus.

    LW_SAFE_FREE_MEMORY(pCreds->pszUserName);

    LW_SAFE_FREE_MEMORY(pCreds);

    *pCredHandle = NULL;
    BAIL_ON_LSA_ERROR(MinorStatus);

cleanup:
    if (pMinorStatus)
    {
        *pMinorStatus = MinorStatus;
    }
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

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
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE hContext = NULL;
    NTLM_CONTEXT_HANDLE hNewContext = NULL;
    TimeStamp tsExpiry = 0;
    SecBufferDesc InputBuffer = {0};
    SecBufferDesc OutputBuffer = {0};
    SecBuffer InputToken = {0};
    SecBuffer OutputToken = {0};
    NTLM_CRED_HANDLE CredHandle = NULL;
    TimeStamp Expiry = 0;
    DWORD dwNtlmFlags = 0;
    DWORD dwOutNtlmFlags = 0;
    OM_uint32 RetFlags = 0;
    // Do not free
    SEC_CHAR *pTargetNameStr = NULL;

    InputBuffer.cBuffers = 1;
    InputBuffer.pBuffers = &InputToken;

    OutputBuffer.cBuffers = 1;
    OutputBuffer.pBuffers = &OutputToken;

    InputToken.BufferType = SECBUFFER_TOKEN;

    if (pInputToken)
    {
        InputToken.cbBuffer = pInputToken->length;
        InputToken.pvBuffer = pInputToken->value;
    }

    if (pContextHandle)
    {
        hContext = (NTLM_CONTEXT_HANDLE)*pContextHandle;
    }

    // if no credentials are passed in, create default creds
    if (nReqFlags & GSS_C_ANON_FLAG)
    {
        CredHandle = NULL;
    }
    else if (GSS_C_NO_CREDENTIAL == InitiatorCredHandle)
    {
        MinorStatus = NtlmClientAcquireCredentialsHandle(
            NULL,
            "NTLM",
            NTLM_CRED_OUTBOUND,
            NULL,
            NULL,
            &CredHandle,
            &Expiry
            );
        BAIL_ON_LSA_ERROR(MinorStatus);
    }
    else
    {
        CredHandle = ((PNTLM_GSS_CREDS)InitiatorCredHandle)->
            CredHandle;
        if (CredHandle == NULL)
        {
            // This means ntlm_gss_acquire_cred was called, but no default
            // credentials were available. The user should have called
            // ntlm_gssspi_set_cred_option to supply a password, but they did
            // not.

            MajorStatus = GSS_S_NO_CRED;
            MinorStatus = LW_ERROR_NO_CRED;
            BAIL_ON_LSA_ERROR(MinorStatus);
        }
    }

    // The server will ignore these flags and always perform signing and
    // sealing, but they are translated here for the sake of consistency.
    if (nReqFlags & GSS_C_INTEG_FLAG)
    {
        dwNtlmFlags |= ISC_REQ_INTEGRITY;
    }
    if (nReqFlags & GSS_C_CONF_FLAG)
    {
        dwNtlmFlags |= ISC_REQ_CONFIDENTIALITY;
    }
    if (nReqFlags & GSS_C_ANON_FLAG)
    {
        dwNtlmFlags |= ISC_REQ_NULL_SESSION;
    }
    if (nReqFlags & GSS_C_DCE_STYLE)
    {
        dwNtlmFlags |= ISC_REQ_USE_DCE_STYLE;
    }

    if (pTargetName)
    {
        pTargetNameStr = ((PNTLM_GSS_NAME)pTargetName)->pszName;
    }

    MinorStatus = NtlmClientInitializeSecurityContext(
        &CredHandle,
        &hContext,
        pTargetNameStr,
        dwNtlmFlags,
        0, // Reserved
        NTLM_NATIVE_DATA_REP,
        &InputBuffer,
        0, // Reserved
        &hNewContext,
        &OutputBuffer,
        &dwOutNtlmFlags,
        &tsExpiry
        );

    if (MinorStatus == LW_WARNING_CONTINUE_NEEDED)
    {
        MajorStatus = GSS_S_CONTINUE_NEEDED;
    }
    else
    {
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (dwOutNtlmFlags & ISC_RET_INTEGRITY)
    {
        RetFlags |= GSS_C_INTEG_FLAG;
    }
    if (dwOutNtlmFlags & ISC_RET_CONFIDENTIALITY)
    {
        RetFlags |= GSS_C_CONF_FLAG;
    }
    if (dwOutNtlmFlags & ISC_RET_NULL_SESSION)
    {
        RetFlags |= GSS_C_ANON_FLAG;
    }
    if (dwOutNtlmFlags & ISC_RET_USE_DCE_STYLE)
    {
        RetFlags |= GSS_C_DCE_STYLE;
    }

cleanup:
    *pMinorStatus = MinorStatus;

    if (GSS_C_NO_CREDENTIAL == InitiatorCredHandle && CredHandle)
    {
        NtlmClientFreeCredentialsHandle(&CredHandle);
    }

    if (pOutputToken)
    {
        pOutputToken->length = OutputToken.cbBuffer;
        pOutputToken->value = OutputToken.pvBuffer;
    }

    if (pActualMechType)
    {
        *pActualMechType = gGssNtlmOid;
    }

    if (pRetFlags)
    {
        *pRetFlags = RetFlags;
    }

    if (pTimeRec)
    {
        *pTimeRec = GSS_C_INDEFINITE;
    }

    if (pContextHandle)
    {
        *pContextHandle = (gss_ctx_id_t)hNewContext;
    }

    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_accept_sec_context(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t *pContextHandle,
    const gss_cred_id_t AcceptorCredHandle,
    const gss_buffer_t pInputTokenBuffer,
    const gss_channel_bindings_t pInputChanBindings,
    gss_name_t* ppSrcName,
    gss_OID* pMechType,
    gss_buffer_t pOutputToken,
    OM_uint32* pRetFlags,
    OM_uint32* pTimeRec,
    gss_cred_id_t* pDelegatedCredHandle
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    DWORD dwRetFlags = 0;
    DWORD dwReqFlags = 0;
    DWORD dwGssRetFlags = 0;
    SecBufferDesc InputBuffer = {0};
    SecBufferDesc OutputBuffer = {0};
    SecBuffer InputToken = {0};
    SecBuffer OutputToken = {0};
    TimeStamp tsExpiry = 0;
    gss_name_t pSrcName = NULL;
    NTLM_CONTEXT_HANDLE NewCtxtHandle = NULL;
    gss_cred_id_t LocalCreds = NULL;
    // Do not free
    gss_cred_id_t PassAcceptorCredHandle = NULL;

    *pMinorStatus = LW_ERROR_SUCCESS;

    if (ppSrcName)
    {
        *ppSrcName = NULL;
    }
    if (pMechType)
    {
        *pMechType = NULL;
    }
    if (pTimeRec)
    {
        *pTimeRec = 0;
    }
    if (pDelegatedCredHandle)
    {
        *pDelegatedCredHandle = NULL;
    }
    if (pRetFlags)
    {
        if (*pRetFlags & GSS_C_INTEG_FLAG)
        {
            dwReqFlags |= ASC_REQ_INTEGRITY;
        }
        if (*pRetFlags & GSS_C_CONF_FLAG)
        {
            dwReqFlags |= ASC_REQ_CONFIDENTIALITY;
        }
        if (*pRetFlags & GSS_C_ANON_FLAG)
        {
            dwReqFlags |= ASC_REQ_NULL_SESSION;
        }
    }

    if (AcceptorCredHandle)
    {
        PassAcceptorCredHandle = AcceptorCredHandle;
    }
    else
    {
        MajorStatus = ntlm_gss_acquire_cred(
            &MinorStatus,
            NULL,
            0,
            NULL,
            GSS_C_ACCEPT,
            &LocalCreds,
            NULL,
            NULL);
        BAIL_ON_LSA_ERROR(MinorStatus);
        PassAcceptorCredHandle = LocalCreds;
    }

    memset(pOutputToken, 0, sizeof(*pOutputToken));

    InputBuffer.cBuffers = 1;
    InputBuffer.pBuffers = &InputToken;

    OutputBuffer.cBuffers = 1;
    OutputBuffer.pBuffers = &OutputToken;

    InputToken.BufferType = SECBUFFER_TOKEN;
    InputToken.cbBuffer = pInputTokenBuffer->length;
    InputToken.pvBuffer = pInputTokenBuffer->value;

    MinorStatus = NtlmClientAcceptSecurityContext(
        &((PNTLM_GSS_CREDS)PassAcceptorCredHandle)->CredHandle,
        (PNTLM_CONTEXT_HANDLE)pContextHandle,
        &InputBuffer,
        dwReqFlags,
        NTLM_NATIVE_DATA_REP,
        &NewCtxtHandle,
        &OutputBuffer,
        &dwRetFlags,
        &tsExpiry);

    if (MinorStatus == LW_WARNING_CONTINUE_NEEDED)
    {
        MajorStatus = GSS_S_CONTINUE_NEEDED;
    }
    else
    {
        BAIL_ON_LSA_ERROR(MinorStatus);

        MajorStatus = ntlm_gss_inquire_context(
                          &MinorStatus,
                          (gss_ctx_id_t)NewCtxtHandle,
                          &pSrcName,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL
                          );
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

cleanup:

    *pMinorStatus = MinorStatus;

    if (pOutputToken)
    {
        pOutputToken->length = OutputToken.cbBuffer;
        pOutputToken->value = OutputToken.pvBuffer;
    }

    *pContextHandle = (gss_ctx_id_t)NewCtxtHandle;

    ntlm_gss_release_cred(
            NULL,
            &LocalCreds);

    if (pMechType)
    {
        *pMechType = gGssNtlmOid;
    }

    if (ppSrcName)
    {
        *ppSrcName = pSrcName;
    }
    else
    {
        ntlm_gss_release_name(NULL, &pSrcName);
    }

    if (pRetFlags)
    {
        if (dwRetFlags & ASC_RET_INTEGRITY)
        {
            dwGssRetFlags |= GSS_C_INTEG_FLAG;
        }
        if (dwRetFlags & ASC_RET_CONFIDENTIALITY)
        {
            dwGssRetFlags |= GSS_C_CONF_FLAG;
        }
        if (dwRetFlags & ASC_RET_NULL_SESSION)
        {
            dwGssRetFlags |= GSS_C_ANON_FLAG;
        }
        *pRetFlags = dwGssRetFlags;
    }

    if (pTimeRec)
    {
        *pTimeRec = GSS_C_INDEFINITE;
    }

    return MajorStatus;
error:

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    ntlm_gss_release_name(NULL, &pSrcName);

    goto cleanup;
}

OM_uint32
ntlm_gss_delete_sec_context(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t* pContextHandle,
    gss_buffer_t OutputToken
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = NULL;

    if (OutputToken)
    {
        OutputToken = GSS_C_NO_BUFFER;
    }

    if (!pContextHandle || !*pContextHandle)
    {
        MajorStatus = GSS_S_NO_CONTEXT;
        MinorStatus = LW_ERROR_NO_CONTEXT;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    ContextHandle = (NTLM_CONTEXT_HANDLE)*pContextHandle;

    MinorStatus = NtlmClientDeleteSecurityContext(
        &ContextHandle
        );

    BAIL_ON_LSA_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_get_mic(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_qop_t Qop,
    gss_buffer_t Message,
    gss_buffer_t Token
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecBufferDesc NtlmMessage = {0};
    SecBuffer NtlmBuffer[2];
    PBYTE pNtlmToken = NULL;
    SecPkgContext_Sizes spcSizes = {0};

    if(Qop != GSS_C_QOP_DEFAULT)
    {
        MajorStatus = GSS_S_BAD_QOP;
        BAIL_ON_LSA_ERROR(MajorStatus);
    }

    MinorStatus = NtlmClientQueryContextAttributes(
        &ContextHandle,
        SECPKG_ATTR_SIZES,
        &spcSizes
        );
    BAIL_ON_LSA_ERROR(MinorStatus);

    NtlmMessage.cBuffers = 2;
    NtlmMessage.pBuffers = NtlmBuffer;

    MinorStatus = LwAllocateMemory(
        spcSizes.cbMaxSignature,
        OUT_PPVOID(&pNtlmToken));
    BAIL_ON_LSA_ERROR(MinorStatus);

    NtlmBuffer[0].BufferType = SECBUFFER_DATA;
    NtlmBuffer[0].cbBuffer = Message->length;
    NtlmBuffer[0].pvBuffer = Message->value;

    NtlmBuffer[1].BufferType = SECBUFFER_TOKEN;
    NtlmBuffer[1].cbBuffer = spcSizes.cbMaxSignature;
    NtlmBuffer[1].pvBuffer = pNtlmToken;

    MinorStatus = NtlmClientMakeSignature(
        &ContextHandle,
        0,
        &NtlmMessage,
        0
        );
    BAIL_ON_LSA_ERROR(MinorStatus);

    Token->value = NtlmBuffer[1].pvBuffer;
    Token->length = NtlmBuffer[1].cbBuffer;

cleanup:
    *pMinorStatus = MinorStatus;

    return MajorStatus;

error:
    LW_SAFE_FREE_MEMORY(pNtlmToken);
    Token->value = NULL;
    Token->length = 0;

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_verify_mic(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_buffer_t Message,
    gss_buffer_t Token,
    gss_qop_t* pQop
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecBufferDesc NtlmMessage = {0};
    SecBuffer NtlmBuffer[2];
    PNTLM_SIGNATURE pSignature = NULL;
    DWORD dwQop = GSS_C_QOP_DEFAULT;

    NtlmMessage.cBuffers = 2;
    NtlmMessage.pBuffers = NtlmBuffer;

    NtlmBuffer[0].BufferType = SECBUFFER_DATA;
    NtlmBuffer[0].cbBuffer = Message->length;
    NtlmBuffer[0].pvBuffer = Message->value;

    NtlmBuffer[1].BufferType = SECBUFFER_TOKEN;
    NtlmBuffer[1].cbBuffer = Token->length;
    NtlmBuffer[1].pvBuffer = Token->value;

    MinorStatus = NtlmClientVerifySignature(
        &ContextHandle,
        &NtlmMessage,
        0,
        &dwQop
        );

    if (MinorStatus)
    {
        MajorStatus = GSS_S_BAD_SIG;
    }
    BAIL_ON_LSA_ERROR(MinorStatus);

    pSignature = (PNTLM_SIGNATURE)Token->value;

    if (pSignature->dwVersion == NTLM_VERSION &&
        pSignature->v1.encrypted.dwCounterValue == 0 &&
        pSignature->v1.encrypted.dwCrc32 == 0 &&
        pSignature->v1.encrypted.dwMsgSeqNum == 0)
    {
        dwQop = GSS_C_QOP_DUMMY_SIG;
    }

cleanup:
    if(pQop)
    {
        *pQop = (gss_qop_t)dwQop;
    }

    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:
    dwQop = GSS_C_QOP_DEFAULT;

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_wrap(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    INT nEncrypt,
    gss_qop_t Qop,
    gss_buffer_t InputMessage,
    PINT pEncrypted,
    gss_buffer_t OutputMessage
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecBufferDesc Message = {0};
    SecBuffer NtlmBuffer[2];
    SecPkgContext_Sizes Sizes = {0};
    DWORD dwBufferSize = 0;
    PBYTE pBuffer = NULL;
    INT nEncrypted = 0;

    Message.cBuffers = 2;
    Message.pBuffers = NtlmBuffer;

    memset(NtlmBuffer, 0, sizeof(SecBuffer) * Message.cBuffers);

    if(Qop != GSS_C_QOP_DEFAULT)
    {
        MajorStatus = GSS_S_BAD_QOP;
        BAIL_ON_LSA_ERROR(MajorStatus);
    }

    // Since every encrypted message gets a signature, we need to get the size
    MinorStatus = NtlmClientQueryContextAttributes(
        &ContextHandle,
        SECPKG_ATTR_SIZES,
        &Sizes
        );
    BAIL_ON_LSA_ERROR(MinorStatus);

    dwBufferSize += Sizes.cbMaxSignature;       // Token
    dwBufferSize += InputMessage->length;       // Data

    // We only need the padding for the duration of this operation, it should
    // not be returned with the rest of the data.

    dwBufferSize += Sizes.cbSecurityTrailer;    // Padding

    MinorStatus = LwAllocateMemory(dwBufferSize, OUT_PPVOID(&pBuffer));
    BAIL_ON_LSA_ERROR(MinorStatus);

    NtlmBuffer[0].BufferType = SECBUFFER_TOKEN;
    NtlmBuffer[0].cbBuffer = Sizes.cbMaxSignature;
    NtlmBuffer[0].pvBuffer = pBuffer;

    NtlmBuffer[1].BufferType = SECBUFFER_DATA;
    NtlmBuffer[1].cbBuffer = InputMessage->length;
    NtlmBuffer[1].pvBuffer =
        (PBYTE)NtlmBuffer[0].pvBuffer + NtlmBuffer[0].cbBuffer;

    memcpy(
        NtlmBuffer[1].pvBuffer,
        InputMessage->value,
        NtlmBuffer[1].cbBuffer);

    if (nEncrypt)
    {
        MinorStatus = NtlmClientEncryptMessage(
            &ContextHandle,
            TRUE,
            &Message,
            0);
    }
    else
    {
        MinorStatus = NtlmClientMakeSignature(
            &ContextHandle,
            Qop,
            &Message,
            0);
    }
    BAIL_ON_LSA_ERROR(MinorStatus);

    // As noted above, we'll trim the size down to exclude the padding
    dwBufferSize -= Sizes.cbSecurityTrailer;

    if (nEncrypt)
    {
        nEncrypted = 1;
    }

cleanup:
    OutputMessage->value = pBuffer;
    OutputMessage->length = dwBufferSize;

    if (pEncrypted)
    {
        *pEncrypted = nEncrypted;
    }

    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:
    LW_SAFE_FREE_MEMORY(pBuffer);
    dwBufferSize = 0;

    nEncrypted = 0;

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_wrap_iov(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    INT nEncrypt,
    gss_qop_t Qop,
    PINT pEncrypted,
    gss_iov_buffer_desc* pBuffers,
    int cBuffers
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecBufferDesc Message = {0};
    SecBuffer *NtlmBuffer = NULL;
    SecPkgContext_Sizes Sizes = {0};
    INT nEncrypted = 0;
    DWORD dwIndex = 0;
    BOOLEAN bFoundHeader = FALSE;

    if (cBuffers < 2)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    MinorStatus = LwAllocateMemory(
        cBuffers * sizeof(SecBuffer),
        OUT_PPVOID(&NtlmBuffer));
    BAIL_ON_LSA_ERROR(MinorStatus);

    Message.cBuffers = cBuffers;
    Message.pBuffers = NtlmBuffer;

    if (Qop != GSS_C_QOP_DEFAULT)
    {
        MajorStatus = GSS_S_BAD_QOP;
        BAIL_ON_LSA_ERROR(MajorStatus);
    }

    // Since every encrypted message gets a signature, we need to get the size
    MinorStatus = NtlmClientQueryContextAttributes(
        &ContextHandle,
        SECPKG_ATTR_SIZES,
        &Sizes
        );
    BAIL_ON_LSA_ERROR(MinorStatus);

    for (dwIndex = 0 ; dwIndex < cBuffers ; dwIndex++ )
    {
        if (GSS_IOV_BUFFER_FLAGS(pBuffers[dwIndex].type) & GSS_IOV_BUFFER_FLAG_ALLOCATE)
        {
            switch (GSS_IOV_BUFFER_TYPE(pBuffers[dwIndex].type))
            {
                case GSS_IOV_BUFFER_TYPE_HEADER:
                    pBuffers[dwIndex].buffer.length = Sizes.cbMaxSignature;
                    bFoundHeader = TRUE;
                    break;
                case GSS_IOV_BUFFER_TYPE_PADDING:
                    pBuffers[dwIndex].buffer.length = 0;
                    break;
                default:
                    MinorStatus = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(MinorStatus);
                    break;
            }

            if (pBuffers[dwIndex].buffer.length == 0)
            {
                pBuffers[dwIndex].buffer.value = NULL;
            }
            else
            {
                MinorStatus = LwAllocateMemory(
                    pBuffers[dwIndex].buffer.length,
                    OUT_PPVOID(&pBuffers[dwIndex].buffer.value));
                BAIL_ON_LSA_ERROR(MinorStatus);

                pBuffers[dwIndex].type |= GSS_IOV_BUFFER_FLAG_ALLOCATED;
            }
        }
        else
        {
            switch (GSS_IOV_BUFFER_TYPE(pBuffers[dwIndex].type))
            {
                case GSS_IOV_BUFFER_TYPE_HEADER:
                    NtlmBuffer[dwIndex].BufferType = SECBUFFER_TOKEN;
                    bFoundHeader = TRUE;
                    break;
                case GSS_IOV_BUFFER_TYPE_DATA:
                    NtlmBuffer[dwIndex].BufferType = SECBUFFER_DATA;
                    break;
                case GSS_IOV_BUFFER_TYPE_SIGN_ONLY:
                    NtlmBuffer[dwIndex].BufferType = SECBUFFER_DATA |
                                                     SECBUFFER_READONLY_WITH_CHECKSUM;
                    break;
                case GSS_IOV_BUFFER_TYPE_PADDING:
                    NtlmBuffer[dwIndex].BufferType = SECBUFFER_PADDING;
                    pBuffers[dwIndex].buffer.length = 0;
                    break;
                case GSS_IOV_BUFFER_TYPE_EMPTY:
                    NtlmBuffer[dwIndex].BufferType = SECBUFFER_EMPTY;
                    pBuffers[dwIndex].buffer.length = 0;
                    break;
                default:
                    MinorStatus = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(MinorStatus);
                    break;
            }
        }

        NtlmBuffer[dwIndex].cbBuffer = pBuffers[dwIndex].buffer.length;
        NtlmBuffer[dwIndex].pvBuffer = pBuffers[dwIndex].buffer.value;
    }

    if (!bFoundHeader)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (nEncrypt)
    {
        MinorStatus = NtlmClientEncryptMessage(
            &ContextHandle,
            TRUE,
            &Message,
            0
            );
    }
    else
    {
        MinorStatus = NtlmClientMakeSignature(
            &ContextHandle,
            Qop,
            &Message,
            0);
    }
    BAIL_ON_LSA_ERROR(MinorStatus);

    if (nEncrypt)
    {
        nEncrypted = 1;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(NtlmBuffer);

    if (pEncrypted)
    {
        *pEncrypted = nEncrypted;
    }

    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:

    for (dwIndex = 0 ; dwIndex < cBuffers ; dwIndex++ )
    {
        if (GSS_IOV_BUFFER_FLAGS(pBuffers[dwIndex].type) & GSS_IOV_BUFFER_FLAG_ALLOCATED)
        {
            LW_SAFE_FREE_MEMORY(pBuffers[dwIndex].buffer.value);
            pBuffers[dwIndex].type |= ~GSS_IOV_BUFFER_FLAG_ALLOCATED;
        }
    }

    nEncrypted = 0;

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_wrap_iov_length(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    INT nEncrypt,
    gss_qop_t Qop,
    PINT pEncrypted,
    gss_iov_buffer_desc* pBuffers,
    int cBuffers
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecPkgContext_Sizes Sizes = {0};
    DWORD dwIndex = 0;
    BOOLEAN bFoundHeader = FALSE;
    INT nEncrypted = 0;

    if (cBuffers < 2)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (Qop != GSS_C_QOP_DEFAULT)
    {
        MajorStatus = GSS_S_BAD_QOP;
        BAIL_ON_LSA_ERROR(MajorStatus);
    }

    // Since every encrypted message gets a signature, we need to get the size
    MinorStatus = NtlmClientQueryContextAttributes(
        &ContextHandle,
        SECPKG_ATTR_SIZES,
        &Sizes
        );
    BAIL_ON_LSA_ERROR(MinorStatus);

    for (dwIndex = 0 ; dwIndex < cBuffers ; dwIndex++ )
    {
        switch (GSS_IOV_BUFFER_TYPE(pBuffers[dwIndex].type))
        {
            case GSS_IOV_BUFFER_TYPE_HEADER:
                pBuffers[dwIndex].buffer.length = Sizes.cbMaxSignature;
                pBuffers[dwIndex].buffer.value = NULL;
                bFoundHeader = TRUE;
                break;
            case GSS_IOV_BUFFER_TYPE_PADDING:
                pBuffers[dwIndex].buffer.length = 0;
                pBuffers[dwIndex].buffer.value = NULL;
                break;
        }
    }

    if (!bFoundHeader)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (nEncrypt)
    {
        nEncrypted = 1;
    }

cleanup:

    if (pEncrypted)
    {
        *pEncrypted = nEncrypted;
    }
    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_unwrap(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_buffer_t InputMessage,
    gss_buffer_t OutputMessage,
    PINT pEncrypted,
    gss_qop_t* pQop
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecBufferDesc Message;
    SecBuffer NtlmBuffer[2];
    PVOID pBuffer = NULL;
    DWORD dwBufferSize = 0;
    BOOLEAN bEncrypted = FALSE;
    SecPkgContext_Sizes Sizes = {0};
    DWORD dwNtlmFlags = 0;
    DWORD dwQop = GSS_C_QOP_DEFAULT;

    LW_ASSERT(InputMessage);

    Message.cBuffers = 2;
    Message.pBuffers = NtlmBuffer;

    memset(NtlmBuffer, 0, sizeof(SecBuffer) * Message.cBuffers);

    // Since every encrypted message gets a signature, we need to get the size
    MinorStatus = NtlmClientQueryContextAttributes(
        &ContextHandle,
        SECPKG_ATTR_SIZES,
        &Sizes
        );
    BAIL_ON_LSA_ERROR(MinorStatus);

    LW_ASSERT(InputMessage->length >= Sizes.cbMaxSignature);

    // Check the negotiated flags to find out if the message is expected
    // to be encrypted or signed only.
    MinorStatus = NtlmClientQueryContextAttributes(
        &ContextHandle,
        SECPKG_ATTR_FLAGS,
        &dwNtlmFlags);
    BAIL_ON_LSA_ERROR(MinorStatus);

    // Here we are taking out the signature, but adding back in for the
    // padding.  The padding is only needed for the duration of the operation
    // and will be ignored afterwards.
    dwBufferSize =
        InputMessage->length - Sizes.cbMaxSignature + Sizes.cbSecurityTrailer;

    MinorStatus = LwAllocateMemory(
        dwBufferSize,
        OUT_PPVOID(&pBuffer));
    BAIL_ON_LSA_ERROR(MinorStatus);

    // Reduce the size to exclude the padding trailer
    dwBufferSize -= Sizes.cbSecurityTrailer;

    // Copy the input into our buffer... making sure to skip past the signature
    memcpy(
        pBuffer,
        (PBYTE)InputMessage->value + Sizes.cbMaxSignature,
        dwBufferSize);

    // We should be getting in a blob containing a signature and data... no size
    // information.  We place these values into a SECBUFFER_TOKEN and a
    // SECBUFFER_DATA (see ntlm_gss_wrap for how these get packaged
    // together).  Then we send them to the decryptor.
    NtlmBuffer[0].BufferType = SECBUFFER_TOKEN;
    NtlmBuffer[0].cbBuffer = Sizes.cbMaxSignature;
    NtlmBuffer[0].pvBuffer = InputMessage->value;

    NtlmBuffer[1].BufferType = SECBUFFER_DATA;
    NtlmBuffer[1].cbBuffer = dwBufferSize;
    NtlmBuffer[1].pvBuffer = pBuffer;

    if (dwNtlmFlags & ISC_RET_CONFIDENTIALITY)
    {
        MinorStatus = NtlmClientDecryptMessage(
            &ContextHandle,
            &Message,
            0,
            &bEncrypted
            );
    }
    else if (dwNtlmFlags & ISC_RET_INTEGRITY)
    {
        MinorStatus = NtlmClientVerifySignature(
            &ContextHandle,
            &Message,
            0,
            &dwQop
            );
    }
    else
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(MinorStatus);

    if (pQop)
    {
        *pQop = (gss_qop_t)dwQop;
    }

    OutputMessage->value  = pBuffer;
    OutputMessage->length = dwBufferSize;

cleanup:

    if (pEncrypted)
    {
        *pEncrypted = bEncrypted;
    }

    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:
    LW_SAFE_FREE_MEMORY(pBuffer);
    OutputMessage->value = NULL;
    OutputMessage->length = 0;

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_unwrap_iov(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    PINT pEncrypted,
    gss_qop_t* pQop,
    gss_iov_buffer_desc* pBuffers,
    int cBuffers
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecBufferDesc Message;
    SecBuffer *NtlmBuffer = NULL;
    BOOLEAN bEncrypted = FALSE;
    DWORD dwIndex = 0;
    BOOLEAN bFoundHeader = FALSE;
    DWORD dwNtlmFlags = 0;
    DWORD dwQop = GSS_C_QOP_DEFAULT;

    if (cBuffers < 2)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    MinorStatus = LwAllocateMemory(
        cBuffers * sizeof(SecBuffer),
        OUT_PPVOID(&NtlmBuffer));
    BAIL_ON_LSA_ERROR(MinorStatus);

    Message.cBuffers = cBuffers;
    Message.pBuffers = NtlmBuffer;

    for (dwIndex = 0 ; dwIndex < cBuffers ; dwIndex++)
    {    
        switch (GSS_IOV_BUFFER_TYPE(pBuffers[dwIndex].type))
        {
            case GSS_IOV_BUFFER_TYPE_HEADER:
                NtlmBuffer[dwIndex].BufferType = SECBUFFER_TOKEN;
                bFoundHeader = TRUE;
                break;
            case GSS_IOV_BUFFER_TYPE_DATA:
                NtlmBuffer[dwIndex].BufferType = SECBUFFER_DATA;
                break;
            case GSS_IOV_BUFFER_TYPE_SIGN_ONLY:
                NtlmBuffer[dwIndex].BufferType = SECBUFFER_DATA |
                                                 SECBUFFER_READONLY_WITH_CHECKSUM;
                break;
            case GSS_IOV_BUFFER_TYPE_PADDING:
                NtlmBuffer[dwIndex].BufferType = SECBUFFER_PADDING;
                break;
            case GSS_IOV_BUFFER_TYPE_EMPTY:
                NtlmBuffer[dwIndex].BufferType = SECBUFFER_EMPTY;
                break;
            default:
                MinorStatus = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(MinorStatus);
                break;
        }

        NtlmBuffer[dwIndex].cbBuffer = pBuffers[dwIndex].buffer.length;
        NtlmBuffer[dwIndex].pvBuffer = pBuffers[dwIndex].buffer.value;
    }

    if (!bFoundHeader)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    // Check the negotiated flags to find out if the message is expected
    // to be encrypted or signed only.
    MinorStatus = NtlmClientQueryContextAttributes(
        &ContextHandle,
        SECPKG_ATTR_FLAGS,
        &dwNtlmFlags);
    BAIL_ON_LSA_ERROR(MinorStatus);

    if (dwNtlmFlags & ISC_RET_CONFIDENTIALITY)
    {
        MinorStatus = NtlmClientDecryptMessage(
            &ContextHandle,
            &Message,
            0,
            &bEncrypted
            );
    }
    else if (dwNtlmFlags & ISC_RET_INTEGRITY)
    {
        MinorStatus = NtlmClientVerifySignature(
            &ContextHandle,
            &Message,
            0,
            &dwQop
            );
    }
    else
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(MinorStatus);

cleanup:

    LW_SAFE_FREE_MEMORY(NtlmBuffer);

    if (pEncrypted)
    {
        *pEncrypted = bEncrypted;
    }

    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_display_status(
    OM_uint32* pMinorStatus,
    OM_uint32 dwConvertStatus,
    INT dwStatusType,
    gss_OID pMechType,
    OM_uint32* pdwContinueNeeded,
    gss_buffer_t pMsg
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PCSTR pszCommonStr = NULL;

    if (pdwContinueNeeded)
    {
        *pdwContinueNeeded = 0;
    }
    if (pMechType != GSS_C_NULL_OID)
    {
        BOOLEAN bEqual = FALSE;
        MajorStatus = ntlm_gss_compare_oid(
                &MinorStatus,
                pMechType,
                gGssNtlmOid,
                &bEqual);
        BAIL_ON_LSA_ERROR(MinorStatus);

        if (!bEqual)
        {
            MinorStatus = LW_ERROR_BAD_MECH;
            MajorStatus = GSS_S_BAD_MECH;
            BAIL_ON_LSA_ERROR(MinorStatus);
        }
    }
    if (dwStatusType != GSS_C_MECH_CODE)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        MajorStatus = GSS_S_BAD_MECH;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }
    pszCommonStr = LwWin32ExtErrorToName(dwConvertStatus);
    if (!pszCommonStr)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        MajorStatus = GSS_S_UNAVAILABLE;
    }

    if (!pMsg)
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        MajorStatus = GSS_S_FAILURE;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    MinorStatus = LwAllocateString(
        pszCommonStr,
        (PSTR*)(PSTR)(&pMsg->value));
    BAIL_ON_LSA_ERROR(MinorStatus);

    pMsg->length = strlen(pszCommonStr);
    
cleanup:
    if (*pMinorStatus)
    {
        *pMinorStatus = MinorStatus;
    }
    return MajorStatus;

error:
    if (pMsg)
    {
        LW_SAFE_FREE_STRING(pMsg->value);
        pMsg->length = 0;
    }
    if (!MajorStatus)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_display_name(
    OM_uint32* pMinorStatus,
    gss_name_t pGssName,
    gss_buffer_t pOutputName,
    gss_OID* ppNameType
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_GSS_NAME pName = (PNTLM_GSS_NAME)pGssName;

    if (!pName)
    {
        MajorStatus = GSS_S_BAD_NAME;
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (pOutputName)
    {
        MinorStatus = LwAllocateString(
            pName->pszName,
            (PSTR*)(PSTR)(&pOutputName->value));
        BAIL_ON_LSA_ERROR(MinorStatus);

        pOutputName->length = strlen(pOutputName->value);
    }

    if (ppNameType)
    {
        // The caller assumes that *ppNameType is static read only memory
        *ppNameType = pName->NameType;
    }

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:
    if (pOutputName)
    {
        LW_SAFE_FREE_STRING(pOutputName->value);
        pOutputName->length = 0;
    }
    if (ppNameType)
    {
        *ppNameType = NULL;
    }
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_compare_oid(
    OM_uint32* pMinorStatus,
    const gss_OID a,
    const gss_OID b,
    BOOLEAN *bIsEqual
    )
{
    *pMinorStatus = 0;

    if (a->length != b->length)
    {
        *bIsEqual = FALSE;
        return 0;
    }

    if (!a->elements)
    {
        *bIsEqual = !b->elements;
        return 0;
    }

    *bIsEqual = !memcmp(a->elements, b->elements, a->length);
    return 0;
}

OM_uint32
ntlm_gss_import_name(
    OM_uint32* pMinorStatus,
    const gss_buffer_t InputNameBuffer,
    const gss_OID InputNameType,
    gss_name_t* pOutputName
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_GSS_NAME pResult = NULL;
    BOOLEAN bIsUserName = FALSE;
    BOOLEAN bIsHostService = FALSE;
    BOOLEAN bIsKrbPrinc = FALSE;

    MinorStatus = LwAllocateMemory(
        sizeof(*pResult),
        OUT_PPVOID(&pResult));
    BAIL_ON_LSA_ERROR(MinorStatus);

    ntlm_gss_compare_oid(
            &MinorStatus,
            InputNameType,
            GSS_C_NT_USER_NAME,
            &bIsUserName);
    BAIL_ON_LSA_ERROR(MinorStatus);
    ntlm_gss_compare_oid(
            &MinorStatus,
            InputNameType,
            GSS_C_NT_HOSTBASED_SERVICE,
            &bIsHostService);
    BAIL_ON_LSA_ERROR(MinorStatus);
    ntlm_gss_compare_oid(
            &MinorStatus,
            InputNameType,
            (gss_OID)GSS_KRB5_NT_PRINCIPAL_NAME,
            &bIsKrbPrinc);
    BAIL_ON_LSA_ERROR(MinorStatus);

    if (bIsUserName)
    {
        pResult->NameType = GSS_C_NT_USER_NAME;
    }
    else if(bIsHostService)
    {
        pResult->NameType = GSS_C_NT_HOSTBASED_SERVICE;
    }
    else if(bIsKrbPrinc)
    {
        pResult->NameType = (gss_OID)GSS_KRB5_NT_PRINCIPAL_NAME;
    }
    else
    {
        MajorStatus = GSS_S_BAD_NAMETYPE;
        MinorStatus = LW_ERROR_BAD_NAMETYPE;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    MinorStatus = LwStrndup(
        InputNameBuffer->value,
        InputNameBuffer->length,
        &pResult->pszName);
    BAIL_ON_LSA_ERROR(MinorStatus);

    *pOutputName = (gss_name_t)pResult;

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;

error:
    *pOutputName = NULL;
    ntlm_gss_release_name(NULL, (gss_name_t *)&pResult);
    goto cleanup;
}

OM_uint32
ntlm_gss_release_name(
    OM_uint32* pMinorStatus,
    gss_name_t* ppName
    )
{
    PNTLM_GSS_NAME pName = (PNTLM_GSS_NAME)*ppName;

    if (pName)
    {
        LW_SAFE_FREE_STRING(pName->pszName);
        LW_SAFE_FREE_MEMORY(pName);
        *ppName = NULL;
    }
    if (pMinorStatus)
    {
        *pMinorStatus = LW_ERROR_SUCCESS;
    }
    return GSS_S_COMPLETE;
}

OM_uint32
ntlm_gss_inquire_cred(
    OM_uint32* pMinorStatus,
    const gss_cred_id_t CredHandle,
    gss_name_t* pName,
    OM_uint32* pLifeTime,
    gss_cred_usage_t* pCredUsage,
    gss_OID_set* pMechs
    )
{
    if (pName)
    {
        *pName = GSS_C_NO_NAME;
    }
    if (pLifeTime)
    {
        *pLifeTime = 0;
    }
    if (pCredUsage)
    {
        *pCredUsage = 0;
    }
    if (pMechs)
    {
        *pMechs = GSS_C_NO_OID_SET;
    }

    return GSS_S_COMPLETE;
}

OM_uint32
ntlm_gss_export_sec_context(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t* pGssCtxtHandle,
    gss_buffer_t pInterprocessToken
   )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT_HANDLE pContextHandle = (PNTLM_CONTEXT_HANDLE) pGssCtxtHandle;
    SecBuffer Token = {0};

    MinorStatus = NtlmClientExportSecurityContext(
                      pContextHandle,
                      SECPKG_CONTEXT_EXPORT_DELETE_OLD,
                      &Token);
    BAIL_ON_LSA_ERROR(MinorStatus);

cleanup:

    *pMinorStatus = MinorStatus;
    
    if (pInterprocessToken)
    {
        pInterprocessToken->length = Token.cbBuffer;
        pInterprocessToken->value = Token.pvBuffer;
    }

    return MajorStatus;

error:

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    goto cleanup;
}

OM_uint32
ntlm_gss_import_sec_context(
    OM_uint32 *pMinorStatus,
    gss_buffer_t pInterprocessToken,
    gss_ctx_id_t* pGssCtxtHandle
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE NewContext =  NULL;
    SecBuffer Token = {0};

    Token.BufferType = SECBUFFER_TOKEN;
    Token.cbBuffer = pInterprocessToken->length;
    Token.pvBuffer = pInterprocessToken->value;

    MinorStatus = NtlmClientImportSecurityContext(
                      &Token,
                      &NewContext);
    BAIL_ON_LSA_ERROR(MinorStatus);

cleanup:

    *pMinorStatus = MinorStatus;

    *pGssCtxtHandle = (gss_ctx_id_t)NewContext;    

    return MajorStatus;

error:

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_context(
    OM_uint32* pMinorStatus,
    gss_ctx_id_t GssCtxtHandle,
    gss_name_t* ppSourceName,
    gss_name_t* ppTargetName,
    OM_uint32* pLifeTime,
    gss_OID* pMechType,
    OM_uint32* pCtxtFlags,
    PINT pLocal,
    PINT pOpen
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE NtlmCtxtHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    DWORD dwNtlmFlags = 0;
    SecPkgContext_Names Names = { 0 };
    gss_name_t pSourceName = NULL;
    PNTLM_GSS_NAME pUserName = NULL;

    if (pLocal || pOpen)
    {
        MinorStatus = LW_ERROR_NOT_SUPPORTED;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (pCtxtFlags)
    {
        MinorStatus = NtlmClientQueryContextAttributes(
            &NtlmCtxtHandle,
            SECPKG_ATTR_FLAGS,
            &dwNtlmFlags);
        BAIL_ON_LSA_ERROR(MinorStatus);

        if (dwNtlmFlags & ISC_RET_INTEGRITY)
        {
            *pCtxtFlags |= GSS_C_INTEG_FLAG;
        }

        if (dwNtlmFlags & ISC_RET_CONFIDENTIALITY)
        {
            *pCtxtFlags |= GSS_C_CONF_FLAG;
        }
    }

    if (ppTargetName)
    {
        *ppTargetName = GSS_C_NO_NAME;
    }

    if (ppSourceName)
    {
        MinorStatus = NtlmClientQueryContextAttributes(
            &NtlmCtxtHandle,
            SECPKG_ATTR_NAMES,
            &Names);
        BAIL_ON_LSA_ERROR(MinorStatus);

        MinorStatus = LwAllocateMemory(
            sizeof(*pUserName),
            OUT_PPVOID(&pUserName));
        BAIL_ON_LSA_ERROR(MinorStatus);

        pUserName->NameType = GSS_C_NT_USER_NAME;
        pUserName->hContext = NtlmCtxtHandle;

        MinorStatus = LwAllocateString(
            Names.pUserName,
            (PSTR*)(PSTR)(&pUserName->pszName));
        BAIL_ON_LSA_ERROR(MinorStatus);

        pSourceName = (gss_name_t)pUserName;
        pUserName = NULL;
    }

cleanup:

    *pMinorStatus = MinorStatus;

    if(Names.pUserName)
    {
        NtlmFreeContextBuffer(Names.pUserName);
    }

    if (pUserName)
    {
        ntlm_gss_release_name(NULL, (gss_name_t *)&pUserName);
    }

    if (ppSourceName)
    {
        *ppSourceName = pSourceName;
    }

    if (pLifeTime)
    {
        *pLifeTime = GSS_C_INDEFINITE;
    }

    if (pMechType)
    {
        *pMechType = gGssNtlmOid;
    }

    return MajorStatus;

error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    ntlm_gss_release_name(NULL, &pSourceName);

    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_sec_context_by_oid(
    OM_uint32* pMinorStatus,
    const gss_ctx_id_t GssCtxtHandle,
    const gss_OID Attrib,
    gss_buffer_set_t* ppBufferSet
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    gss_OID SessionKeyOid = GSS_C_INQ_SSPI_SESSION_KEY;
    gss_OID NamesOid = GSS_C_NT_STRING_UID_NAME;
    NTLM_CONTEXT_HANDLE NtlmCtxtHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecPkgContext_SessionKey SessionKey = {0};
    SecPkgContext_Names Names = {0};
    gss_buffer_set_t pBufferSet = NULL;
    gss_buffer_t pBuffer = NULL;

    MinorStatus = LwAllocateMemory(
        sizeof(*pBufferSet),
        OUT_PPVOID(&pBufferSet));
    BAIL_ON_LSA_ERROR(MinorStatus);

    MinorStatus = LwAllocateMemory(
        sizeof(*pBuffer),
        OUT_PPVOID(&pBuffer));
    BAIL_ON_LSA_ERROR(MinorStatus);

    if (Attrib->length == SessionKeyOid->length &&
        !memcmp(Attrib->elements, SessionKeyOid->elements, Attrib->length))
    {
        MinorStatus = NtlmClientQueryContextAttributes(
            &NtlmCtxtHandle,
            SECPKG_ATTR_SESSION_KEY,
            &SessionKey);
        BAIL_ON_LSA_ERROR(MinorStatus);

        pBuffer->value = SessionKey.pSessionKey;
        pBuffer->length = SessionKey.SessionKeyLength;
    }
    else if (Attrib->length == NamesOid->length &&
        !memcmp(Attrib->elements, NamesOid->elements, Attrib->length))
    {
        MinorStatus = NtlmClientQueryContextAttributes(
            &NtlmCtxtHandle,
            SECPKG_ATTR_NAMES,
            &Names);
        BAIL_ON_LSA_ERROR(MinorStatus);

        pBuffer->value = Names.pUserName;
        pBuffer->length = strlen(pBuffer->value);
    }
    else
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    pBufferSet->count = 1;
    pBufferSet->elements = pBuffer;

cleanup:
    *pMinorStatus = MinorStatus;
    *ppBufferSet = pBufferSet;

    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    LW_SAFE_FREE_MEMORY(pBuffer);
    LW_SAFE_FREE_MEMORY(pBufferSet);

    goto cleanup;
}

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
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_GSS_NAME pUserName = (PNTLM_GSS_NAME) pName;

    if (pMore && *pMore != -1)
    {
        MinorStatus = ERROR_NO_MORE_ITEMS;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (!strncmp("urn:mspac:logon-info", (char*) pAttr->value, pAttr->length))
    {
        if (pValue)
        {
            SecPkgContext_PacLogonInfo LogonInfo = {0};

            MinorStatus = NtlmClientQueryContextAttributes(
                &pUserName->hContext,
                SECPKG_ATTR_PAC_LOGON_INFO,
                &LogonInfo);
            BAIL_ON_LSA_ERROR(MinorStatus);
            
            pValue->value = LogonInfo.pLogonInfo;
            pValue->length = LogonInfo.LogonInfoLength;
        }
    }
    else if (!strncmp("urn:likewise:mapped-to-guest", (char*) pAttr->value,
                      pAttr->length))
    {
        if (pValue)
        {
            SecPkgContext_MappedToGuest mappedToGuest = { 0 };

            MinorStatus = NtlmClientQueryContextAttributes(
                            &pUserName->hContext,
                            SECPKG_ATTR_CUSTOM_MAPPED_TO_GUEST,
                            &mappedToGuest);
            BAIL_ON_LSA_ERROR(MinorStatus);

            MinorStatus = LwAllocateMemory(
                            sizeof(mappedToGuest.MappedToGuest),
                            OUT_PPVOID(&pValue->value));
            BAIL_ON_LSA_ERROR(MinorStatus);

            memcpy(pValue->value, &mappedToGuest.MappedToGuest,
                   sizeof(mappedToGuest.MappedToGuest));

            pValue->length = sizeof(mappedToGuest.MappedToGuest);
        }
    }
    else
    {
        MinorStatus = LW_ERROR_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

    if (pAuthenticate)
    {
        *pAuthenticate = 1;
    }
    
    if (pComplete)
    {
        *pComplete = 1;
    }

    if (pMore)
    {
        *pMore = 0;
    }

cleanup:

    *pMinorStatus = MinorStatus;

    return MajorStatus;

error:

    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    goto cleanup;
}

PGSS_MECH_CONFIG
gss_mech_initialize(void)
{
    return &gNtlmMech;
}

OM_uint32
ntlm_gssspi_set_cred_option(
    OM_uint32* pMinorStatus,
    gss_cred_id_t GssCredHandle,
    const gss_OID OptionOid,
    const gss_buffer_t Buffer
    )
{
    OM_uint32 MajorStatus = GSS_S_UNAVAILABLE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_GSS_CREDS pCreds = (PNTLM_GSS_CREDS)GssCredHandle;
    PSEC_WINNT_AUTH_IDENTITY pAuthData = NULL;
    PSTR pszDomain = NULL;
    SecPkgCred_DomainName spcDomainName = {0};

    if (OptionOid->length == gGssCredOptionPasswordOid->length &&
        !memcmp(
            OptionOid->elements,
            gGssCredOptionPasswordOid->elements,
            OptionOid->length))
    {
        MajorStatus = GSS_S_COMPLETE;

        if (!Buffer || Buffer->length != sizeof(SEC_WINNT_AUTH_IDENTITY) ||
                !Buffer->value)
        {
            MinorStatus = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(MinorStatus);
        }

        pAuthData = (PSEC_WINNT_AUTH_IDENTITY)Buffer->value;

        if (pCreds->CredHandle)
        {
            MinorStatus = NtlmClientFreeCredentialsHandle(
                &pCreds->CredHandle
                );
            BAIL_ON_LSA_ERROR(MinorStatus);
        }

        MinorStatus = NtlmClientAcquireCredentialsHandle(
            pCreds->pszUserName,
            "NTLM",
            pCreds->fCredentialUse,
            NULL,
            pAuthData,
            &pCreds->CredHandle,
            &pCreds->tsExpiry
            );
        BAIL_ON_LSA_ERROR(MinorStatus);
    }
    else if (OptionOid->length == gGssCredOptionDomainOid->length &&
        !memcmp(
            OptionOid->elements,
            gGssCredOptionDomainOid->elements,
            OptionOid->length))
    {
        MajorStatus = GSS_S_COMPLETE;

        if (!Buffer || !Buffer->length || !Buffer->value)
        {
            MinorStatus = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(MinorStatus);
        }

        MinorStatus = LwStrndup(
                          Buffer->value,
                          Buffer->length,
                          &pszDomain);
        BAIL_ON_LSA_ERROR(MinorStatus);

        spcDomainName.pName = pszDomain;

        MinorStatus = NtlmClientSetCredentialsAttributes(
                          &pCreds->CredHandle,
                          SECPKG_CRED_ATTR_DOMAIN_NAME,
                          &spcDomainName);
        BAIL_ON_LSA_ERROR(MinorStatus);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszDomain);

    *pMinorStatus = MinorStatus;

    return MajorStatus;

error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    goto cleanup;
}

OM_uint32
ntlm_gss_release_oid(
    OM_uint32* MinorStatus,
    gss_OID *pOid
    )
{
    /* Don't free the global static OID */

    if ((pOid &&
         ((*pOid == gGssNtlmOid) || (*pOid == GSS_C_NO_OID))))
    {

        *MinorStatus = 0;
        return GSS_S_COMPLETE;
    }

    return GSS_S_FAILURE;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
