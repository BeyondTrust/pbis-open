/* MODULE: auth_krb5 */

/* COPYRIGHT
 * Copyright (c) 1997 Messaging Direct Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MESSAGING DIRECT LTD. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL MESSAGING DIRECT LTD. OR
 * ITS EMPLOYEES OR AGENTS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * END COPYRIGHT */

#ifdef __GNUC__
#ident "$Id: auth_krb5.c,v 1.17 2005/02/14 05:50:49 shadow Exp $"
#endif

/* ok, this is  wrong but the most convenient way of doing 
 * it for now. We assume (possibly incorrectly) that if GSSAPI exists then 
 * the Kerberos 5 headers and libraries exist.   
 * What really should be done is a configure.in check for krb5.h and use 
 * that since none of this code is GSSAPI but rather raw Kerberos5.
 */


/* Also, at some point one would hope it would be possible to
 * have less divergence between Heimdal and MIT Kerberos 5.
 *
 * As of the summer of 2003, the obvious issues are that
 * MIT doesn't have krb5_verify_opt_*() and Heimdal doesn't
 * have krb5_sname_to_principal().
 */

/* PUBLIC DEPENDENCIES */
#include "mechanisms.h"
#include "globals.h" /* mech_option */
#include "cfile.h"
#include "krbtf.h"

#ifdef AUTH_KRB5
# include <krb5.h>
static cfile config = 0;
static char *keytabname = NULL; /* "system default" */
static char *verify_principal = "host"; /* a principal in the default keytab */
#endif /* AUTH_KRB5 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include "auth_krb5.h"

/* END PUBLIC DEPENDENCIES */

int					/* R: -1 on failure, else 0 */
auth_krb5_init (
  /* PARAMETERS */
  void					/* no parameters */
  /* END PARAMETERS */
  )
{
#ifdef AUTH_KRB5
    int rc;
    char *configname = 0;

    if (krbtf_init() == -1) {
	syslog(LOG_ERR, "auth_krb5_init krbtf_init failed");
	return -1;
    }

    if (mech_option)
	configname = mech_option;
    else if (access(SASLAUTHD_CONF_FILE_DEFAULT, F_OK) == 0)
	configname = SASLAUTHD_CONF_FILE_DEFAULT;
 
    if (configname) {
	char complaint[1024];

	if (!(config = cfile_read(configname, complaint, sizeof (complaint)))) {
	    syslog(LOG_ERR, "auth_krb5_init %s", complaint);
	    return -1;
	}
    }

    if (config) {
	keytabname = cfile_getstring(config, "krb5_keytab", keytabname);
	verify_principal = cfile_getstring(config, "krb5_verify_principal", verify_principal);
    }

    return 0;

#else
    return -1;
#endif
}

#ifdef AUTH_KRB5

static int
form_principal_name (
  const char *user,
  const char *service,
  const char *realm,
  char *pname,
  int pnamelen
  )
{
    const char *forced_instance = 0;
	int plen;

    if (config) {
	char keyname[1024];

	snprintf(keyname, sizeof (keyname), "krb5_%s_instance", service);
	forced_instance = cfile_getstring(config, keyname, 0);
    }

    if (forced_instance) {
	char *user_specified;

	if (user_specified = strchr(user, '/')) {
	    if (strcmp(user_specified + 1, forced_instance)) {
		/* user not allowed to override sysadmin */
		return -1;
	    } else {
		/* don't need to force--user already asked for it */
		forced_instance = 0;
	    }
	}
    }

    /* form user[/instance][@realm] */
    plen = snprintf(pname, pnamelen, "%s%s%s%s%s",
	user,
	(forced_instance ? "/" : ""),
	(forced_instance ? forced_instance : ""),
	((realm && realm[0]) ? "@" : ""),
	((realm && realm[0]) ? realm : "")
	);
    if ((plen <= 0) || (plen >= pnamelen))
	return -1;

    /* Perhaps we should uppercase the realm? */

    return 0;
}

#ifdef KRB5_HEIMDAL

