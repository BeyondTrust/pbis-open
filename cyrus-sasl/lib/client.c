/* SASL client API implementation
 * Rob Siemborski
 * Tim Martin
 * $Id: client.c,v 1.67 2006/04/26 15:33:41 mel Exp $
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
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* SASL Headers */
#include "sasl.h"
#include "saslplug.h"
#include "saslutil.h"
#include "saslint.h"

static cmech_list_t *cmechlist; /* global var which holds the list */
sasl_global_callbacks_t global_callbacks_client; 
static int _sasl_client_active = 0;

static int init_mechlist()
{
  cmechlist->mutex = sasl_MUTEX_ALLOC();
  if(!cmechlist->mutex) return SASL_FAIL;
  
  cmechlist->utils=_sasl_alloc_utils(NULL, &global_callbacks_client);
  if (cmechlist->utils==NULL)
    return SASL_NOMEM;

  cmechlist->mech_list=NULL;
  cmechlist->mech_length=0;

  return SASL_OK;
}

static int client_done(void) {
  cmechanism_t *cm;
  cmechanism_t *cprevm;

  if(!_sasl_client_active)
      return SASL_NOTINIT;
  else
      _sasl_client_active--;
  
  if(_sasl_client_active) {
      /* Don't de-init yet! Our refcount is nonzero. */
      return SASL_CONTINUE;
  }
  
  cm=cmechlist->mech_list; /* m point to begging of the list */
  while (cm!=NULL)
  {
    cprevm=cm;
    cm=cm->next;

    if (cprevm->m.plug->mech_free) {
	cprevm->m.plug->mech_free(cprevm->m.plug->glob_context,
				cmechlist->utils);
    }

    sasl_FREE(cprevm->m.plugname);
    sasl_FREE(cprevm);    
  }
  sasl_MUTEX_FREE(cmechlist->mutex);
  _sasl_free_utils(&cmechlist->utils);
  sasl_FREE(cmechlist);

  cmechlist = NULL;

  return SASL_OK;
}

int sasl_client_add_plugin(const char *plugname,
			   sasl_client_plug_init_t *entry_point)
{
  int plugcount;
  sasl_client_plug_t *pluglist;
  cmechanism_t *mech;
  int result;
  int version;
  int lupe;

  if(!plugname || !entry_point) return SASL_BADPARAM;
  
  result = entry_point(cmechlist->utils, SASL_CLIENT_PLUG_VERSION, &version,
		       &pluglist, &plugcount);

  if (result != SASL_OK)
  {
    _sasl_log(NULL, SASL_LOG_WARN,
	      "entry_point failed in sasl_client_add_plugin for %s",
	      plugname);
    return result;
  }

  if (version != SASL_CLIENT_PLUG_VERSION)
  {
    _sasl_log(NULL, SASL_LOG_WARN,
	      "version conflict in sasl_client_add_plugin for %s", plugname);
    return SASL_BADVERS;
  }

  for (lupe=0;lupe< plugcount ;lupe++)
    {
      mech = sasl_ALLOC(sizeof(cmechanism_t));
      if (! mech) return SASL_NOMEM;

      mech->m.plug=pluglist++;
      if(_sasl_strdup(plugname, &mech->m.plugname, NULL) != SASL_OK) {
	sasl_FREE(mech);
	return SASL_NOMEM;
      }
      mech->m.version = version;
      mech->next = cmechlist->mech_list;
      cmechlist->mech_list = mech;
      cmechlist->mech_length++;
    }

  return SASL_OK;
}

static int
client_idle(sasl_conn_t *conn)
{
  cmechanism_t *m;
  if (! cmechlist)
    return 0;

  for (m = cmechlist->mech_list;
       m;
       m = m->next)
    if (m->m.plug->idle
	&&  m->m.plug->idle(m->m.plug->glob_context,
			  conn,
			  conn ? ((sasl_client_conn_t *)conn)->cparams : NULL))
      return 1;
  return 0;
}

/* initialize the SASL client drivers
 *  callbacks      -- base callbacks for all client connections
 * returns:
 *  SASL_OK        -- Success
 *  SASL_NOMEM     -- Not enough memory
 *  SASL_BADVERS   -- Mechanism version mismatch
 *  SASL_BADPARAM  -- error in config file
 *  SASL_NOMECH    -- No mechanisms available
 *  ...
 */

