/*
 * Daniel Kouril <kouril@users.sourceforge.net>
 *
 * Source and Documentation can be found at:
 * http://modauthkerb.sourceforge.net/
 *
 * Based on work by
 *   James E. Robinson, III <james@ncstate.net>
 *   Daniel Henninger <daniel@ncsu.edu>
 *   Ludek Sulak <xsulak@fi.muni.cz>
 */

/*
 * Copyright (c) 2004-2006 Masarykova universita
 * (Masaryk University, Brno, Czech Republic)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if 0
#ident "$Id: mod_auth_kerb.c,v 1.131.2.1 2006/11/22 10:32:58 kouril Exp $"
#endif

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#define MODAUTHKERB_VERSION "5.3"

#define MECH_NEGOTIATE "Negotiate"
#define SERVICE_NAME "HTTP"

#include <httpd.h>
#include <http_config.h>
#include <http_core.h>
#include <http_log.h>
#include <http_protocol.h>
#include <http_request.h>
#include <http_connection.h>

#ifdef STANDARD20_MODULE_STUFF
#include <apr_strings.h>
#include <apr_base64.h>
#else
#define apr_pstrdup		ap_pstrdup
#define apr_psprintf		ap_psprintf
#define apr_pstrcat		ap_pstrcat
#define apr_pcalloc		ap_pcalloc
#define apr_table_setn		ap_table_setn
#define apr_table_add		ap_table_add
#define apr_base64_decode_len	ap_base64decode_len
#define apr_base64_decode	ap_base64decode
#define apr_base64_encode_len	ap_base64encode_len
#define apr_base64_encode	ap_base64encode
#define apr_pool_cleanup_null	ap_null_cleanup
#define apr_pool_cleanup_register	ap_register_cleanup
#endif /* STANDARD20_MODULE_STUFF */

#ifndef AP_REGEX_H
#define ap_regex_t regex_t
#define ap_regmatch_t regmatch_t
#define AP_REG_ICASE REG_ICASE
#define AP_REG_EXTENDED REG_EXTENDED
#endif

#ifdef _WIN32
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif

#ifdef KRB5
#include <krb5.h>
#ifdef HEIMDAL
#  include <gssapi.h>
#else
#  include <gssapi/gssapi.h>
#  include <gssapi/gssapi_generic.h>
#  include <gssapi/gssapi_krb5.h>
#  define GSS_C_NT_USER_NAME gss_nt_user_name
#  define GSS_C_NT_HOSTBASED_SERVICE gss_nt_service_name
#  define GSS_KRB5_NT_PRINCIPAL_NAME gss_nt_krb5_name
#  define krb5_get_err_text(context,code) error_message(code)
#endif
#ifndef GSSAPI_SUPPORTS_SPNEGO
#  include "spnegokrb5.h"
#endif
#endif /* KRB5 */

#ifdef KRB4
/* Prevent warning about closesocket redefinition (Apache's ap_config.h and 
 * MIT Kerberos' port-sockets.h both define it as close) */
#ifdef closesocket
#  undef closesocket
#endif
#include <krb.h>
#include <netdb.h> /* gethostbyname() */
#endif /* KRB4 */

#ifndef _WIN32
/* should be HAVE_UNISTD_H instead */
#include <unistd.h>
#endif

#ifndef KRB5_LIB_FUNCTION
#  if defined(_WIN32)
#    define KRB5_LIB_FUNCTION _stdcall
#  else
#    define KRB5_LIB_FUNCTION
#  endif
#endif

#ifdef STANDARD20_MODULE_STUFF
module AP_MODULE_DECLARE_DATA auth_kerb_module;
#else
module auth_kerb_module;
#endif

/*************************************************************************** 
 Macros To Ease Compatibility
 ***************************************************************************/
#ifdef STANDARD20_MODULE_STUFF
#define MK_POOL apr_pool_t
#define MK_TABLE_GET apr_table_get
#define MK_USER r->user
#define MK_MAIN_USER mainr->user
#define MK_AUTH_TYPE r->ap_auth_type
#define MK_MAIN_AUTH_TYPE mainr->ap_auth_type
#else
#define MK_POOL pool
#define MK_TABLE_GET ap_table_get
#define MK_USER r->connection->user
#define MK_MAIN_USER mainr->connection->user
#define MK_AUTH_TYPE r->connection->ap_auth_type
#define MK_MAIN_AUTH_TYPE mainr->connection->ap_auth_type
#define PROXYREQ_PROXY STD_PROXY
#endif

/*************************************************************************** 
 Auth Configuration Structure
 ***************************************************************************/
typedef struct {
	char *krb_auth_realms;
	int krb_save_credentials;
	int krb_verify_kdc;
	const char *krb_service_name;
	int krb_authoritative;
	int krb_delegate_basic;
	const char* krb_auth_user_format;
#if 0
	int krb_ssl_preauthentication;
#endif
#ifdef KRB5
	char *krb_5_keytab;
	int krb_method_gssapi;
	int krb_method_k5pass;
#endif
#ifdef KRB4
	char *krb_4_srvtab;
	int krb_method_k4pass;
#endif
} kerb_auth_config;

static void
set_kerb_auth_headers(request_rec *r, const kerb_auth_config *conf,
                      int use_krb4, int use_krb5pwd, char *negotiate_ret_value);

static const char*
krb5_save_realms(cmd_parms *cmd, void *sec, const char *arg);

#ifdef STANDARD20_MODULE_STUFF
#define command(name, func, var, type, usage)           \
  AP_INIT_ ## type (name, (void*) func,                 \
        (void*)APR_OFFSETOF(kerb_auth_config, var),     \
        OR_AUTHCFG | RSRC_CONF, usage)
#else
#define command(name, func, var, type, usage) 		\
  { name, func, 					\
    (void*)XtOffsetOf(kerb_auth_config, var), 		\
    OR_AUTHCFG | RSRC_CONF, type, usage }
#endif

static const command_rec kerb_auth_cmds[] = {
   command("KrbAuthRealms", krb5_save_realms, krb_auth_realms,
     RAW_ARGS, "Realms to attempt authentication against (can be multiple)."),

   command("KrbAuthRealm", krb5_save_realms, krb_auth_realms,
     RAW_ARGS, "Alias for KrbAuthRealms."),

   command("KrbSaveCredentials", ap_set_flag_slot, krb_save_credentials,
     FLAG, "Save and store credentials/tickets retrieved during auth."),

   command("KrbVerifyKDC", ap_set_flag_slot, krb_verify_kdc,
     FLAG, "Verify tickets against keytab to prevent KDC spoofing attacks."),

   command("KrbServiceName", ap_set_string_slot, krb_service_name,
     TAKE1, "Full or partial service name to be used by Apache for authentication."),

   command("KrbAuthoritative", ap_set_flag_slot, krb_authoritative,
     FLAG, "Set to 'off' to allow access control to be passed along to lower modules iff the UserID is not known to this module."),

   command("KrbDelegateBasic", ap_set_flag_slot, krb_delegate_basic,
     FLAG, "Always offer Basic authentication regardless of KrbMethodK5Pass and pass on authentication to lower modules if Basic headers arrive."),

#if 0
   command("KrbEnableSSLPreauthentication", ap_set_flag_slot, krb_ssl_preauthentication,
     FLAG, "Don't do Kerberos authentication if the user is already authenticated using SSL and her client certificate."),
#endif

#ifdef KRB5
   command("Krb5Keytab", ap_set_file_slot, krb_5_keytab,
     TAKE1, "Location of Kerberos V5 keytab file."),

   command("KrbMethodNegotiate", ap_set_flag_slot, krb_method_gssapi,
     FLAG, "Enable Negotiate authentication method."),

   command("KrbMethodK5Passwd", ap_set_flag_slot, krb_method_k5pass,
     FLAG, "Enable Kerberos V5 password authentication."),
#endif 

#ifdef KRB4
   command("Krb4Srvtab", ap_set_file_slot, krb_4_srvtab,
     TAKE1, "Location of Kerberos V4 srvtab file."),

   command("KrbMethodK4Passwd", ap_set_flag_slot, krb_method_k4pass,
     FLAG, "Enable Kerberos V4 password authentication."),
#endif

   { NULL }
};