char *					/* R: allocated response string */
auth_krb5 (
  /* PARAMETERS */
  const char *user,			/* I: plaintext authenticator */
  const char *password,			/* I: plaintext password */
  const char *service,                  /* I: service authenticating to */
  const char *realm                     /* I: user's realm */
  /* END PARAMETERS */
  )
{
    /* VARIABLES */
    krb5_context context;
    krb5_ccache ccache = NULL;
    krb5_keytab kt = NULL;
    krb5_principal auth_user;
    krb5_verify_opt opt;
    char * result;
    char tfname[2048];
    char principalbuf[2048];
    /* END VARIABLES */

    if (!user || !password) {
	syslog(LOG_ERR, "auth_krb5: NULL password or username?");
	return strdup("NO saslauthd internal NULL password or username");
    }

    if (krb5_init_context(&context)) {
	syslog(LOG_ERR, "auth_krb5: krb5_init_context");
	return strdup("NO saslauthd internal krb5_init_context error");
    }

    if (form_principal_name(user, service, realm, principalbuf, sizeof (principalbuf))) {
	syslog(LOG_ERR, "auth_krb5: form_principal_name");
	return strdup("NO saslauthd principal name error");
    }

    if (krb5_parse_name (context, principalbuf, &auth_user)) {
	krb5_free_context(context);
	syslog(LOG_ERR, "auth_krb5: krb5_parse_name");
	return strdup("NO saslauthd internal krb5_parse_name error");
    }

    if (krbtf_name(tfname, sizeof (tfname)) != 0) {
	syslog(LOG_ERR, "auth_krb5: could not generate ccache name");
	return strdup("NO saslauthd internal error");
    }

    if (krb5_cc_resolve(context, tfname, &ccache)) {
	krb5_free_principal(context, auth_user);
	krb5_free_context(context);
	syslog(LOG_ERR, "auth_krb5: krb5_cc_resolve");
	return strdup("NO saslauthd internal error");
    }

    if (keytabname) {
	if (krb5_kt_resolve(context, keytabname, &kt)) {
	    krb5_free_principal(context, auth_user);
	    krb5_cc_destroy(context, ccache);
	    krb5_free_context(context);
	    syslog(LOG_ERR, "auth_krb5: krb5_kt_resolve");
	    return strdup("NO saslauthd internal error");
	}
    }
    
    krb5_verify_opt_init(&opt);
    krb5_verify_opt_set_secure(&opt, 1);
    krb5_verify_opt_set_ccache(&opt, ccache);
    if (kt)
	krb5_verify_opt_set_keytab(&opt,  kt);
    krb5_verify_opt_set_service(&opt, verify_principal);
    
    if (krb5_verify_user_opt(context, auth_user, password, &opt)) {
	result = strdup("NO krb5_verify_user_opt failed");
    } else {
        result = strdup("OK");
    }
    
    krb5_free_principal(context, auth_user);
    krb5_cc_destroy(context, ccache);
    if (kt)
	krb5_kt_close(context, kt);
    krb5_free_context(context);

    return result;
}

#else /* !KRB5_HEIMDAL */

/* returns 0 for failure, 1 for success */
static int k5support_verify_tgt(krb5_context context, 
				krb5_ccache ccache) 
{
    krb5_principal server;
    krb5_data packet;
    krb5_keyblock *keyblock = NULL;
    krb5_auth_context auth_context = NULL;
    krb5_error_code k5_retcode;
    krb5_keytab kt = NULL;
    char thishost[BUFSIZ];
    int result = 0;
    
    memset(&packet, 0, sizeof(packet));

    if (krb5_sname_to_principal(context, NULL, verify_principal,
				KRB5_NT_SRV_HST, &server)) {
	return 0;
    }

    if (keytabname) {
	if (krb5_kt_resolve(context, keytabname, &kt)) {
	    goto fini;
	}
    }
    
    if (krb5_kt_read_service_key(context, kt, server, 0,
				 0, &keyblock)) {
	goto fini;
    }
    
    if (keyblock) {
	krb5_free_keyblock(context, keyblock);
    }
    
    /* this duplicates work done in krb5_sname_to_principal
     * oh well.
     */
    if (gethostname(thishost, BUFSIZ) < 0) {
	goto fini;
    }
    thishost[BUFSIZ-1] = '\0';
    
    k5_retcode = krb5_mk_req(context, &auth_context, 0, verify_principal, 
			     thishost, NULL, ccache, &packet);
    
    if (auth_context) {
	krb5_auth_con_free(context, auth_context);
	auth_context = NULL;
    }
    
    if (k5_retcode) {
	goto fini;
    }
    
    if (krb5_rd_req(context, &auth_context, &packet, 
		    server, NULL, NULL, NULL)) {
	goto fini;
    }

    if (auth_context) {
      krb5_auth_con_free(context, auth_context);
      auth_context = NULL;
    }
    
    /* all is good now */
    result = 1;
 fini:
    krb5_free_data_contents(context, &packet);
    krb5_free_principal(context, server);
    
    return result;
}