int sasl_client_init(const sasl_callback_t *callbacks)
{
  int ret;
  const add_plugin_list_t ep_list[] = {
      { "sasl_client_plug_init", (add_plugin_t *)sasl_client_add_plugin },
      { "sasl_canonuser_init", (add_plugin_t *)sasl_canonuser_add_plugin },
      { NULL, NULL }
  };

  if(_sasl_client_active) {
      /* We're already active, just increase our refcount */
      /* xxx do something with the callback structure? */
      _sasl_client_active++;
      return SASL_OK;
  }

  global_callbacks_client.callbacks = callbacks;
  global_callbacks_client.appname = NULL;

  cmechlist=sasl_ALLOC(sizeof(cmech_list_t));
  if (cmechlist==NULL) return SASL_NOMEM;

  /* We need to call client_done if we fail now */
  _sasl_client_active = 1;

  /* load plugins */
  ret=init_mechlist();  
  if (ret!=SASL_OK) {
      client_done();
      return ret;
  }

  sasl_client_add_plugin("EXTERNAL", &external_client_plug_init);

  ret = _sasl_common_init(&global_callbacks_client);

  if (ret == SASL_OK)
      ret = _sasl_load_plugins(ep_list,
			       _sasl_find_getpath_callback(callbacks),
			       _sasl_find_verifyfile_callback(callbacks));
  
  if (ret == SASL_OK) {
      _sasl_client_cleanup_hook = &client_done;
      _sasl_client_idle_hook = &client_idle;

      ret = _sasl_build_mechlist();
  } else {
      client_done();
  }
      
  return ret;
}

static void client_dispose(sasl_conn_t *pconn)
{
  sasl_client_conn_t *c_conn=(sasl_client_conn_t *) pconn;

  if (c_conn->mech && c_conn->mech->m.plug->mech_dispose) {
    c_conn->mech->m.plug->mech_dispose(pconn->context,
				     c_conn->cparams->utils);
  }

  pconn->context = NULL;

  if (c_conn->clientFQDN)
      sasl_FREE(c_conn->clientFQDN);

  if (c_conn->cparams) {
      _sasl_free_utils(&(c_conn->cparams->utils));
      sasl_FREE(c_conn->cparams);
  }

  _sasl_conn_dispose(pconn);
}

/* initialize a client exchange based on the specified mechanism
 *  service       -- registered name of the service using SASL (e.g. "imap")
 *  serverFQDN    -- the fully qualified domain name of the server
 *  iplocalport   -- client IPv4/IPv6 domain literal string with port
 *                    (if NULL, then mechanisms requiring IPaddr are disabled)
 *  ipremoteport  -- server IPv4/IPv6 domain literal string with port
 *                    (if NULL, then mechanisms requiring IPaddr are disabled)
 *  prompt_supp   -- list of client interactions supported
 *                   may also include sasl_getopt_t context & call
 *                   NULL prompt_supp = user/pass via SASL_INTERACT only
 *                   NULL proc = interaction supported via SASL_INTERACT
 *  secflags      -- security flags (see above)
 * in/out:
 *  pconn         -- connection negotiation structure
 *                   pointer to NULL => allocate new
 *                   non-NULL => recycle storage and go for next available mech
 *
 * Returns:
 *  SASL_OK       -- success
 *  SASL_NOMECH   -- no mechanism meets requested properties
 *  SASL_NOMEM    -- not enough memory
 */