#ifdef _WIN32
int
mkstemp(char *template)
{
    int start, i;
    pid_t val;
    val = getpid();
    start = strlen(template) - 1;
    while(template[start] == 'X') {
	template[start] = '0' + val % 10;
	val /= 10;
	start--;
    }
    
    do{
	int fd;
	fd = open(template, O_RDWR | O_CREAT | O_EXCL, 0600);
	if(fd >= 0 || errno != EEXIST)
	    return fd;
	i = start + 1;
	do{
	    if(template[i] == 0)
		return -1;
	    template[i]++;
	    if(template[i] == '9' + 1)
		template[i] = 'a';
	    if(template[i] <= 'z')
		break;
	    template[i] = 'a';
	    i++;
	}while(1);
    }while(1);
}
#endif


/*************************************************************************** 
 Auth Configuration Initialization
 ***************************************************************************/
static void *kerb_dir_create_config(MK_POOL *p, char *d)
{
	kerb_auth_config *rec;

	rec = (kerb_auth_config *) apr_pcalloc(p, sizeof(kerb_auth_config));
        ((kerb_auth_config *)rec)->krb_verify_kdc = 1;
	((kerb_auth_config *)rec)->krb_service_name = NULL;
	((kerb_auth_config *)rec)->krb_auth_user_format = NULL;
	((kerb_auth_config *)rec)->krb_authoritative = 1;
	((kerb_auth_config *)rec)->krb_delegate_basic = 0;
#if 0
	((kerb_auth_config *)rec)->krb_ssl_preauthentication = 0;
#endif
#ifdef KRB5
	((kerb_auth_config *)rec)->krb_method_k5pass = 1;
	((kerb_auth_config *)rec)->krb_method_gssapi = 1;
#endif
#ifdef KRB4
	((kerb_auth_config *)rec)->krb_method_k4pass = 1;
#endif
	return rec;
}

static const char*
krb5_save_realms(cmd_parms *cmd, void *vsec, const char *arg)
{
   kerb_auth_config *sec = (kerb_auth_config *) vsec;
   sec->krb_auth_realms= apr_pstrdup(cmd->pool, arg);
   return NULL;
}

static void
log_rerror(const char *file, int line, int level, int status,
           const request_rec *r, const char *fmt, ...)
{
   char errstr[1024];
   va_list ap;

   va_start(ap, fmt);
   vsnprintf(errstr, sizeof(errstr), fmt, ap);
   va_end(ap);

   
#ifdef STANDARD20_MODULE_STUFF
   ap_log_rerror(file, line, level | APLOG_NOERRNO, status, r, "%s", errstr);
#else
   ap_log_rerror(file, line, level | APLOG_NOERRNO, r, "%s", errstr);
#endif
}

static int
is_basic_auth_on(const kerb_auth_config *conf)
{
#ifdef KRB5
   if (conf->krb_method_k5pass || conf->krb_delegate_basic)
       return 1;
#endif

#ifdef KRB4
   if (conf->krb_method_k4pass || conf->krb_delegate_basic)
       return 1;
#endif

   return 0;
}

#ifdef KRB4
/*************************************************************************** 
 Username/Password Validation for Krb4
 ***************************************************************************/
static int
verify_krb4_user(request_rec *r, const char *name, const char *instance,
                 const char *realm, const char *password, const char *linstance, const char *srvtab, int krb_verify_kdc)
{
   int ret;
   char *phost;
   unsigned long addr;
   struct hostent *hp;
   const char *hostname;
   KTEXT_ST ticket;
   AUTH_DAT authdata;
   char lrealm[REALM_SZ];

   ret = krb_get_pw_in_tkt(name, instance, realm, "krbtgt", realm, 
	 		   DEFAULT_TKT_LIFE, password);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "Cannot get krb4 ticket: krb_get_pw_in_tkt() failed: %s",
		 krb_get_err_text(ret));
      return ret;
   }

   if (!krb_verify_kdc)
      return ret;

   hostname = ap_get_server_name(r);

   hp = gethostbyname(hostname);
   if (hp == NULL) {
      dest_tkt();
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "Cannot verify krb4 ticket: gethostbyname() failed: %s",
		 hstrerror(h_errno));
      return h_errno;
   }
   memcpy(&addr, hp->h_addr, sizeof(addr));

   phost = krb_get_phost((char *)hostname);

   krb_get_lrealm(lrealm, 1);

   ret = krb_mk_req(&ticket, linstance, phost, lrealm, 0);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "Cannot verify krb4 ticket: krb_mk_req() failed: %s",
		 krb_get_err_text(ret));
      dest_tkt();
      return ret;
   }

   ret = krb_rd_req(&ticket, (char *)linstance, phost, addr, &authdata, (char *)srvtab);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "Cannot verify krb4 ticket: krb_rd_req() failed: %s",
		 krb_get_err_text(ret));
      dest_tkt();
   }

   return ret;
}

static int
krb4_cache_cleanup(void *data)
{
   char *tkt_file = (char *) data;
   
   krb_set_tkt_string(tkt_file);
   dest_tkt();
   return OK;
}

static int 
authenticate_user_krb4pwd(request_rec *r,
      			  kerb_auth_config *conf,
			  const char *auth_line)
{
   int ret;
   const char *sent_pw;
   const char *sent_name;
   char *sent_instance;
   char tkt_file[32];
   char *tkt_file_p = NULL;
   int fd;
   const char *realms;
   const char *realm;
   char *user;
   char lrealm[REALM_SZ];
   int all_principals_unkown;

   sent_pw = ap_pbase64decode(r->pool, auth_line);
   sent_name = ap_getword (r->pool, &sent_pw, ':');

   sent_instance = strchr(sent_name, '.');
   if (sent_instance)
      *sent_instance++ = '\0'; 

   snprintf(tkt_file, sizeof(tkt_file), "/tmp/apache_tkt_XXXXXX");
   fd = mkstemp(tkt_file);
   if (fd < 0) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "Cannot create krb4 ccache: mkstemp() failed: %s",
		 strerror(errno));
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   tkt_file_p = apr_pstrdup(r->pool, tkt_file);
   apr_pool_cleanup_register(r->pool, tkt_file_p, krb4_cache_cleanup,
	                     apr_pool_cleanup_null);
   
   krb_set_tkt_string(tkt_file);

   all_principals_unkown = 1;
   realms = conf->krb_auth_realms;
   do {
      memset(lrealm, 0, sizeof(lrealm));
      realm = NULL;
      if (realms)
	 realm = ap_getword_white(r->pool, &realms);

      if (realm == NULL) {
	 ret = krb_get_lrealm(lrealm, 1);
	 if (ret)
	    break;
	 realm = lrealm;
      }

      /* XXX conf->krb_service_name */
      ret = verify_krb4_user(r, (char *)sent_name, 
	                     (sent_instance) ? sent_instance : "",
	    		     (char *)realm, (char *)sent_pw,
			     conf->krb_service_name,
			     conf->krb_4_srvtab, conf->krb_verify_kdc);
      if (!conf->krb_authoritative && ret) {
	 /* if we're not authoritative, we allow authentication to pass on
	  * to another modules if (and only if) the user is not known to us */
	 if (all_principals_unkown && ret != KDC_PR_UNKNOWN)
	    all_principals_unkown = 0;
      }

      if (ret == 0)
	 break;
   } while (realms && *realms);

   if (ret) {
      /* XXX log only in the verify_krb4_user() call */
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Verifying krb4 password failed");
      ret = (!conf->krb_authoritative && all_principals_unkown == 1 && ret == KDC_PR_UNKNOWN) ?
	         DECLINED : HTTP_UNAUTHORIZED;
      goto end;
   }

   user = apr_pstrdup(r->pool, sent_name);

   log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
 	      "Setting the request user to %s",
 	      user ? user : "(NULL)");

   MK_USER = user;
   MK_AUTH_TYPE = "Basic";
   apr_table_setn(r->subprocess_env, "KRBTKFILE", tkt_file_p);

   if (!conf->krb_save_credentials)
      krb4_cache_cleanup(tkt_file);

