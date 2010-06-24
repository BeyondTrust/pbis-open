/* SASL server API implementation
 * Rob Siemborski
 * Tim Martin
 * $Id: sasldb.c,v 1.11 2006/04/03 10:58:19 mel Exp $
 */
/* 
 * Copyright (c) 1998-2003 Carnegie Mellon University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any other legal
 *    details, please contact  
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>

/* sasldb stuff */

#include <stdio.h>

#include "sasl.h"
#include "saslutil.h"
#include "saslplug.h"
#include "../sasldb/sasldb.h"

#include "plugin_common.h"

static void sasldb_auxprop_lookup(void *glob_context __attribute__((unused)),
				  sasl_server_params_t *sparams,
				  unsigned flags,
				  const char *user,
				  unsigned ulen) 
{
    char *userid = NULL;
    char *realm = NULL;
    const char *user_realm = NULL;
    int ret;
    const struct propval *to_fetch, *cur;
    char value[8192];
    size_t value_len;
    char *user_buf;
    
    if(!sparams || !user) return;

    user_buf = sparams->utils->malloc(ulen + 1);
    if(!user_buf) {
	goto done;
    }

    memcpy(user_buf, user, ulen);
    user_buf[ulen] = '\0';

    if(sparams->user_realm) {
	user_realm = sparams->user_realm;
    } else {
	user_realm = sparams->serverFQDN;
    }

    ret = _plug_parseuser(sparams->utils, &userid, &realm, user_realm,
			  sparams->serverFQDN, user_buf);
    if(ret != SASL_OK) goto done;

    to_fetch = sparams->utils->prop_get(sparams->propctx);
    if(!to_fetch) goto done;

    for(cur = to_fetch; cur->name; cur++) {
	const char *realname = cur->name;
	
	/* Only look up properties that apply to this lookup! */
	if(cur->name[0] == '*' && (flags & SASL_AUXPROP_AUTHZID)) continue;
	if(!(flags & SASL_AUXPROP_AUTHZID)) {
	    if(cur->name[0] != '*') continue;
	    else realname = cur->name + 1;
	}
	
	/* If it's there already, we want to see if it needs to be
	 * overridden */
	if(cur->values && !(flags & SASL_AUXPROP_OVERRIDE))
	    continue;
	else if(cur->values)
	    sparams->utils->prop_erase(sparams->propctx, cur->name);
	    
	ret = _sasldb_getdata(sparams->utils,
			      sparams->utils->conn, userid, realm,
			      realname, value, sizeof(value), &value_len);
	if(ret != SASL_OK) {
	    /* We didn't find it, leave it as not found */
	    continue;
	}

	sparams->utils->prop_set(sparams->propctx, cur->name,
				 value, (unsigned) value_len);
    }

 done:
    if (userid) sparams->utils->free(userid);
    if (realm)  sparams->utils->free(realm);
    if (user_buf) sparams->utils->free(user_buf);
}

static int sasldb_auxprop_store(void *glob_context __attribute__((unused)),
				sasl_server_params_t *sparams,
				struct propctx *ctx,
				const char *user,
				unsigned ulen) 
{
    char *userid = NULL;
    char *realm = NULL;
    const char *user_realm = NULL;
    int ret = SASL_FAIL;
    int tmp_res;
    const struct propval *to_store, *cur;
    char *user_buf;

    /* just checking if we are enabled */
    if(!ctx) return SASL_OK;
    
    if(!sparams || !user) return SASL_BADPARAM;

    user_buf = sparams->utils->malloc(ulen + 1);
    if(!user_buf) {
	ret = SASL_NOMEM;
	goto done;
    }

    memcpy(user_buf, user, ulen);
    user_buf[ulen] = '\0';

    if(sparams->user_realm) {
	user_realm = sparams->user_realm;
    } else {
	user_realm = sparams->serverFQDN;
    }

    ret = _plug_parseuser(sparams->utils, &userid, &realm, user_realm,
			  sparams->serverFQDN, user_buf);
    if(ret != SASL_OK) goto done;

    to_store = sparams->utils->prop_get(ctx);
    if(!to_store) {
	ret = SASL_BADPARAM;
	goto done;
    }

    /* All iterations return SASL_NOUSER                   ==> ret = SASL_NOUSER
       Some iterations return SASL_OK and some SASL_NOUSER ==> ret = SASL_OK
       At least one iteration returns any other error      ==> ret = the error */
    ret = SASL_NOUSER;
    for(cur = to_store; cur->name; cur++) {
	/* We only support one value at a time right now. */
	tmp_res = _sasldb_putdata(sparams->utils, sparams->utils->conn,
			      userid, realm, cur->name,
			      cur->values && cur->values[0] ?
			      cur->values[0] : NULL,
			      cur->values && cur->values[0] ?
			      strlen(cur->values[0]) : 0);
        /* SASL_NOUSER is returned when _sasldb_putdata fails to delete
           a non-existent entry, which should not be treated as an error */
        if ((tmp_res != SASL_NOUSER) &&
            (ret == SASL_NOUSER || ret == SASL_OK)) {
            ret = tmp_res;
        }

        /* Abort the loop if an error has occurred */
        if (ret != SASL_NOUSER && ret != SASL_OK) {
            break;
        }
    }

 done:
    if (userid) sparams->utils->free(userid);
    if (realm)  sparams->utils->free(realm);
    if (user_buf) sparams->utils->free(user_buf);

    return ret;
}

static sasl_auxprop_plug_t sasldb_auxprop_plugin = {
    0,           		/* Features */
    0,           		/* spare */
    NULL,        		/* glob_context */
    sasldb_auxprop_free,        /* auxprop_free */
    sasldb_auxprop_lookup,	/* auxprop_lookup */
    "sasldb",			/* name */
    sasldb_auxprop_store	/* auxprop_store */
};

int sasldb_auxprop_plug_init(const sasl_utils_t *utils,
                             int max_version,
                             int *out_version,
                             sasl_auxprop_plug_t **plug,
                             const char *plugname __attribute__((unused))) 
{
    if(!out_version || !plug) return SASL_BADPARAM;

    /* Do we have database support? */
    /* Note that we can use a NULL sasl_conn_t because our
     * sasl_utils_t is "blessed" with the global callbacks */
    if(_sasl_check_db(utils, NULL) != SASL_OK)
	return SASL_NOMECH;

    if(max_version < SASL_AUXPROP_PLUG_VERSION) return SASL_BADVERS;
    
    *out_version = SASL_AUXPROP_PLUG_VERSION;

    *plug = &sasldb_auxprop_plugin;

    return SASL_OK;
}