int sasl_client_new(const char *service,
		    const char *serverFQDN,
		    const char *iplocalport,
		    const char *ipremoteport,
		    const sasl_callback_t *prompt_supp,
		    unsigned flags,
		    sasl_conn_t **pconn)
{
  int result;
  char name[MAXHOSTNAMELEN];
  sasl_client_conn_t *conn;
  sasl_utils_t *utils;

  if(_sasl_client_active==0) return SASL_NOTINIT;
  
  /* Remember, serverFQDN, iplocalport and ipremoteport can be NULL and be valid! */
  if (!pconn || !service)
    return SASL_BADPARAM;

  *pconn=sasl_ALLOC(sizeof(sasl_client_conn_t));
  if (*pconn==NULL) {
      _sasl_log(NULL, SASL_LOG_ERR,
		"Out of memory allocating connection context");
      return SASL_NOMEM;
  }
  memset(*pconn, 0, sizeof(sasl_client_conn_t));

  (*pconn)->destroy_conn = &client_dispose;

  conn = (sasl_client_conn_t *)*pconn;
  
  conn->mech = NULL;

  conn->cparams=sasl_ALLOC(sizeof(sasl_client_params_t));
  if (conn->cparams==NULL) 
      MEMERROR(*pconn);
  memset(conn->cparams,0,sizeof(sasl_client_params_t));

  result = _sasl_conn_init(*pconn, service, flags, SASL_CONN_CLIENT,
			   &client_idle, serverFQDN,
			   iplocalport, ipremoteport,
			   prompt_supp, &global_callbacks_client);
  if (result != SASL_OK) RETURN(*pconn, result);
  
  utils=_sasl_alloc_utils(*pconn, &global_callbacks_client);
  if (utils==NULL)
      MEMERROR(*pconn);
  
  utils->conn= *pconn;

  /* Setup the non-lazy parts of cparams, the rest is done in
   * sasl_client_start */
  conn->cparams->utils = utils;
  conn->cparams->canon_user = &_sasl_canon_user;
  conn->cparams->flags = flags;
  conn->cparams->prompt_supp = (*pconn)->callbacks;
  
  /* get the clientFQDN (serverFQDN was set in _sasl_conn_init) */
  memset(name, 0, sizeof(name));
  gethostname(name, MAXHOSTNAMELEN);

  result = _sasl_strdup(name, &conn->clientFQDN, NULL);

  if(result == SASL_OK) return SASL_OK;

  /* result isn't SASL_OK */
  _sasl_conn_dispose(*pconn);
  sasl_FREE(*pconn);
  *pconn = NULL;
  _sasl_log(NULL, SASL_LOG_ERR, "Out of memory in sasl_client_new");
  return result;
}

static int have_prompts(sasl_conn_t *conn,
			const sasl_client_plug_t *mech)
{
  static const unsigned long default_prompts[] = {
    SASL_CB_AUTHNAME,
    SASL_CB_PASS,
    SASL_CB_LIST_END
  };

  const unsigned long *prompt;
  int (*pproc)();
  void *pcontext;
  int result;

  for (prompt = (mech->required_prompts
		 ? mech->required_prompts :
		 default_prompts);
       *prompt != SASL_CB_LIST_END;
       prompt++) {
    result = _sasl_getcallback(conn, *prompt, &pproc, &pcontext);
    if (result != SASL_OK && result != SASL_INTERACT)
      return 0;			/* we don't have this required prompt */
  }

  return 1; /* we have all the prompts */
}

/* select a mechanism for a connection
 *  mechlist      -- mechanisms server has available (punctuation ignored)
 *  secret        -- optional secret from previous session
 * output:
 *  prompt_need   -- on SASL_INTERACT, list of prompts needed to continue
 *  clientout     -- the initial client response to send to the server
 *  mech          -- set to mechanism name
 *
 * Returns:
 *  SASL_OK       -- success
 *  SASL_NOMEM    -- not enough memory
 *  SASL_NOMECH   -- no mechanism meets requested properties
 *  SASL_INTERACT -- user interaction needed to fill in prompt_need list
 */

/* xxx confirm this with rfc 2222
 * SASL mechanism allowable characters are "AZaz-_"
 * seperators can be any other characters and of any length
 * even variable lengths between
 *
 * Apps should be encouraged to simply use space or comma space
 * though
 */