end:
   if (ret)
      krb4_cache_cleanup(tkt_file);
   close(fd);
   tf_close();

   return ret;
}
#endif /* KRB4 */

#ifdef KRB5
/*************************************************************************** 
 Username/Password Validation for Krb5
 ***************************************************************************/

#ifdef HEIMDAL
static void krb5_free_unparsed_name(krb5_context context, char *p)
{
    if (p)
        free(p);
}
#endif

/* MIT kerberos uses replay cache checks even during credential verification
 * (i.e. in krb5_verify_init_creds()), which is obviosuly useless. In order to
 * avoid problems with multiple apache processes accessing the same rcache file
 * we had to use this call instead, which is only a bit modified version of
 * krb5_verify_init_creds() */
static krb5_error_code
verify_krb5_init_creds(request_rec *r, krb5_context context, krb5_creds *creds,
                       krb5_principal ap_req_server, krb5_keytab ap_req_keytab)
{
   krb5_error_code ret;
   krb5_data req;
   krb5_ccache local_ccache = NULL;
   krb5_creds *new_creds = NULL;
   krb5_auth_context auth_context = NULL;
   krb5_keytab keytab = NULL;
   char *server_name;

   memset(&req, 0, sizeof(req));

   if (ap_req_keytab == NULL) {
      ret = krb5_kt_default (context, &keytab);
      if (ret)
	     goto end;
   } else
      keytab = ap_req_keytab;

   ret = krb5_cc_resolve(context, "MEMORY:", &local_ccache);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "krb5_cc_resolve() failed when verifying KDC");
      goto end;
   }

   ret = krb5_cc_initialize(context, local_ccache, creds->client);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "krb5_cc_initialize() failed when verifying KDC");
      goto end;
   }

   ret = krb5_cc_store_cred (context, local_ccache, creds);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "krb5_cc_initialize() failed when verifying KDC");
      goto end;
   }
   
   ret = krb5_unparse_name(context, ap_req_server, &server_name);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "krb5_unparse_name() failed when verifying KDC");
      goto end;
   }
   log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	      "Trying to verify authenticity of KDC using principal %s", server_name);
   krb5_free_unparsed_name(context, server_name);

   if (!krb5_principal_compare (context, ap_req_server, creds->server)) {
      krb5_creds match_cred;

      memset (&match_cred, 0, sizeof(match_cred));

      match_cred.client = creds->client;
      match_cred.server = ap_req_server;

      ret = krb5_get_credentials (context, 0, local_ccache, 
	                          &match_cred, &new_creds);
      if (ret) {
	 log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	            "krb5_get_credentials() failed when verifying KDC");
	 goto end;
      }
      creds = new_creds;
   }

   ret = krb5_mk_req_extended (context, &auth_context, 0, NULL, creds, &req);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "krb5_mk_req_extended() failed when verifying KDC");
      goto end;
   }

   krb5_auth_con_free (context, auth_context);
   auth_context = NULL;
   ret = krb5_auth_con_init(context, &auth_context);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "krb5_auth_con_init() failed when verifying KDC");
      goto end;
   }
   /* use KRB5_AUTH_CONTEXT_DO_SEQUENCE to skip replay cache checks */
   krb5_auth_con_setflags(context, auth_context, KRB5_AUTH_CONTEXT_DO_SEQUENCE);

   ret = krb5_rd_req (context, &auth_context, &req, ap_req_server,
		      keytab, 0, NULL);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "krb5_rd_req() failed when verifying KDC");
      goto end;
   }

end:
#ifdef HEIMDAL
   /* XXX Do I ever want to support Heimdal 0.4 ??? */
   krb5_data_free(&req);
#else
   krb5_free_data_contents(context, &req);
#endif
   if (auth_context)
      krb5_auth_con_free (context, auth_context);
   if (new_creds)
      krb5_free_creds (context, new_creds);
   if (ap_req_keytab == NULL && keytab)
      krb5_kt_close (context, keytab);
   if (local_ccache)
      krb5_cc_destroy (context, local_ccache);

   return ret;
}

/* Inspired by krb5_verify_user from Heimdal */
static krb5_error_code
verify_krb5_user(request_rec *r, krb5_context context, krb5_principal principal,
      		 const char *password, krb5_principal server,
		 krb5_keytab keytab, int krb_verify_kdc, krb5_ccache *ccache)
{
   krb5_creds creds;
   krb5_error_code ret;
   krb5_ccache ret_ccache = NULL;
   char *name = NULL;

   /* XXX error messages shouldn't be logged here (and in the while() loop in
    * authenticate_user_krb5pwd() as weell), in order to avoid confusing log
    * entries when using multiple realms */

   memset(&creds, 0, sizeof(creds));

   ret = krb5_unparse_name(context, principal, &name);
   if (ret == 0) {
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	         "Trying to get TGT for user %s", name);
      krb5_free_unparsed_name(context,name);
   }

   ret = krb5_get_init_creds_password(context, &creds, principal, 
	 			      (char *)password, NULL,
				      NULL, 0, NULL, NULL);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "krb5_get_init_creds_password() failed: %s",
		 krb5_get_err_text(context, ret));
      goto end;
   }

   /* XXX
   {
      char *realm;

      krb5_get_default_realm(context, &realm);
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                 "trying to verify password using key for %s/%s@%s",
		 service, ap_get_server_name(r), realm);
   }
   */

   if (krb_verify_kdc &&
       (ret = verify_krb5_init_creds(r, context, &creds, server, keytab))) {
       log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	          "failed to verify krb5 credentials: %s",
		  krb5_get_err_text(context, ret));
       goto end;
   }

   ret = krb5_cc_resolve(context, "MEMORY:", &ret_ccache);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
   	         "generating new memory ccache failed: %s",
 		 krb5_get_err_text(context, ret));
      goto end;
   }

   ret = krb5_cc_initialize(context, ret_ccache, principal);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
		 "krb5_cc_initialize() failed: %s",
		 krb5_get_err_text(context, ret));
      goto end;
   }

   ret = krb5_cc_store_cred(context, ret_ccache, &creds);
   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
		 "krb5_cc_store_cred() failed: %s",
		 krb5_get_err_text(context, ret));
      goto end;
   }
   *ccache = ret_ccache;
   ret_ccache = NULL;

end:
   krb5_free_cred_contents(context, &creds);
   if (ret_ccache)
      krb5_cc_destroy(context, ret_ccache);

   return ret;
}

