#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "config.h"

#ifdef HEIMDAL
#  include <gssapi.h>
#else
#  include <gssapi/gssapi.h>
#  include <gssapi/gssapi_generic.h>
#endif
#include <spnego_asn1.h>
#include <spnegokrb5.h>
#include <der.h>
#include <asn1_err.h>

#define ALLOC(X) (X) = calloc(1, sizeof(*(X)))

extern gss_OID GSS_KRB5_MECH;
extern gss_OID GSS_SPNEGO_MECH;

OM_uint32
gssapi_spnego_encapsulate(
                        OM_uint32 *,
                        unsigned char *,
                        size_t,
                        gss_buffer_t,
                        const gss_OID);

OM_uint32
gssapi_spnego_decapsulate(
                        OM_uint32 *,
                        gss_buffer_t,
                        unsigned char **,
                        size_t *,
                        const gss_OID);
