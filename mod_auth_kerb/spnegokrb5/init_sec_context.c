/*
 *  SPNEGO wrapper for Kerberos5 GSS-API
 *  kouril@ics.muni.cz, 2003
 *  (mostly based on Heimdal code)
 */

#include "spnegokrb5_locl.h"

static int
add_mech(MechTypeList *mech_list, gss_OID mech)
{
   MechType *tmp;
   int ret;

   tmp = realloc(mech_list->val, (mech_list->len + 1) * sizeof(*tmp));
   if (tmp == NULL)
      return ENOMEM;
   mech_list->val = tmp;

   ret = der_get_oid(mech->elements, mech->length, 
	             &mech_list->val[mech_list->len], NULL);
   if (ret)
     return ret;

   mech_list->len++;
   return 0;
}

#if 0
static int
set_context_flags(OM_uint32 req_flags, ContextFlags *flags)
{
   if (req_flags & GSS_C_DELEG_FLAG)
      flags->delegFlag = 1;
   if (req_flags & GSS_C_MUTUAL_FLAG)
      flags->mutualFlag = 1;
   if (req_flags & GSS_C_REPLAY_FLAG)
      flags->replayFlag = 1;
   if (req_flags & GSS_C_SEQUENCE_FLAG)
      flags->sequenceFlag = 1;
   if (req_flags & GSS_C_ANON_FLAG)
      flags->anonFlag = 1;
   if (req_flags & GSS_C_CONF_FLAG)
      flags->confFlag = 1;
   if (req_flags & GSS_C_INTEG_FLAG)
      flags->integFlag = 1;
   return 0;
}
#endif

OM_uint32 KRB5_LIB_FUNCTION gss_init_sec_context_spnego(
	    OM_uint32 * minor_status,
            const gss_cred_id_t initiator_cred_handle,
            gss_ctx_id_t * context_handle,
            const gss_name_t target_name,
            const gss_OID mech_type,
            OM_uint32 req_flags,
            OM_uint32 time_req,
            const gss_channel_bindings_t input_chan_bindings,
            const gss_buffer_t input_token,
            gss_OID * actual_mech_type,
            gss_buffer_t output_token,
            OM_uint32 * ret_flags,
            OM_uint32 * time_rec)
{
   NegTokenInit token_init;
   OM_uint32 major_status, minor_status2;
   gss_buffer_desc krb5_output_token = GSS_C_EMPTY_BUFFER;
   unsigned char *buf = NULL;
   size_t buf_size;
   size_t len;
   int ret;

   memset(&token_init, 0, sizeof(token_init));

   ALLOC(token_init.mechTypes);
   if (token_init.mechTypes == NULL) {
      *minor_status = ENOMEM;
      return GSS_S_FAILURE;
   }

   ret = add_mech(token_init.mechTypes, GSS_KRB5_MECH);
   if (ret) {
      *minor_status = ret;
      ret = GSS_S_FAILURE;
      goto end;
   }

#if 0
   ALLOC(token_init.reqFlags);
   if (token_init.reqFlags == NULL) {
      *minor_status = ENOMEM;
      ret = GSS_S_FAILURE;
      goto end;
   }
   set_context_flags(req_flags, token_init.reqFlags);
#endif

   major_status = gss_init_sec_context(minor_status,
	 			       initiator_cred_handle,
				       context_handle,
				       target_name,
				       GSS_KRB5_MECH,
				       req_flags,
				       time_req,
				       input_chan_bindings,
				       input_token,
				       actual_mech_type,
				       &krb5_output_token,
				       ret_flags,
				       time_rec);
   if (GSS_ERROR(major_status)) {
      ret = major_status;
      goto end;
   }

   if (krb5_output_token.length > 0) {
      ALLOC(token_init.mechToken);
      if (token_init.mechToken == NULL) {
	 *minor_status = ENOMEM;
	 ret = GSS_S_FAILURE;
	 goto end;
      }
      token_init.mechToken->data = krb5_output_token.value;
      token_init.mechToken->length = krb5_output_token.length;
      krb5_output_token.length = 0; /* don't free it later */
   }

   /* The MS implementation of SPNEGO seems to not like the mechListMIC field,
    * so we omit it (it's optional anyway) */

   buf_size = 1024;
   buf = malloc(buf_size);

    do {
	ret = encode_NegTokenInit(buf + buf_size -1,
				  buf_size,
				  &token_init, &len);
	if (ret == 0) {
	    size_t tmp;

	    ret = der_put_length_and_tag(buf + buf_size - len - 1,
					 buf_size - len,
					 len,
					 KERB_CTXT,
					 CONS,
					 0,
					 &tmp);
	    if (ret == 0)
		len += tmp;
	}
	if (ret) {
	    if (ret == ASN1_OVERFLOW) {
		u_char *tmp;

		buf_size *= 2;
		tmp = realloc (buf, buf_size);
		if (tmp == NULL) {
		    *minor_status = ENOMEM;
		    ret = GSS_S_FAILURE;
		    goto end;
		}
		buf = tmp;
	    } else {
		*minor_status = ret;
		ret = GSS_S_FAILURE;
		goto end;
	    }
	}
    } while (ret == ASN1_OVERFLOW);

    ret = gssapi_spnego_encapsulate(minor_status,
	                            buf + buf_size - len, len,
				    output_token, GSS_SPNEGO_MECH);

   ret = major_status;

end:
   free_NegTokenInit(&token_init);
   if (krb5_output_token.length > 0)
      gss_release_buffer(&minor_status2, &krb5_output_token);
   if (buf)
      free(buf);

   return ret;
}