static int
krb5_cache_cleanup(void *data)
{
   krb5_context context;
   krb5_ccache  cache;
   krb5_error_code problem;
   char *cache_name = (char *) data;

   problem = krb5_init_context(&context);
   if (problem) {
      /* ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL, "krb5_init_context() failed"); */
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   problem = krb5_cc_resolve(context, cache_name, &cache);
   if (problem) {
      /* log_error(APLOG_MARK, APLOG_ERR, 0, NULL, 
                "krb5_cc_resolve() failed (%s: %s)",
	        cache_name, krb5_get_err_text(context, problem)); */
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   krb5_cc_destroy(context, cache);
   krb5_free_context(context);
   return OK;
}

static int
create_krb5_ccache(krb5_context kcontext,
      		   request_rec *r,
		   kerb_auth_config *conf,
		   krb5_principal princ,
		   krb5_ccache *ccache)
{
   char *ccname;
   int fd;
   krb5_error_code problem;
   int ret;
   krb5_ccache tmp_ccache = NULL;

   ccname = apr_psprintf(r->pool, "FILE:%s/krb5cc_apache_XXXXXX", P_tmpdir);
   fd = mkstemp(ccname + strlen("FILE:"));
   if (fd < 0) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                 "mkstemp() failed: %s", strerror(errno));
      ret = HTTP_INTERNAL_SERVER_ERROR;
      goto end;
   }
   close(fd);

   problem = krb5_cc_resolve(kcontext, ccname, &tmp_ccache);
   if (problem) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                 "krb5_cc_resolve() failed: %s",
                 krb5_get_err_text(kcontext, problem));
      ret = HTTP_INTERNAL_SERVER_ERROR;
      unlink(ccname);
      goto end;
   }

   problem = krb5_cc_initialize(kcontext, tmp_ccache, princ);
   if (problem) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
		 "Cannot initialize krb5 ccache %s: krb5_cc_initialize() failed: %s",
		 ccname, krb5_get_err_text(kcontext, problem));
      ret = HTTP_INTERNAL_SERVER_ERROR;
      goto end;
   }

   apr_table_setn(r->subprocess_env, "KRB5CCNAME", ccname);
   apr_pool_cleanup_register(r->pool, ccname, krb5_cache_cleanup,
	 		     apr_pool_cleanup_null);

   *ccache = tmp_ccache;
   tmp_ccache = NULL;

   ret = OK;

end:
   if (tmp_ccache)
      krb5_cc_destroy(kcontext, tmp_ccache);

   return ret;
}

static int
store_krb5_creds(krb5_context kcontext,
      		 request_rec *r,
		 kerb_auth_config *conf,
		 krb5_ccache delegated_cred)
{
   char errstr[1024];
   krb5_error_code problem;
   krb5_principal princ;
   krb5_ccache ccache;
   int ret;

   problem = krb5_cc_get_principal(kcontext, delegated_cred, &princ);
   if (problem) {
      snprintf(errstr, sizeof(errstr), "krb5_cc_get_principal() failed: %s",
	       krb5_get_err_text(kcontext, problem));
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   ret = create_krb5_ccache(kcontext, r, conf, princ, &ccache);
   if (ret) {
      krb5_free_principal(kcontext, princ);
      return ret;
   }

#ifdef HEIMDAL
   problem = krb5_cc_copy_cache(kcontext, delegated_cred, ccache);
#else
   problem = krb5_cc_copy_creds(kcontext, delegated_cred, ccache);
#endif
   krb5_free_principal(kcontext, princ);
   if (problem) {
      snprintf(errstr, sizeof(errstr), "Failed to store credentials: %s",
	       krb5_get_err_text(kcontext, problem));
      krb5_cc_destroy(kcontext, ccache);
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   krb5_cc_close(kcontext, ccache);
   return OK;
}


static char* 
apply_httpd_mapping( request_rec *r, 
		     const char *input_string,
		     char *mapping )
{

    char *regex = NULL;
    char *target = NULL;
    char *tmp = NULL;
    char arg[10];
    char *result = NULL;
    const char *saveresult = apr_pstrdup(r->pool, input_string);
    ap_regex_t *preg = NULL;
    ap_regmatch_t *matches = NULL;
    int nmatches = 0;   
    long int i = 0;

    if ( mapping && strlen(mapping) > 0 ) {

	/* 
	   the mapping will consist of a regular expression match and a target separated by space
	   such as: DOMAIN\\(.*) $1@DOMAIN.COM
	        or: (.*)@DOMAIN\.COM DOMAIN\\$1
	*/
      
	regex = ap_getword_white_nc( r->pool, &mapping );
	target = ap_getword_white_nc( r->pool, &mapping );

	while ( regex && *regex && target && *target ) {
	    log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
		       "Trying httpd mapping %s => %s", regex, target );

	    nmatches = 1;
	    tmp = regex;
	    while( *tmp )
	      if ( *tmp++ == '(' )
		nmatches++;
	    
	    matches = (ap_regmatch_t*)apr_pcalloc( r->pool, nmatches * sizeof( ap_regex_t ) );
	    preg = ap_pregcomp( r->pool, regex, AP_REG_EXTENDED | AP_REG_ICASE );	   
	    
	    if ( !preg ) {
		log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
			   "Failed to compile regular expression [%s]", regex );
	    }
	    else {
		if ( ap_regexec(preg, input_string, nmatches, matches, 0) != 0 ) {
		    regex = ap_getword_white_nc( r->pool, &mapping );
		    target = ap_getword_white_nc( r->pool, &mapping );
		}
		else {
		    saveresult = result = (char*)apr_pcalloc( r->pool, strlen(input_string) + strlen(target) );
		    tmp = target;
		    while ( *tmp ) {
			if ( *tmp == '$' ) {
			    tmp++;
			    i = 0;
			    while ( isdigit( (int) *tmp ) && (i < (sizeof( arg )-1))  ) {
				arg[i++] = *tmp;
				tmp++;
			    }
			    arg[i] = '\0';
			    i = strtol(arg, NULL, 10);
			    if ( i >= 0 && i < nmatches && matches[i].rm_so != -1 ) {
				memcpy( result, input_string + matches[i].rm_so, matches[i].rm_eo - matches[i].rm_so );
				result += matches[i].rm_eo - matches[i].rm_so;
			    }
			}
			else {
			    *result++ = *tmp++;	      
			}		    
		    }
		    *result = '\0';
		    log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
			       "[%s] was mapped to [%s]", input_string, saveresult );
		    break;
		}
	    }	     
	}
    }
    
    return (char*)saveresult;
}

static char*
map_principal_to_domain_user( request_rec *r, 
			      const krb5_context *pkcontext, 
			      const char* principal)
{
    char *mapping = NULL;
    krb5_appdefault_string(*pkcontext, "httpd", NULL, "reverse_mappings", "", &mapping);
    return apply_httpd_mapping( r, principal, mapping );
}

static char*
map_domain_user_to_principal( request_rec *r,
			      const krb5_context *pkcontext,
			      char *login_creds )
{
    char *mapping = NULL;
    krb5_appdefault_string(*pkcontext, "httpd", NULL, "mappings", "", &mapping);
    return apply_httpd_mapping( r, login_creds, mapping );
}

