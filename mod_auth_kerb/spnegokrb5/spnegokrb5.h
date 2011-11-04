#ifndef _SPNEGOKRB5_H_
#define _SPNEGOKRB5_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "config.h"
#ifdef HEIMDAL
#  include <gssapi.h>
#else
#  include <gssapi/gssapi.h>
#endif

#ifndef KRB5_LIB_FUNCTION
#  if defined(_WIN32)
#    define KRB5_LIB_FUNCTION _stdcall
#  else
#    define KRB5_LIB_FUNCTION
#  endif
#endif

OM_uint32 KRB5_LIB_FUNCTION gss_init_sec_context_spnego(
            OM_uint32 *,
            const gss_cred_id_t,
            gss_ctx_id_t *,
            const gss_name_t,
            const gss_OID,
            OM_uint32,
            OM_uint32,
            const gss_channel_bindings_t,
            const gss_buffer_t,
            gss_OID *,
            gss_buffer_t,
            OM_uint32 *,
            OM_uint32 *);

OM_uint32 KRB5_LIB_FUNCTION gss_accept_sec_context_spnego
           (OM_uint32 *,
            gss_ctx_id_t *,
            const gss_cred_id_t,
            const gss_buffer_t,
            const gss_channel_bindings_t,
            gss_name_t *,
            gss_OID *,
            gss_buffer_t,
            OM_uint32 *,
            OM_uint32 *,
            gss_cred_id_t *);

#ifdef  __cplusplus
}
#endif

#endif
