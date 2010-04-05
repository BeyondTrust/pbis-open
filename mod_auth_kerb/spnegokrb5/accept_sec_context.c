/*
 * SPNEGO wrapper for Kerberos5 GSS-API 
 * kouril@ics.muni.cz, 2003
 * (mostly based on Heimdal code)
 */

#include "spnegokrb5_locl.h"

#define OID_cmp(o1, o2) \
	(((o1)->length == (o2)->length) && \
	 (memcmp((o1)->components, (o2)->components,(int) (o1)->length) == 0))

static OM_uint32
code_NegTokenArg(OM_uint32 *minor_status,
		 const NegTokenTarg *targ,
		 unsigned char **outbuf,
		 size_t *outbuf_size)
{
    OM_uint32 ret;
    u_char *buf;
    size_t buf_size, buf_len;

    buf_size = 1024;
    buf = malloc(buf_size);
    if (buf == NULL) {
	*minor_status = ENOMEM;
	return GSS_S_FAILURE;
    }

    do {
	ret = encode_NegTokenTarg(buf + buf_size -1,
				  buf_size,
				  targ, &buf_len);
	if (ret == 0) {
	    size_t tmp;

	    ret = der_put_length_and_tag(buf + buf_size - buf_len - 1,
					 buf_size - buf_len,
					 buf_len,
					 KERB_CTXT,
					 CONS,
					 1,
					 &tmp);
	    if (ret == 0)
		buf_len += tmp;
	}
	if (ret) {
	    if (ret == ASN1_OVERFLOW) {
		u_char *tmp;

		buf_size *= 2;
		tmp = realloc (buf, buf_size);
		if (tmp == NULL) {
		    *minor_status = ENOMEM;
		    free(buf);
		    return GSS_S_FAILURE;
		}
		buf = tmp;
	    } else {
		*minor_status = ret;
		free(buf);
		return GSS_S_FAILURE;
	    }
	}
    } while (ret == ASN1_OVERFLOW);

    *outbuf = malloc(buf_len);
    if (*outbuf == NULL) {
       *minor_status = ENOMEM;
       free(buf);
       return GSS_S_FAILURE;
    }

    memcpy(*outbuf, buf + buf_size - buf_len, buf_len);
    *outbuf_size = buf_len;

    free(buf);
    
    return GSS_S_COMPLETE;
}

static OM_uint32
send_reject (OM_uint32 *minor_status,
	     gss_buffer_t output_token)
{
    NegTokenTarg targ;
    OM_uint32 ret;

    targ.negResult = malloc(sizeof(*targ.negResult));
    if (targ.negResult == NULL) {
	*minor_status = ENOMEM;
	return GSS_S_FAILURE;
    }
    *(targ.negResult) = reject;

    targ.supportedMech = NULL;
    targ.responseToken = NULL;
    targ.mechListMIC   = NULL;
    
    ret = code_NegTokenArg (minor_status, &targ, 
	                    (unsigned char**) &output_token->value, &output_token->length);
    free_NegTokenTarg(&targ);
    if (ret)
	return ret;

    return GSS_S_BAD_MECH;
}

static OM_uint32
send_accept (OM_uint32 *minor_status,
	     gss_buffer_t output_token,
	     gss_buffer_t mech_token)
{
    NegTokenTarg targ;
    OM_uint32 ret;

    memset(&targ, 0, sizeof(targ));
    targ.negResult = malloc(sizeof(*targ.negResult));
    if (targ.negResult == NULL) {
	*minor_status = ENOMEM;
	return GSS_S_FAILURE;
    }
    *(targ.negResult) = accept_completed;

    targ.supportedMech = malloc(sizeof(*targ.supportedMech));
    if (targ.supportedMech == NULL) {
	free_NegTokenTarg(&targ);
	*minor_status = ENOMEM;
	return GSS_S_FAILURE;
    }

    ret = der_get_oid(GSS_KRB5_MECH->elements,
		      GSS_KRB5_MECH->length,
		      targ.supportedMech,
		      NULL);
    if (ret) {
	free_NegTokenTarg(&targ);
	*minor_status = ENOMEM;
	return GSS_S_FAILURE;
    }

    if (mech_token != NULL && mech_token->length != 0) {
	targ.responseToken = malloc(sizeof(*targ.responseToken));
	if (targ.responseToken == NULL) {
	    free_NegTokenTarg(&targ);
	    *minor_status = ENOMEM;
	    return GSS_S_FAILURE;
	}
	targ.responseToken->length = mech_token->length;
	targ.responseToken->data   = mech_token->value;
	mech_token->length = 0;
	mech_token->value  = NULL;
    } else {
	targ.responseToken = NULL;
    }

    ret = code_NegTokenArg (minor_status, &targ, 
	                    (unsigned char **) &output_token->value, &output_token->length);
    free_NegTokenTarg(&targ);
    if (ret)
	return ret;

    return GSS_S_COMPLETE;
}