static
char* get_first_principal_name( request_rec *r, krb5_context kcontext, krb5_keytab kt )
{
     krb5_keytab_entry entry;
     krb5_kt_cursor cursor;
     char *pname;
     char *ret = NULL;
     int code;
     
     if ((code = krb5_kt_start_seq_get(kcontext, kt, &cursor))) {
	 log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
		    "krb5_kt_start_seq_get() failed %s",
		    krb5_get_err_text(kcontext, code) );
	 return NULL;
     }

     if ( (code = krb5_kt_next_entry(kcontext, kt, &entry, &cursor)) == 0) {
	 if ((code = krb5_unparse_name(kcontext, entry.principal, &pname))) {
	     log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
			"krb5_unparse_name() failed %s",
			krb5_get_err_text(kcontext, code) );	     
	 }
     }
     else
     {
	 log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
		    "krb5_kt_ntext_entry() failed %s",
		    krb5_get_err_text(kcontext, code) );	 
     }

     code = krb5_kt_end_seq_get(kcontext, kt, &cursor);    
     
     if ( pname )
     {
	 ret = strdup( pname );
	 krb5_free_unparsed_name(kcontext, pname);	 
     }

     return ret;
}

static int
authenticate_user_krb5pwd(request_rec *r,
                          kerb_auth_config *conf,
                          const char *auth_line)
{
   const char      *sent_pw = NULL; 
   char            *sent_name = NULL;
   const char      *realms = NULL;
   const char      *realm = NULL;
   krb5_context    kcontext = NULL;
   krb5_error_code code;
   krb5_principal  client = NULL;
   krb5_principal  server = NULL;
   krb5_ccache     ccache = NULL;
   krb5_keytab     keytab = NULL;
   int             ret;
   char            *name = NULL;
   int             all_principals_unkown;
   char            *p = NULL;
   char            *tmp = NULL;  

   code = krb5_init_context(&kcontext);
   if (code) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
    		 "Cannot initialize Kerberos5 context (%d)", code);
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   sent_pw = ap_pbase64decode(r->pool, auth_line);
   sent_name = ap_getword (r->pool, &sent_pw, ':');

   if (sent_pw == NULL || *sent_pw == '\0') {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "empty passwords are not accepted");
      ret = HTTP_UNAUTHORIZED;
      goto end;
   }

   if (conf->krb_5_keytab)
      krb5_kt_resolve(kcontext, conf->krb_5_keytab, &keytab);

   if (conf->krb_service_name && strchr(conf->krb_service_name, '/') != NULL)
      ret = krb5_parse_name (kcontext, conf->krb_service_name, &server);
   else if ( !conf->krb_service_name && keytab && (name = get_first_principal_name( r, kcontext, keytab )) )
       ret = krb5_parse_name(kcontext, name, &server);
   else
      ret = krb5_sname_to_principal(kcontext, ap_get_server_name(r),
	    			    (conf->krb_service_name) ? conf->krb_service_name : SERVICE_NAME,
				    KRB5_NT_SRV_HST, &server);

   if (ret) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "Error parsing server name (%s): %s",
		 (conf->krb_service_name) ? conf->krb_service_name : SERVICE_NAME,
		 krb5_get_err_text(kcontext, ret));
      ret = HTTP_UNAUTHORIZED;
      goto end;
   }

   if ( !name )
   {
       code = krb5_unparse_name(kcontext, server, &name);
       if (code) {
	   log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
		      "krb5_unparse_name() failed: %s",
		      krb5_get_err_text(kcontext, code));
	   ret = HTTP_UNAUTHORIZED;
	   goto end;
       }
   }

   log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "Using %s as server principal for password verification", name);
   krb5_free_unparsed_name(kcontext, name);
   name = NULL;

   sent_name = map_domain_user_to_principal( r, &kcontext, sent_name );

   p = strchr(sent_name, '@');
   if (p) {
       *p++ = '\0';
   } else if ( (p = strchr(sent_name, '\\' )) ) {
       /* perhaps we have realm\user rather than user@realm */ 
       *p++ = '\0';
       tmp = p;
       p = sent_name;
       sent_name = tmp;
   }

   if (p) {
      if (conf->krb_auth_realms && !ap_find_token(r->pool, conf->krb_auth_realms, p)) {
	 log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	            "Specified realm `%s' not allowed by configuration", p);
         ret = HTTP_UNAUTHORIZED;
         goto end;
      }
      tmp = p;
      while( *tmp ) { 
	  *tmp = toupper(*tmp);
	  tmp++;
      }
      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "Received user=%s realm=%s" , sent_name, p );
   }

   realms = (p) ? p : conf->krb_auth_realms;
   all_principals_unkown = 1;
   do {
      name = (char *) sent_name;
      if (realms && (realm = ap_getword_white(r->pool, &realms)))
	 name = apr_psprintf(r->pool, "%s@%s", sent_name, realm);

      if (client) {
	 krb5_free_principal(kcontext, client);
	 client = NULL;
      }

      code = krb5_parse_name(kcontext, name, &client);
      if (code) {
	 log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	            "krb5_parse_name() failed: %s",
		    krb5_get_err_text(kcontext, code));
	 continue;
      }

      code = verify_krb5_user(r, kcontext, client, sent_pw,
	    		      server, keytab, conf->krb_verify_kdc, &ccache);
      if (!conf->krb_authoritative && code) {
	 /* if we're not authoritative, we allow authentication to pass on
	  * to another modules if (and only if) the user is not known to us */
	 if (all_principals_unkown && code != KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN)
	    all_principals_unkown = 0;
      }

      if (code == 0)
	 break;

      /* ap_getword_white() used above shifts the parameter, so it's not
         needed to touch the realms variable */
   } while (realms && *realms);

   memset((char *)sent_pw, 0, strlen(sent_pw));

   if (code) {
      if (!conf->krb_authoritative && all_principals_unkown == 1 && code == KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN)
	 ret = DECLINED;
      else
	 ret = HTTP_UNAUTHORIZED;

      goto end;
   }

   code = krb5_unparse_name(kcontext, client, &name);
   if (code) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "krb5_unparse_name() failed: %s",
	         krb5_get_err_text(kcontext, code));
      ret = HTTP_UNAUTHORIZED;
      goto end;
   }

   tmp = map_principal_to_domain_user(r, &kcontext, name);

   log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	      "Setting the request user to %s",
	      tmp ? tmp : "(NULL)");

   MK_USER = tmp;
   MK_AUTH_TYPE = "Basic";
   krb5_free_unparsed_name(kcontext, name);

   if (conf->krb_save_credentials)
      store_krb5_creds(kcontext, r, conf, ccache);

   ret = OK;

end:
   log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	      "kerb_authenticate_user_krb5pwd ret=%d user=%s authtype=%s",
	      ret, (MK_USER)?MK_USER:"(NULL)", (MK_AUTH_TYPE)?MK_AUTH_TYPE:"(NULL)");
   if (client)
      krb5_free_principal(kcontext, client);
   if (server)
      krb5_free_principal(kcontext, server);
   if (ccache)
      krb5_cc_destroy(kcontext, ccache);
   if (keytab)
      krb5_kt_close(kcontext, keytab);
   krb5_free_context(kcontext);

   return ret;
}

/*********************************************************************
 * GSSAPI Authentication
 ********************************************************************/