/* FUNCTION: auth_krb5 */

/* SYNOPSIS
 * Authenticate against Kerberos V.
 * END SYNOPSIS */

char *					/* R: allocated response string */
auth_krb5 (
  /* PARAMETERS */
  const char *user,			/* I: plaintext authenticator */
  const char *password,			/* I: plaintext password */
  const char *service,			/* I: service authenticating to */
  const char *realm			/* I: user's realm */
  /* END PARAMETERS */
  )
{
    /* VARIABLES */
    krb5_context context;
    krb5_ccache ccache = NULL;
    krb5_principal auth_user;
    krb5_creds creds;
    krb5_get_init_creds_opt opts;
    char * result;
    char tfname[2048];
    char principalbuf[2048];
    krb5_error_code code;
    /* END VARIABLES */

    if (!user|| !password) {
	syslog(LOG_ERR, "auth_krb5: NULL password or username?");
	return strdup("NO saslauthd internal error");
    }

    if (krb5_init_context(&context)) {
	syslog(LOG_ERR, "auth_krb5: krb5_init_context");
	return strdup("NO saslauthd internal error");
    }

    if (form_principal_name(user, service, realm, principalbuf, sizeof (principalbuf))) {
	syslog(LOG_ERR, "auth_krb5: form_principal_name");
	return strdup("NO saslauthd principal name error");
    }

    if (krb5_parse_name (context, principalbuf, &auth_user)) {
	krb5_free_context(context);
	syslog(LOG_ERR, "auth_krb5: krb5_parse_name");
	return strdup("NO saslauthd internal error");
    }
    
    if (krbtf_name(tfname, sizeof (tfname)) != 0) {
	syslog(LOG_ERR, "auth_krb5: could not generate ticket file name");
	return strdup("NO saslauthd internal error");
    }

    if (krb5_cc_resolve(context, tfname, &ccache)) {
	krb5_free_principal(context, auth_user);
	krb5_free_context(context);
	syslog(LOG_ERR, "auth_krb5: krb5_cc_resolve");
	return strdup("NO saslauthd internal error");
    }
    
    if (krb5_cc_initialize (context, ccache, auth_user)) {
	krb5_free_principal(context, auth_user);
	krb5_free_context(context);
	syslog(LOG_ERR, "auth_krb5: krb5_cc_initialize");
	return strdup("NO saslauthd internal error");
    }
    
    krb5_get_init_creds_opt_init(&opts);
    /* 15 min should be more than enough */
    krb5_get_init_creds_opt_set_tkt_life(&opts, 900); 
    if (code = krb5_get_init_creds_password(context, &creds, 
				     auth_user, password, NULL, NULL, 
				     0, NULL, &opts)) {
	krb5_cc_destroy(context, ccache);
	krb5_free_principal(context, auth_user);
	krb5_free_context(context);
	syslog(LOG_ERR, "auth_krb5: krb5_get_init_creds_password: %d", code);
	return strdup("NO saslauthd internal error");
    }
    
    /* at this point we should have a TGT. Let's make sure it is valid */
    if (krb5_cc_store_cred(context, ccache, &creds)) {
	krb5_free_principal(context, auth_user);
	krb5_cc_destroy(context, ccache);
	krb5_free_context(context);
	syslog(LOG_ERR, "auth_krb5: krb5_cc_store_cred");
	return strdup("NO saslauthd internal error");
    }
    
    if (!k5support_verify_tgt(context, ccache)) {
	syslog(LOG_ERR, "auth_krb5: k5support_verify_tgt");
	result = strdup("NO saslauthd internal error");
	goto fini;
    }
    
    /* 
     * fall through -- user is valid beyond this point  
     */
    
    result = strdup("OK");
 fini:
/* destroy any tickets we had */
    krb5_free_cred_contents(context, &creds);
    krb5_free_principal(context, auth_user);
    krb5_cc_destroy(context, ccache);
    krb5_free_context(context);

    return result;
}

#endif /* KRB5_HEIMDAL */

#else /* ! AUTH_KRB5 */

char *
auth_krb5 (
  const char *login __attribute__((unused)),
  const char *password __attribute__((unused)),
  const char *service __attribute__((unused)),
  const char *realm __attribute__((unused))
  )
{
    return NULL;
}

#endif /* ! AUTH_KRB5 */

/* END FUNCTION: auth_krb5 */

/* END MODULE: auth_krb5 */
