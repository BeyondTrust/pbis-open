/* SASL Config file API
 * Rob Siemborski
 * Tim Martin (originally in Cyrus distribution)
 * $Id: config.c,v 1.15 2006/04/10 13:28:06 mel Exp $
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

/*
 * Current Valid keys:
 *
 * canon_user_plugin: <string>
 * pwcheck_method: <string>
 * auto_transition: <boolean>
 * plugin_list: <string>
 *
 * srvtab: <string>
 */


#include "sasl.h"
#include "saslint.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

struct configlist {
    char *key;
    char *value;
};

static struct configlist *configlist;
static int nconfiglist;

#define CONFIGLISTGROWSIZE 100

int sasl_config_init(const char *filename)
{
    FILE *infile;
    int lineno = 0;
    int alloced = 0;
    char buf[4096];
    char *p, *key;
    int result;

    nconfiglist=0;

    infile = fopen(filename, "r");
    if (!infile) {
        return SASL_CONTINUE;
    }
    
    while (fgets(buf, sizeof(buf), infile)) {
	lineno++;

	if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
	for (p = buf; *p && isspace((int) *p); p++);
	if (!*p || *p == '#') continue;

	key = p;
	while (*p && (isalnum((int) *p) || *p == '-' || *p == '_')) {
	    if (isupper((int) *p)) *p = tolower(*p);
	    p++;
	}
	if (*p != ':') {
	    return SASL_FAIL;
	}
	*p++ = '\0';

	while (*p && isspace((int) *p)) p++;
	
	if (!*p) {
	    return SASL_FAIL;
	}

	if (nconfiglist == alloced) {
	    alloced += CONFIGLISTGROWSIZE;
	    configlist=sasl_REALLOC((char *)configlist, 
				    alloced * sizeof(struct configlist));
	    if (configlist==NULL) return SASL_NOMEM;
	}



	result = _sasl_strdup(key,
			      &(configlist[nconfiglist].key),
			      NULL);
	if (result!=SASL_OK) return result;
	result = _sasl_strdup(p,
			      &(configlist[nconfiglist].value),
			      NULL);
	if (result!=SASL_OK) return result;

	nconfiglist++;
    }
    fclose(infile);

    return SASL_OK;
}

const char *sasl_config_getstring(const char *key,const char *def)
{
    int opt;

    for (opt = 0; opt < nconfiglist; opt++) {
	if (*key == configlist[opt].key[0] &&
	    !strcmp(key, configlist[opt].key))
	  return configlist[opt].value;
    }
    return def;
}