static const char *
get_gss_error(MK_POOL *p, OM_uint32 err_maj, OM_uint32 err_min, char *prefix)
{
   OM_uint32 maj_stat, min_stat; 
   OM_uint32 msg_ctx = 0;
   gss_buffer_desc status_string;
   char *err_msg;

   err_msg = apr_pstrdup(p, prefix);
   do {
      maj_stat = gss_display_status (&min_stat,
	                             err_maj,
				     GSS_C_GSS_CODE,
				     GSS_C_NO_OID,
				     &msg_ctx,
				     &status_string);
      if (GSS_ERROR(maj_stat))
	 break;
      err_msg = apr_pstrcat(p, err_msg, ": ", (char*) status_string.value, NULL);
      gss_release_buffer(&min_stat, &status_string);
      
      maj_stat = gss_display_status (&min_stat,
	                             err_min,
				     GSS_C_MECH_CODE,
				     GSS_C_NULL_OID,
				     &msg_ctx,
				     &status_string);
      if (!GSS_ERROR(maj_stat)) {
	 err_msg = apr_pstrcat(p, err_msg,
	                      " (", (char*) status_string.value, ")", NULL);
	 gss_release_buffer(&min_stat, &status_string);
      }
   } while (!GSS_ERROR(maj_stat) && msg_ctx != 0);

   return err_msg;
}

static int
store_gss_creds(request_rec *r, kerb_auth_config *conf, char *princ_name,
                gss_cred_id_t delegated_cred)
{
   OM_uint32 maj_stat, min_stat;
   krb5_principal princ = NULL;
   krb5_ccache ccache = NULL;
   krb5_error_code problem;
   krb5_context context;
   int ret = HTTP_INTERNAL_SERVER_ERROR;

   problem = krb5_init_context(&context);
   if (problem) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Cannot initialize krb5 context");
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   problem = krb5_parse_name(context, princ_name, &princ);
   if (problem) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
	 "Cannot parse delegated username (%s)", krb5_get_err_text(context, problem));
      goto end;
   }

   problem = create_krb5_ccache(context, r, conf, princ, &ccache);
   if (problem) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	 "Cannot create krb5 ccache (%s)", krb5_get_err_text(context, problem));
      goto end;
   }

   maj_stat = gss_krb5_copy_ccache(&min_stat, delegated_cred, ccache);
   if (GSS_ERROR(maj_stat)) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	 "Cannot store delegated credential (%s)", 
	 get_gss_error(r->pool, maj_stat, min_stat, "gss_krb5_copy_ccache"));
      goto end;
   }

   krb5_cc_close(context, ccache);
   ccache = NULL;
   ret = 0;

end:
   if (princ)
      krb5_free_principal(context, princ);
   if (ccache)
      krb5_cc_destroy(context, ccache);
   krb5_free_context(context);
   return ret;
}

static int
get_gss_creds(request_rec *r,
              kerb_auth_config *conf,
	      gss_cred_id_t *server_creds)
{
   gss_buffer_desc token = GSS_C_EMPTY_BUFFER;
   OM_uint32 major_status, minor_status, minor_status2;
   gss_name_t server_name = GSS_C_NO_NAME;
   char buf[1024];
   int have_server_princ;


   /* match any prinicipal name from the krb5.keytab */
   if ( !conf->krb_service_name )
   {
       *server_creds = GSS_C_NO_CREDENTIAL;
       return 0;
   }

   have_server_princ = conf->krb_service_name && strchr(conf->krb_service_name, '/') != NULL;
   if (have_server_princ)
      strncpy(buf, conf->krb_service_name, sizeof(buf));
   else
      snprintf(buf, sizeof(buf), "%s@%s",
	       (conf->krb_service_name) ? conf->krb_service_name : SERVICE_NAME,
	       ap_get_server_name(r));

   token.value = buf;
   token.length = strlen(buf) + 1;

   major_status = gss_import_name(&minor_status, &token,
	 			  (gss_OID) ((have_server_princ) ? GSS_KRB5_NT_PRINCIPAL_NAME : GSS_C_NT_HOSTBASED_SERVICE),
				  &server_name);
   memset(&token, 0, sizeof(token));
   if (GSS_ERROR(major_status)) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "%s", get_gss_error(r->pool, major_status, minor_status,
		 "gss_import_name() failed"));
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   major_status = gss_display_name(&minor_status, server_name, &token, NULL);
   if (GSS_ERROR(major_status)) {
      /* Perhaps we could just ignore this error but it's safer to give up now,
         I think */
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "%s", get_gss_error(r->pool, major_status, minor_status,
		                     "gss_display_name() failed"));
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "Acquiring creds for %s",
	      token.value);
   gss_release_buffer(&minor_status, &token);
   
   major_status = gss_acquire_cred(&minor_status, server_name, GSS_C_INDEFINITE,
			           GSS_C_NO_OID_SET, GSS_C_ACCEPT,
				   server_creds, NULL, NULL);
   gss_release_name(&minor_status2, &server_name);
   if (GSS_ERROR(major_status)) {
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	         "%s", get_gss_error(r->pool, major_status, minor_status,
		 		     "gss_acquire_cred() failed"));
      return HTTP_INTERNAL_SERVER_ERROR;
   }

   return 0;
}

static int
cmp_gss_type(gss_buffer_t token, gss_OID oid)
{
   unsigned char *p;
   size_t len;

   if (token->length == 0)
      return GSS_S_DEFECTIVE_TOKEN;

   p = token->value;
   if (*p++ != 0x60)
      return GSS_S_DEFECTIVE_TOKEN;
   len = *p++;
   if (len & 0x80) {
      if ((len & 0x7f) > 4)
	 return GSS_S_DEFECTIVE_TOKEN;
      p += len & 0x7f;
   }
   if (*p++ != 0x06)
      return GSS_S_DEFECTIVE_TOKEN;

   if (((OM_uint32) *p++) != oid->length)
      return GSS_S_DEFECTIVE_TOKEN;

   return memcmp(p, oid->elements, oid->length);
}

struct auth_krb_context {
  gss_ctx_id_t gss_context;
};

static int krb_pre_conn(conn_rec *c, void *csd) {
    struct auth_krb_context *ctxt = apr_pcalloc(c->pool,
	    sizeof(struct auth_krb_context));

    ctxt->gss_context = GSS_C_NO_CONTEXT;

    ap_set_module_config(c->conn_config, &auth_kerb_module, ctxt);

    return OK;

}

static struct auth_krb_context * get_context(struct conn_rec * connection)
{
    struct auth_krb_context *retval = NULL;

    retval = (struct auth_krb_context *)ap_get_module_config(connection->conn_config, &auth_kerb_module);

    return retval;
}

static int
authenticate_user_gss(request_rec *r, kerb_auth_config *conf,
		      const char *auth_line, char **negotiate_ret_value)
{
  OM_uint32 major_status, minor_status, minor_status2;
  gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
  gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
  const char *auth_param = NULL;
  int ret;
  gss_name_t client_name = GSS_C_NO_NAME;
  gss_cred_id_t delegated_cred = GSS_C_NO_CREDENTIAL;
  OM_uint32 (KRB5_LIB_FUNCTION *accept_sec_token)
     			 (OM_uint32 *, gss_ctx_id_t *, const gss_cred_id_t,
			 const gss_buffer_t, const gss_channel_bindings_t,
			 gss_name_t *, gss_OID *, gss_buffer_t, OM_uint32 *,
			 OM_uint32 *, gss_cred_id_t *);
  gss_OID_desc spnego_oid;
  gss_cred_id_t server_creds = GSS_C_NO_CREDENTIAL;
  char* tmp = NULL;
  krb5_context kcontext = NULL;
  krb5_error_code code;
  struct auth_krb_context *ctxt = get_context(r->connection);
  int free_context = 1;

  *negotiate_ret_value = "\0";

  spnego_oid.length = 6;
  spnego_oid.elements = (void *)"\x2b\x06\x01\x05\x05\x02";

  if (conf->krb_5_keytab) {
     char *ktname;
     /* we don't use the ap_* calls here, since the string passed to putenv()
      * will become part of the enviroment and shouldn't be free()ed by apache
      */
     ktname = malloc(strlen("KRB5_KTNAME=") + strlen(conf->krb_5_keytab) + 1);
     if (ktname == NULL) {
	log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "malloc() failed: not enough memory");
	ret = HTTP_INTERNAL_SERVER_ERROR;
	goto end;
     }
     sprintf(ktname, "KRB5_KTNAME=%s", conf->krb_5_keytab);
     putenv(ktname);
#ifdef HEIMDAL
     /* Seems to be also supported by latest MIT */
     gsskrb5_register_acceptor_identity(conf->krb_5_keytab);
#endif
  }

  ret = get_gss_creds(r, conf, &server_creds);
  if (ret)
     goto end;

  /* ap_getword() shifts parameter */
  auth_param = ap_getword_white(r->pool, &auth_line);
  if (auth_param == NULL) {
     log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	        "No Authorization parameter in request from client");
     ret = HTTP_UNAUTHORIZED;
     goto end;
  }

  input_token.length = apr_base64_decode_len(auth_param) + 1;
  input_token.value = apr_pcalloc(r->connection->pool, input_token.length);
  if (input_token.value == NULL) {
     log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	   	"ap_pcalloc() failed (not enough memory)");
     ret = HTTP_INTERNAL_SERVER_ERROR;
     goto end;
  }
  input_token.length = apr_base64_decode(input_token.value, auth_param);