int sasl_client_start(sasl_conn_t *conn,
		      const char *mechlist,
		      sasl_interact_t **prompt_need,
		      const char **clientout,
		      unsigned *clientoutlen,
		      const char **mech)
{
    sasl_client_conn_t *c_conn= (sasl_client_conn_t *) conn;
    char name[SASL_MECHNAMEMAX + 1];
    cmechanism_t *m=NULL,*bestm=NULL;
    size_t pos=0,place;
    size_t list_len;
    sasl_ssf_t bestssf = 0, minssf = 0;
    int result;

    if(_sasl_client_active==0) return SASL_NOTINIT;

    if (!conn) return SASL_BADPARAM;

    /* verify parameters */
    if (mechlist == NULL)
	PARAMERROR(conn);

    /* if prompt_need != NULL we've already been here
       and just need to do the continue step again */

    /* do a step */
    /* FIXME: Hopefully they only give us our own prompt_need back */
    if (prompt_need && *prompt_need != NULL) {
	goto dostep;
    }

    if(conn->props.min_ssf < conn->external.ssf) {
	minssf = 0;
    } else {
	minssf = conn->props.min_ssf - conn->external.ssf;
    }

    /* parse mechlist */
    list_len = strlen(mechlist);

    while (pos<list_len)
    {
	place=0;
	while ((pos<list_len) && (isalnum((unsigned char)mechlist[pos])
				  || mechlist[pos] == '_'
				  || mechlist[pos] == '-')) {
	    name[place]=mechlist[pos];
	    pos++;
	    place++;
	    if (SASL_MECHNAMEMAX < place) {
		place--;
		while(pos<list_len && (isalnum((unsigned char)mechlist[pos])
				       || mechlist[pos] == '_'
				       || mechlist[pos] == '-'))
		    pos++;
	    }
	}
	pos++;
	name[place]=0;

	if (! place) continue;

	/* foreach in client list */
	for (m = cmechlist->mech_list; m != NULL; m = m->next) {
	    int myflags;
	    
	    /* Is this the mechanism the server is suggesting? */
	    if (strcasecmp(m->m.plug->mech_name, name))
		continue; /* no */

	    /* Do we have the prompts for it? */
	    if (!have_prompts(conn, m->m.plug))
		break;

	    /* Is it strong enough? */
	    if (minssf > m->m.plug->max_ssf)
		break;

	    /* Does it meet our security properties? */
	    myflags = conn->props.security_flags;
	    
	    /* if there's an external layer this is no longer plaintext */
	    if ((conn->props.min_ssf <= conn->external.ssf) && 
		(conn->external.ssf > 1)) {
		myflags &= ~SASL_SEC_NOPLAINTEXT;
	    }

	    if (((myflags ^ m->m.plug->security_flags) & myflags) != 0) {
		break;
	    }

	    /* Can we meet it's features? */
	    if ((m->m.plug->features & SASL_FEAT_NEEDSERVERFQDN)
		&& !conn->serverFQDN) {
		break;
	    }

	    /* Can it meet our features? */
	    if ((conn->flags & SASL_NEED_PROXY) &&
		!(m->m.plug->features & SASL_FEAT_ALLOWS_PROXY)) {
		break;
	    }
	    
#ifdef PREFER_MECH
	    if (strcasecmp(m->m.plug->mech_name, PREFER_MECH) &&
		bestm && m->m.plug->max_ssf <= bestssf) {
		/* this mechanism isn't our favorite, and it's no better
		   than what we already have! */
		break;
	    }
#else
	    if (bestm && m->m.plug->max_ssf <= bestssf) {
		/* this mechanism is no better than what we already have! */
		break;
	    }
#endif

	    /* compare security flags, only take new mechanism if it has
	     * all the security flags of the previous one.
	     *
	     * From the mechanisms we ship with, this yields the order:
	     *
	     * SRP
	     * GSSAPI + KERBEROS_V4
	     * DIGEST + OTP
	     * CRAM + EXTERNAL
	     * PLAIN + LOGIN + ANONYMOUS
	     *
	     * This might be improved on by comparing the numeric value of
	     * the bitwise-or'd security flags, which splits DIGEST/OTP,
	     * CRAM/EXTERNAL, and PLAIN/LOGIN from ANONYMOUS, but then we
	     * are depending on the numeric values of the flags (which may
	     * change, and their ordering could be considered dumb luck.
	     */

	    if (bestm &&
		((m->m.plug->security_flags ^ bestm->m.plug->security_flags) &
		 bestm->m.plug->security_flags)) {
		break;
	    }

	    if (mech) {
		*mech = m->m.plug->mech_name;
	    }
	    bestssf = m->m.plug->max_ssf;
	    bestm = m;
	    break;
	}
    }

    if (bestm == NULL) {
	sasl_seterror(conn, 0, "No worthy mechs found");
	result = SASL_NOMECH;
	goto done;
    }

    /* make (the rest of) cparams */
    c_conn->cparams->service = conn->service;
    c_conn->cparams->servicelen = (unsigned) strlen(conn->service);
    
    if (conn->serverFQDN) {
	c_conn->cparams->serverFQDN = conn->serverFQDN; 
	c_conn->cparams->slen = (unsigned) strlen(conn->serverFQDN);
    }

    c_conn->cparams->clientFQDN = c_conn->clientFQDN; 
    c_conn->cparams->clen = (unsigned) strlen(c_conn->clientFQDN);

    c_conn->cparams->external_ssf = conn->external.ssf;
    c_conn->cparams->props = conn->props;
    c_conn->mech = bestm;

    /* init that plugin */
    result = c_conn->mech->m.plug->mech_new(c_conn->mech->m.plug->glob_context,
					  c_conn->cparams,
					  &(conn->context));
    if(result != SASL_OK) goto done;

    /* do a step -- but only if we can do a client-send-first */
 dostep:
    if(clientout) {
        if(c_conn->mech->m.plug->features & SASL_FEAT_SERVER_FIRST) {
            *clientout = NULL;
            *clientoutlen = 0;
            result = SASL_CONTINUE;
        } else {
            result = sasl_client_step(conn, NULL, 0, prompt_need,
                                      clientout, clientoutlen);
        }
    }
    else
	result = SASL_CONTINUE;

 done:
    RETURN(conn, result);
}