OM_uint32 KRB5_LIB_FUNCTION gss_accept_sec_context_spnego
           (OM_uint32 * minor_status,
            gss_ctx_id_t * context_handle,
            const gss_cred_id_t acceptor_cred_handle,
            const gss_buffer_t input_token_buffer,
            const gss_channel_bindings_t input_chan_bindings,
            gss_name_t * src_name,
            gss_OID * mech_type,
            gss_buffer_t output_token,
            OM_uint32 * ret_flags,
            OM_uint32 * time_rec,
            gss_cred_id_t * delegated_cred_handle)
{
   NegTokenInit init_token;
   OM_uint32 major_status;
   OM_uint32 minor_status2;
   gss_buffer_desc ibuf, obuf;
   gss_buffer_t ot = NULL;
   unsigned char *buf;
   size_t buf_size;
   size_t len, taglen, ni_len;
   int found = 0;
   int ret, i;

   memset(&init_token, 0, sizeof(init_token));

   ret = gssapi_spnego_decapsulate(minor_status, input_token_buffer,
	 			   &buf, &buf_size, GSS_SPNEGO_MECH);
   if (ret)
      return ret;

   ret = der_match_tag_and_length(buf, buf_size, KERB_CTXT, CONS,
	 			  0, &len, &taglen);
   if (ret)
      return ret;

   ret = decode_NegTokenInit(buf + taglen, len, &init_token, &ni_len);
   if (ret) {
      *minor_status = EINVAL; /* XXX */
      return GSS_S_DEFECTIVE_TOKEN;
   }

   if (init_token.mechTypes == NULL)
      return send_reject (minor_status, output_token);

   for (i = 0; !found && i < init_token.mechTypes->len; ++i) {
      unsigned char mechbuf[17];
      size_t mech_len;

      ret = der_put_oid (mechbuf + sizeof(mechbuf) - 1,
                         sizeof(mechbuf),
                         &init_token.mechTypes->val[i],
                         &mech_len);
      if (ret)
          return GSS_S_DEFECTIVE_TOKEN;
      if (mech_len == GSS_KRB5_MECH->length
          && memcmp(GSS_KRB5_MECH->elements,
                    mechbuf + sizeof(mechbuf) - mech_len,
                    mech_len) == 0)
          found = 1;
   }

   if (!found)
      return send_reject (minor_status, output_token);

   if (init_token.mechToken != NULL) {
      ibuf.length = init_token.mechToken->length;
      ibuf.value  = init_token.mechToken->data;

      major_status = gss_accept_sec_context(minor_status,
	    				    context_handle,
					    acceptor_cred_handle,
					    &ibuf,
					    input_chan_bindings,
					    src_name,
					    mech_type,
					    &obuf,
					    ret_flags,
					    time_rec,
					    delegated_cred_handle);
      if (GSS_ERROR(major_status)) {
	 send_reject (&minor_status2, output_token);
	 return major_status;
      }
      ot = &obuf;
   }

   ret = send_accept (&minor_status2, output_token, ot);
   if (ot != NULL)
      gss_release_buffer(&minor_status2, ot);

   return ret;
}