#ifdef GSSAPI_SUPPORTS_SPNEGO
  accept_sec_token = gss_accept_sec_context;
#else
  accept_sec_token = (cmp_gss_type(&input_token, &spnego_oid) == 0) ?
     			gss_accept_sec_context_spnego : gss_accept_sec_context;
#endif

  log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "Verifying client data using %s",
	     (accept_sec_token == gss_accept_sec_context)
	       ? "KRB5 GSS-API"
	       : "SPNEGO GSS-API");

  major_status = accept_sec_token(&minor_status,
				  &ctxt->gss_context,
				  server_creds,
				  &input_token,
				  GSS_C_NO_CHANNEL_BINDINGS,
				  &client_name,
				  NULL,
				  &output_token,
				  NULL,
				  NULL,
				  &delegated_cred);
  log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	     "Verification returned code 0x%x", major_status);

  if (output_token.length && ( !is_basic_auth_on(conf) || major_status & GSS_S_CONTINUE_NEEDED )) {
     char *token = NULL;
     size_t len;
     
     len = apr_base64_encode_len(output_token.length) + 1;
     token = apr_pcalloc(r->connection->pool, len + 1);
     if (token == NULL) {
	log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	           "ap_pcalloc() failed (not enough memory)");
        ret = HTTP_INTERNAL_SERVER_ERROR;
	gss_release_buffer(&minor_status2, &output_token);
	goto end;
     }
     apr_base64_encode(token, output_token.value, output_token.length);
     token[len] = '\0';
     *negotiate_ret_value = token;
     log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	        "GSS-API token of length %d bytes will be sent back",
		output_token.length);
     gss_release_buffer(&minor_status2, &output_token);
  }
  else
      /* don't offer more Kerberos */
      *negotiate_ret_value = NULL;

  switch (major_status) {
  case GSS_S_COMPLETE:
      major_status = gss_display_name(&minor_status, client_name, &output_token, NULL);
      gss_release_name(&minor_status, &client_name); 
      if (GSS_ERROR(major_status)) {
          log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	             "%s", get_gss_error(r->pool, major_status, minor_status,
		     "gss_display_name() failed"));
          ret = HTTP_INTERNAL_SERVER_ERROR;
          goto end;
      }
 
      code = krb5_init_context(&kcontext);
      if (code) {
          log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
    		 "Cannot initialize Kerberos5 context (%d)", code);
          ret = HTTP_INTERNAL_SERVER_ERROR;
          goto end;
      }

      tmp = map_principal_to_domain_user( r, &kcontext, output_token.value );

      log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
             "Setting the request user to %s",
	     tmp ? tmp : "(NULL)");

      MK_AUTH_TYPE = MECH_NEGOTIATE;
      MK_USER = tmp;

      if (conf->krb_save_credentials && delegated_cred != GSS_C_NO_CREDENTIAL)
         store_gss_creds(r, conf, (char *)output_token.value, delegated_cred);

      gss_release_buffer(&minor_status, &output_token);

      ret = OK;
      break;
  case GSS_S_CONTINUE_NEEDED:
      ret = HTTP_UNAUTHORIZED;
      free_context = 0;
      break;
  default:
      log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
	        "%s", get_gss_error(r->pool, major_status, minor_status,
		                    "gss_accept_sec_context() failed"));
      /* Don't offer the Negotiate method again if call to GSS layer failed */
      /* *negotiate_ret_value = NULL; */
      ret = HTTP_UNAUTHORIZED;
      break;
  }

end:
  if (delegated_cred)
     gss_release_cred(&minor_status, &delegated_cred);

  if (output_token.length) 
     gss_release_buffer(&minor_status, &output_token);

  if (client_name != GSS_C_NO_NAME)
     gss_release_name(&minor_status, &client_name);

  if (server_creds != GSS_C_NO_CREDENTIAL)
     gss_release_cred(&minor_status, &server_creds);

  if (free_context && ctxt->gss_context != GSS_C_NO_CONTEXT)
     gss_delete_sec_context(&minor_status, &ctxt->gss_context, GSS_C_NO_BUFFER);

  if ( kcontext )
     krb5_free_context(kcontext);

  return ret;
}
#endif /* KRB5 */

static int
already_succeeded(request_rec *r, int *pret)
{
    const char* strret = NULL;
    request_rec *mainr = NULL;

    if ( ap_is_initial_req(r) )
	return 0;

    mainr = r->main;    

    if ( mainr  )
    {
        while(mainr->main)
	    mainr = mainr->main;

	strret = MK_TABLE_GET( mainr->subprocess_env, "KRBAUTHRET" );

	if ( strret && strlen(strret) > 0 && (*pret = atoi(strret)) == OK )
	{	    
	    MK_USER = apr_pstrdup( r->pool, MK_MAIN_USER );
	    MK_AUTH_TYPE = apr_pstrdup( r->pool, MK_MAIN_AUTH_TYPE );
	    return 1;
	}
    }

    return 0;
}

static void
set_kerb_auth_headers(request_rec *r, const kerb_auth_config *conf,
      		      int use_krb4, int use_krb5pwd, char *negotiate_ret_value)
{
   const char *auth_name = NULL;
   int set_basic = 0;
   int basic_on = 0;
   char *negoauth_param;
   const char *header_name = 
      (r->proxyreq == PROXYREQ_PROXY) ? "Proxy-Authenticate" : "WWW-Authenticate";

   basic_on = is_basic_auth_on( conf );

   /* get the user realm specified in .htaccess */
   auth_name = ap_auth_name(r);

   /* XXX should the WWW-Authenticate header be cleared first?
    * apache in the proxy mode should retain client's authN headers? */
#ifdef KRB5
   if (negotiate_ret_value != NULL && conf->krb_method_gssapi) {
      negoauth_param = (*negotiate_ret_value == '\0') ? MECH_NEGOTIATE :
	          apr_pstrcat(r->pool, MECH_NEGOTIATE " ", negotiate_ret_value, NULL);
      apr_table_add(r->err_headers_out, header_name, negoauth_param);
   }
   if ( use_krb5pwd && basic_on ) {
      apr_table_add(r->err_headers_out, header_name,
		   apr_pstrcat(r->pool, "Basic realm=\"", auth_name, "\"", NULL));
      set_basic = 1;
   }
#endif

#ifdef KRB4
   if (!set_basic && ( use_krb4 && basic_on ) )
      apr_table_add(r->err_headers_out, header_name,
		  apr_pstrcat(r->pool, "Basic realm=\"", auth_name, "\"", NULL));
#endif
}