/* do a single authentication step.
 *  serverin    -- the server message received by the client, MUST have a NUL
 *                 sentinel, not counted by serverinlen
 * output:
 *  prompt_need -- on SASL_INTERACT, list of prompts needed to continue
 *  clientout   -- the client response to send to the server
 *
 * returns:
 *  SASL_OK        -- success
 *  SASL_INTERACT  -- user interaction needed to fill in prompt_need list
 *  SASL_BADPROT   -- server protocol incorrect/cancelled
 *  SASL_BADSERV   -- server failed mutual auth
 */

int sasl_client_step(sasl_conn_t *conn,
		     const char *serverin,
		     unsigned serverinlen,
		     sasl_interact_t **prompt_need,
		     const char **clientout,
		     unsigned *clientoutlen)
{
  sasl_client_conn_t *c_conn= (sasl_client_conn_t *) conn;
  int result;

  if(_sasl_client_active==0) return SASL_NOTINIT;
  if(!conn) return SASL_BADPARAM;

  /* check parameters */
  if ((serverin==NULL) && (serverinlen>0))
      PARAMERROR(conn);

  /* Don't do another step if the plugin told us that we're done */
  if (conn->oparams.doneflag) {
      _sasl_log(conn, SASL_LOG_ERR, "attempting client step after doneflag");
      return SASL_FAIL;
  }

  if(clientout) *clientout = NULL;
  if(clientoutlen) *clientoutlen = 0;

  /* do a step */
  result = c_conn->mech->m.plug->mech_step(conn->context,
					 c_conn->cparams,
					 serverin,
					 serverinlen,
					 prompt_need,
					 clientout, clientoutlen,
					 &conn->oparams);

  if (result == SASL_OK) {
      /* So we're done on this end, but if both
       * 1. the mech does server-send-last
       * 2. the protocol does not
       * we need to return no data */
      if(!*clientout && !(conn->flags & SASL_SUCCESS_DATA)) {
	  *clientout = "";
	  *clientoutlen = 0;
      }
      
      if(!conn->oparams.maxoutbuf) {
	  conn->oparams.maxoutbuf = conn->props.maxbufsize;
      }

      if(conn->oparams.user == NULL || conn->oparams.authid == NULL) {
	  sasl_seterror(conn, 0,
			"mech did not call canon_user for both authzid and authid");
	  result = SASL_BADPROT;
      }
  }  

  RETURN(conn,result);
}

/* returns the length of all the mechanisms
 * added up 
 */

static unsigned mech_names_len()
{
  cmechanism_t *listptr;
  unsigned result = 0;

  for (listptr = cmechlist->mech_list;
       listptr;
       listptr = listptr->next)
    result += (unsigned) strlen(listptr->m.plug->mech_name);

  return result;
}


