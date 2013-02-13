#include "spnegokrb5_locl.h"

static gss_OID_desc gss_krb5_mech_oid_desc =
  {9, (void *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x02"};

gss_OID GSS_KRB5_MECH = &gss_krb5_mech_oid_desc;

static gss_OID_desc gss_spnego_mech_oid_desc =
  {6, (void *)"\x2b\x06\x01\x05\x05\x02"};

gss_OID GSS_SPNEGO_MECH = &gss_spnego_mech_oid_desc;