static int
kerb_authenticate_user(request_rec *r)
{
   kerb_auth_config *conf = 
      (kerb_auth_config *) ap_get_module_config(r->per_dir_config,
						&auth_kerb_module);
   const char *auth_type = NULL;
   const char *auth_line = NULL;
   const char *type = NULL;
   int use_krb5 = 0, use_krb4 = 0;
   int ret;
   char *negotiate_ret_value = NULL;

   /* get the type specified in .htaccess */
   type = ap_auth_type(r);

   log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
	      "[pid %d] kerb_authenticate_user entered with user %s and auth_type %s",
	      getpid(), (MK_USER)?MK_USER:"(NULL)",type?type:"(NULL)");

   if (type && strcasecmp(type, "Kerberos") == 0)
      use_krb5 = use_krb4 = 1;
   else if(type && strcasecmp(type, "KerberosV5") == 0)
      use_krb5 = 1;
   else if(type && strcasecmp(type, "KerberosV4") == 0)
      use_krb4 = 1;
   else
      return DECLINED;

#if 0
   if (conf->krb_ssl_preauthentication) {
      const char *ssl_client_verify = ssl_var_lookup(r->pool, r->server,
	    	r->connection, r, "SSL_CLIENT_VERIFY");

      if (ssl_client_verify && strcmp(ssl_client_verify, "SUCCESS") == 0)
	 return OK;
   }
#endif

   /* get what the user sent us in the HTTP header */
   auth_line = MK_TABLE_GET(r->headers_in, (r->proxyreq == PROXYREQ_PROXY)
	                                    ? "Proxy-Authorization"
					    : "Authorization");
   if (!auth_line) {
      set_kerb_auth_headers(r, conf, use_krb4, use_krb5, 
	                    (use_krb5) ? "\0" : NULL);
      return HTTP_UNAUTHORIZED;
   }
   auth_type = ap_getword_white(r->pool, &auth_line);

   /* If we are delegating Basic to other modules, DECLINE the request */
   if (conf->krb_delegate_basic &&
#ifdef KRB5
       !conf->krb_method_k5pass &&
#endif
#ifdef KRB4
       !conf->krb_method_k4pass &&
#endif
       (strcasecmp(auth_type, "Basic") == 0))
       return DECLINED;

   if (already_succeeded(r, &ret))
       return ret;

   ret = HTTP_UNAUTHORIZED;

#ifdef KRB5
   if (use_krb5 && conf->krb_method_gssapi &&
       strcasecmp(auth_type, MECH_NEGOTIATE) == 0) {
      ret = authenticate_user_gss(r, conf, auth_line, &negotiate_ret_value);
   } else if (use_krb5 && conf->krb_method_k5pass &&
	      strcasecmp(auth_type, "Basic") == 0) {
       ret = authenticate_user_krb5pwd(r, conf, auth_line);
   }
#endif

#ifdef KRB4
   if (ret == HTTP_UNAUTHORIZED && use_krb4 && conf->krb_method_k4pass &&
       strcasecmp(auth_type, "Basic") == 0)
      ret = authenticate_user_krb4pwd(r, conf, auth_line);
#endif

   if (ret == HTTP_UNAUTHORIZED || negotiate_ret_value)
      set_kerb_auth_headers(r, conf, use_krb4, use_krb5, negotiate_ret_value);

   if ( ap_is_initial_req(r) )
       apr_table_setn( r->subprocess_env, "KRBAUTHRET", apr_psprintf(r->pool, "%d", ret) );

   return ret;
}

krb5_error_code krb5_rc_resolve_full (krb5_context,krb5_rcache *,char *);
krb5_error_code krb5_rc_destroy (krb5_context, krb5_rcache);

int
have_rcache_type(const char *type)
{
   krb5_error_code ret;
   krb5_context context;
   krb5_rcache id = NULL;
   int found;

   ret = krb5_init_context(&context);
   if (ret)
      return 0;

   ret = krb5_rc_resolve_full(context, &id, "none:");
   found = (ret == 0);

   if (ret == 0)
      krb5_rc_destroy(context, id);
   krb5_free_context(context);

   return found;
}

/*************************************************************************** 
 Module Setup/Configuration
 ***************************************************************************/
#ifndef STANDARD20_MODULE_STUFF
static void
kerb_module_init(server_rec *dummy, pool *p)
{
#ifndef HEIMDAL
   /* Suppress the MIT replay cache.  Requires MIT Kerberos 1.4.0 or later.
      1.3.x are covered by the hack overiding the replay calls */
   if (getenv("KRB5RCACHETYPE") == NULL && have_rcache_type("none"))
      putenv(strdup("KRB5RCACHETYPE=none"));
#endif
}

module MODULE_VAR_EXPORT auth_kerb_module = {
	STANDARD_MODULE_STUFF,
	kerb_module_init,		/*      module initializer            */
	kerb_dir_create_config,		/*      per-directory config creator  */
	NULL,				/*      per-directory config merger   */
	NULL,				/*      per-server    config creator  */
	NULL,				/*      per-server    config merger   */
	kerb_auth_cmds,			/*      command table                 */
	NULL,				/* [ 9] content handlers              */
	NULL,				/* [ 2] URI-to-filename translation   */
	kerb_authenticate_user,		/* [ 5] check/validate user_id        */
	NULL,				/* [ 6] check user_id is valid *here* */
	NULL,				/* [ 4] check access by host address  */
	NULL,				/* [ 7] MIME type checker/setter      */
	NULL,				/* [ 8] fixups                        */
	NULL,				/* [10] logger                        */
	NULL,				/* [ 3] header parser                 */
	NULL,				/*      process initialization        */
	NULL,				/*      process exit/cleanup          */
	NULL				/* [ 1] post read_request handling    */
#ifdef EAPI
       ,NULL,				/* EAPI: add_module		      */
	NULL,				/* EAPI: remove_module		      */
	NULL,				/* EAPI: rewrite_command	      */
	NULL				/* EAPI: new_connection		      */
#endif
};
#else
static int
kerb_init_handler(apr_pool_t *p, apr_pool_t *plog,
      		  apr_pool_t *ptemp, server_rec *s)
{
   ap_add_version_component(p, "mod_auth_kerb/" MODAUTHKERB_VERSION);
#ifndef HEIMDAL
   /* Suppress the MIT replay cache.  Requires MIT Kerberos 1.4.0 or later.
      1.3.x are covered by the hack overiding the replay calls */
   if (getenv("KRB5RCACHETYPE") == NULL && have_rcache_type("none"))
      putenv(strdup("KRB5RCACHETYPE=none"));
#endif
   
   return OK;
}

static void
kerb_register_hooks(apr_pool_t *p)
{
   ap_hook_pre_connection(krb_pre_conn, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_post_config(kerb_init_handler, NULL, NULL, APR_HOOK_MIDDLE);
   ap_hook_check_user_id(kerb_authenticate_user, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA auth_kerb_module =
{
   STANDARD20_MODULE_STUFF,
   kerb_dir_create_config,	/* create per-dir    conf structures  */
   NULL,			/* merge  per-dir    conf structures  */
   NULL,			/* create per-server conf structures  */
   NULL,			/* merge  per-server conf structures  */
   kerb_auth_cmds,		/* table of configuration directives  */
   kerb_register_hooks		/* register hooks                     */
};
#endif