int _sasl_client_listmech(sasl_conn_t *conn,
			  const char *prefix,
			  const char *sep,
			  const char *suffix,
			  const char **result,
			  unsigned *plen,
			  int *pcount)
{
    cmechanism_t *m=NULL;
    sasl_ssf_t minssf = 0;
    int ret;
    size_t resultlen;
    int flag;
    const char *mysep;

    if(_sasl_client_active == 0) return SASL_NOTINIT;
    if (!conn) return SASL_BADPARAM;
    if(conn->type != SASL_CONN_CLIENT) PARAMERROR(conn);
    
    if (! result)
	PARAMERROR(conn);
    
    if (plen != NULL)
	*plen = 0;
    if (pcount != NULL)
	*pcount = 0;

    if (sep) {
	mysep = sep;
    } else {
	mysep = " ";
    }

    if(conn->props.min_ssf < conn->external.ssf) {
	minssf = 0;
    } else {
	minssf = conn->props.min_ssf - conn->external.ssf;
    }

    if (! cmechlist || cmechlist->mech_length <= 0)
	INTERROR(conn, SASL_NOMECH);

    resultlen = (prefix ? strlen(prefix) : 0)
	+ (strlen(mysep) * (cmechlist->mech_length - 1))
	+ mech_names_len()
	+ (suffix ? strlen(suffix) : 0)
	+ 1;
    ret = _buf_alloc(&conn->mechlist_buf,
		     &conn->mechlist_buf_len, resultlen);
    if(ret != SASL_OK) MEMERROR(conn);

    if (prefix)
	strcpy (conn->mechlist_buf,prefix);
    else
	*(conn->mechlist_buf) = '\0';

    flag = 0;
    for (m = cmechlist->mech_list; m != NULL; m = m->next) {
	    /* do we have the prompts for it? */
	    if (!have_prompts(conn, m->m.plug))
		continue;

	    /* is it strong enough? */
	    if (minssf > m->m.plug->max_ssf)
		continue;

	    /* does it meet our security properties? */
	    if (((conn->props.security_flags ^ m->m.plug->security_flags)
		 & conn->props.security_flags) != 0) {
		continue;
	    }

	    /* Can we meet it's features? */
	    if ((m->m.plug->features & SASL_FEAT_NEEDSERVERFQDN)
		&& !conn->serverFQDN) {
		continue;
	    }

	    /* Can it meet our features? */
	    if ((conn->flags & SASL_NEED_PROXY) &&
		!(m->m.plug->features & SASL_FEAT_ALLOWS_PROXY)) {
		continue;
	    }

	    /* Okay, we like it, add it to the list! */

	    if (pcount != NULL)
		(*pcount)++;

	    /* print seperator */
	    if (flag) {
		strcat(conn->mechlist_buf, mysep);
	    } else {
		flag = 1;
	    }
	    
	    /* now print the mechanism name */
	    strcat(conn->mechlist_buf, m->m.plug->mech_name);
    }
    
  if (suffix)
      strcat(conn->mechlist_buf,suffix);

  if (plen!=NULL)
      *plen = (unsigned) strlen(conn->mechlist_buf);

  *result = conn->mechlist_buf;

  return SASL_OK;
}

sasl_string_list_t *_sasl_client_mechs(void) 
{
  cmechanism_t *listptr;
  sasl_string_list_t *retval = NULL, *next=NULL;

  if(!_sasl_client_active) return NULL;

  /* make list */
  for (listptr = cmechlist->mech_list; listptr; listptr = listptr->next) {
      next = sasl_ALLOC(sizeof(sasl_string_list_t));

      if(!next && !retval) return NULL;
      else if(!next) {
	  next = retval->next;
	  do {
	      sasl_FREE(retval);
	      retval = next;
	      next = retval->next;
	  } while(next);
	  return NULL;
      }
      
      next->d = listptr->m.plug->mech_name;

      if(!retval) {
	  next->next = NULL;
	  retval = next;
      } else {
	  next->next = retval;
	  retval = next;
      }
  }

  return retval;
}




/* It would be nice if we can show other information like Author, Company, Year, plugin version */
static void
_sasl_print_mechanism (
  client_sasl_mechanism_t *m,
  sasl_info_callback_stage_t stage,
  void *rock
)
{
    char delimiter;

    if (stage == SASL_INFO_LIST_START) {
	printf ("List of client plugins follows\n");
	return;
    } else if (stage == SASL_INFO_LIST_END) {
	return;
    }

    /* Process the mechanism */
    printf ("Plugin \"%s\" ", m->plugname);

    /* There is no delay loading for client side plugins */
    printf ("[loaded]");

    printf (", \tAPI version: %d\n", m->version);

    if (m->plug != NULL) {
	printf ("\tSASL mechanism: %s, best SSF: %d\n",
		m->plug->mech_name,
		m->plug->max_ssf);

	printf ("\tsecurity flags:");
	
	delimiter = ' ';
	if (m->plug->security_flags & SASL_SEC_NOANONYMOUS) {
	    printf ("%cNO_ANONYMOUS", delimiter);
	    delimiter = '|';
	}

	if (m->plug->security_flags & SASL_SEC_NOPLAINTEXT) {
	    printf ("%cNO_PLAINTEXT", delimiter);
	    delimiter = '|';
	}
	
	if (m->plug->security_flags & SASL_SEC_NOACTIVE) {
	    printf ("%cNO_ACTIVE", delimiter);
	    delimiter = '|';
	}

	if (m->plug->security_flags & SASL_SEC_NODICTIONARY) {
	    printf ("%cNO_DICTIONARY", delimiter);
	    delimiter = '|';
	}

	if (m->plug->security_flags & SASL_SEC_FORWARD_SECRECY) {
	    printf ("%cFORWARD_SECRECY", delimiter);
	    delimiter = '|';
	}

	if (m->plug->security_flags & SASL_SEC_PASS_CREDENTIALS) {
	    printf ("%cPASS_CREDENTIALS", delimiter);
	    delimiter = '|';
	}

	if (m->plug->security_flags & SASL_SEC_MUTUAL_AUTH) {
	    printf ("%cMUTUAL_AUTH", delimiter);
	    delimiter = '|';
	}



	printf ("\n\tfeatures:");
	
	delimiter = ' ';
	if (m->plug->features & SASL_FEAT_WANT_CLIENT_FIRST) {
	    printf ("%cWANT_CLIENT_FIRST", delimiter);
	    delimiter = '|';
	}

	if (m->plug->features & SASL_FEAT_SERVER_FIRST) {
	    printf ("%cSERVER_FIRST", delimiter);
	    delimiter = '|';
	}

	if (m->plug->features & SASL_FEAT_ALLOWS_PROXY) {
	    printf ("%cPROXY_AUTHENTICATION", delimiter);
	    delimiter = '|';
	}

	if (m->plug->features & SASL_FEAT_NEEDSERVERFQDN) {
	    printf ("%cNEED_SERVER_FQDN", delimiter);
	    delimiter = '|';
	}
    }

/* Delay loading is not supported for the client side plugins:
    if (m->f) {
	printf ("\n\twill be loaded from \"%s\"", m->f);
    }
 */

    printf ("\n");
}


/* Dump information about available client plugins */
int sasl_client_plugin_info (
  const char *c_mech_list,		/* space separated mechanism list or NULL for ALL */
  sasl_client_info_callback_t *info_cb,
  void *info_cb_rock
)
{
    cmechanism_t *m;
    client_sasl_mechanism_t plug_data;
    char * cur_mech;
    char * mech_list = NULL;
    char * p;

    if (info_cb == NULL) {
	info_cb = _sasl_print_mechanism;
    }

    if (cmechlist != NULL) {
	info_cb (NULL, SASL_INFO_LIST_START, info_cb_rock);

	if (c_mech_list == NULL) {
	    m = cmechlist->mech_list; /* m point to beginning of the list */

	    while (m != NULL) {
		memcpy (&plug_data, &m->m, sizeof(plug_data));

		info_cb (&plug_data, SASL_INFO_LIST_MECH, info_cb_rock);
	    
		m = m->next;
	    }
	} else {
            mech_list = strdup (c_mech_list);

	    cur_mech = mech_list;

	    while (cur_mech != NULL) {
		p = strchr (cur_mech, ' ');
		if (p != NULL) {
		    *p = '\0';
		    p++;
		}

		m = cmechlist->mech_list; /* m point to beginning of the list */

		while (m != NULL) {
		    if (strcasecmp (cur_mech, m->m.plug->mech_name) == 0) {
			memcpy (&plug_data, &m->m, sizeof(plug_data));

			info_cb (&plug_data, SASL_INFO_LIST_MECH, info_cb_rock);
		    }
	    
		    m = m->next;
		}

		cur_mech = p;
	    }

            free (mech_list);
	}

	info_cb (NULL, SASL_INFO_LIST_END, info_cb_rock);

	return (SASL_OK);
    }

    return (SASL_NOTINIT);
}
