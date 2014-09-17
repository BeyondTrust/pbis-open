/* MODULE: auth_rimap */

/* COPYRIGHT
 * Copyright (c) 1998 Messaging Direct Ltd.
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
 * 
 * Copyright 1998, 1999 Carnegie Mellon University
 * 
 *                       All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Carnegie Mellon
 * University not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 * 
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * END COPYRIGHT */

/* SYNOPSIS
 * Proxy authentication to a remote IMAP (or IMSP) server.
 * END SYNOPSIS */

#ifdef __GNUC__
#ident "$Id: auth_rimap.c,v 1.12 2006/04/06 20:19:54 jeaton Exp $"
#endif

/* PUBLIC DEPENDENCIES */
#include "mechanisms.h"

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#ifdef _AIX
# include <strings.h>
#endif /* _AIX */
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

#include "auth_rimap.h"
#include "utils.h"
#include "globals.h"
/* END PUBLIC DEPENDENCIES */

/* PRIVATE DEPENDENCIES */
static const char *r_host = NULL;       /* remote hostname (mech_option) */
static struct addrinfo *ai = NULL;	/* remote authentication host    */
/* END PRIVATE DEPENDENCIES */

#define DEFAULT_REMOTE_SERVICE "imap"	/* getservbyname() name for remote
					   service we connect to.	 */
#define TAG "saslauthd"			/* IMAP command tag */
#define LOGIN_CMD (TAG " LOGIN ")	/* IMAP login command (with tag) */
#define NETWORK_IO_TIMEOUT 30		/* network I/O timeout (seconds) */
#define RESP_LEN 1000			/* size of read response buffer  */

/* Common failure response strings for auth_rimap() */

#define RESP_IERROR	"NO [ALERT] saslauthd internal error"
#define RESP_UNAVAILABLE "NO [ALERT] The remote authentication server is currently unavailable"
#define RESP_UNEXPECTED	"NO [ALERT] Unexpected response from remote authentication server"

/* FUNCTION: sig_null */

/* SYNOPSIS
 * Catch and ignore a signal.
 * END SYNOPSIS */

static RETSIGTYPE				/* R: OS dependent */
sig_null (
  /* PARAMETERS */
  int sig					/* I: signal being caught */
  /* END PARAMETERS */
  )
{

    switch (sig) {
	
      case SIGALRM:
	signal(SIGALRM, sig_null);
	break;

      case SIGPIPE:
	signal(SIGPIPE, sig_null);
	break;

      default:
	syslog(LOG_WARNING, "auth_rimap: unexpected signal %d", sig);
	break;
    }
#ifdef __APPLE__
    return;
#else /* __APPLE__ */
# if RETSIGTYPE == void
    return;
# else /* RETSIGTYPE */
    return 0;
# endif /* RETSIGTYPE */
#endif /* __APPLE__ */
}

/* END FUNCTION: sig_null */

/* FUNCTION: qstring */

/* SYNOPSIS
 * Quote a string for transmission over the IMAP protocol.
 * END SYNOPSIS */

static char *				/* R: the quoted string		*/
qstring (
  /* PARAMETERS */
  const char *s				/* I: string to quote		*/
  /* END PARAMETERS */
  )
{
    char *c;				/* pointer to returned string   */
    register const char *p1;		/* scratch pointers		*/
    register char *p2;			/* scratch pointers             */
    int len;				/* length of array to malloc    */
    int num_quotes;			/* number of '"' chars in string*/

    /* see of we have to deal with any '"' characters */
    num_quotes = 0;
    p1 = s;
    while ((p1 = strchr(p1, '"')) != NULL) {
	num_quotes++;
    }
    
    if (!num_quotes) {
	/*
	 * no double-quotes to escape, so just wrap the input string
	 * in double-quotes and return it.
	 */
	len = strlen(s) + 2 + 1;
	c = malloc(len);
	if (c == NULL) {
	    return NULL;
	}
	*c = '"';
        *(c+1) = '\0';
	strcat(c, s);
	strcat(c, "\"");
	return c;
    }
    /*
     * Ugh, we have to escape double quotes ...
     */
    len = strlen(s) + 2 + (2*num_quotes) + 1;
    c = malloc(len);
    if (c == NULL) {
	return NULL;
    }
    p1 = s;
    p2 = c;
    *p2++ = '"';
    while (*p1) {
	if (*p1 == '"') {
	    *p2++ = '\\';		/* escape the '"' */
	}
	*p2++ = *p1++;
    }
    strcat(p2, "\"");
    return c;
}

/* END FUNCTION: qstring */

/* FUNCTION: auth_rimap_init */

/* SYNOPSIS
 * Validate the host and service names for the remote server.
 * END SYNOPSIS */

int
auth_rimap_init (
  /* PARAMETERS */
  void					/* no parameters */
  /* END PARAMETERS */
  )
{

    /* VARIABLES */
    struct addrinfo hints;
    int err;
    char *c;				/* scratch pointer               */
    /* END VARIABLES */

    if (mech_option == NULL) {
	syslog(LOG_ERR, "rimap_init: no hostname specified");
	return -1;
    } else {
	r_host = mech_option;
    }

    /* Determine the port number to connect to.
     *
     * r_host has already been initialized to the hostname and optional port
     * port name to connect to. The format of the string is:
     *
     *		hostname
     * or
     *		hostname/port
     */

    c = strchr(r_host, '/');		/* look for optional service  */
    
    if (c != NULL) {
	*c++ = '\0';			/* tie off hostname and point */
					/* to service string          */
    } else {
	c = DEFAULT_REMOTE_SERVICE;
    }

    if (ai)
	freeaddrinfo(ai);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if ((err = getaddrinfo(r_host, c, &hints, &ai)) != 0) {
	syslog(LOG_ERR, "auth_rimap_init: getaddrinfo %s/%s: %s",
	       r_host, c, gai_strerror(err));
	return -1;
     }
    /* Make sure we have AF_INET or AF_INET6 addresses. */
    if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6) {
	syslog(LOG_ERR, "auth_rimap_init: no IP address info for %s",
	       ai->ai_canonname ? ai->ai_canonname : r_host);
	freeaddrinfo(ai);
	ai = NULL;
	return -1;
    }

    return 0;
}

/* END FUNCTION: auth_rimap_init */

/* FUNCTION: auth_rimap */

/* SYNOPSIS
 * Proxy authenticate to a remote IMAP server.
 *
 * This mechanism takes the plaintext authenticator and password, forms
 * them into an IMAP LOGIN command, then attempts to authenticate to
 * a remote IMAP server using those values. If the remote authentication
 * succeeds the credentials are considered valid.
 *
 * NOTE: since IMSP uses the same form of LOGIN command as IMAP does,
 * this driver will also work with IMSP servers.
 */

/* XXX This should be extended to support SASL PLAIN authentication */

char *					/* R: Allocated response string */
auth_rimap (
  /* PARAMETERS */
  const char *login,			/* I: plaintext authenticator */
  const char *password,			/* I: plaintext password */
  const char *service __attribute__((unused)),
  const char *realm __attribute__((unused))
  /* END PARAMETERS */
  )
{
    /* VARIABLES */
    int	s=-1;				/* socket to remote auth host   */
    struct addrinfo *r;			/* remote socket address info   */
    struct iovec iov[5];		/* for sending LOGIN command    */
    char *qlogin;			/* pointer to "quoted" login    */
    char *qpass;			/* pointer to "quoted" password */
    char *c;				/* scratch pointer              */
    int rc;				/* return code scratch area     */
    char rbuf[RESP_LEN];		/* response read buffer         */
    char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
    int saved_errno;
    int niflags;
    /* END VARIABLES */

    /* sanity checks */
    assert(login != NULL);
    assert(password != NULL);

    /*establish connection to remote */
    for (r = ai; r; r = r->ai_next) {
	s = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
	if (s < 0)
	    continue;
	if (connect(s, r->ai_addr, r->ai_addrlen) >= 0)
	    break;
	close(s);
	s = -1;
	saved_errno = errno;
	niflags = (NI_NUMERICHOST | NI_NUMERICSERV);
#ifdef NI_WITHSCOPEID
	if (r->ai_family == AF_INET6)
	    niflags |= NI_WITHSCOPEID;
#endif
	if (getnameinfo(r->ai_addr, r->ai_addrlen, hbuf, sizeof(hbuf),
			pbuf, sizeof(pbuf), niflags) != 0) {
	    strlcpy(hbuf, "unknown", sizeof(hbuf));
	    strlcpy(pbuf, "unknown", sizeof(pbuf));
	}
	errno = saved_errno;
	syslog(LOG_WARNING, "auth_rimap: connect %s[%s]/%s: %m",
	       ai->ai_canonname ? ai->ai_canonname : r_host, hbuf, pbuf);
    }
    if (s < 0) {
	if (getnameinfo(ai->ai_addr, ai->ai_addrlen, NULL, 0,
			pbuf, sizeof(pbuf), NI_NUMERICSERV) != 0)
	    strlcpy(pbuf, "unknown", sizeof(pbuf));
	syslog(LOG_WARNING, "auth_rimap: couldn't connect to %s/%s",
	       ai->ai_canonname ? ai->ai_canonname : r_host, pbuf);
	return strdup("NO [ALERT] Couldn't contact remote authentication server");
    }

    /* CLAIM: we now have a TCP connection to the remote IMAP server */

    /*
     * Install noop signal handlers. These just reinstall the handler
     * and return so that we take an EINTR during network I/O.
     */
    (void) signal(SIGALRM, sig_null);
    (void) signal(SIGPIPE, sig_null);
    
    /* read and parse the IMAP banner */

    alarm(NETWORK_IO_TIMEOUT);
    rc = read(s, rbuf, sizeof(rbuf));
    alarm(0);
    if (rc == -1) {
	syslog(LOG_WARNING, "auth_rimap: read (banner): %m");
	(void) close(s);
	return strdup("NO [ALERT] error synchronizing with remote authentication server");
    }
    rbuf[rc] = '\0';			/* tie off response */
    c = strpbrk(rbuf, "\r\n");
    if (c != NULL) {
	*c = '\0';			/* tie off line termination */
    }

    if (!strncmp(rbuf, "* NO", sizeof("* NO")-1)) {
	(void) close(s);
	return strdup(RESP_UNAVAILABLE);
    }
    if (!strncmp(rbuf, "* BYE", sizeof("* BYE")-1)) {
	(void) close(s);
	return strdup(RESP_UNAVAILABLE);
    }
    if (strncmp(rbuf, "* OK", sizeof("* OK")-1)) {
	syslog(LOG_WARNING,
	       "auth_rimap: unexpected response during initial handshake: %s",
	       rbuf);
	(void) close(s);
	return strdup(RESP_UNEXPECTED);
    }
    
    /* build the LOGIN command */

    qlogin = qstring(login);		/* quote login */
    qpass = qstring(password);		/* quote password */
    if (qlogin == NULL) {
	if (qpass != NULL) {
	    memset(qpass, 0, strlen(qpass));
	    free(qpass);
	}
	(void) close(s);
	syslog(LOG_WARNING, "auth_rimap: qstring(login) == NULL");
	return strdup(RESP_IERROR);
    }
    if (qpass == NULL) {
	if (qlogin != NULL) {
	    memset(qlogin, 0, strlen(qlogin));
	    free(qlogin);
	}
	(void) close(s);
	syslog(LOG_WARNING, "auth_rimap: qstring(password) == NULL");
	return strdup(RESP_IERROR);
    }

    iov[0].iov_base = LOGIN_CMD;
    iov[0].iov_len  = sizeof(LOGIN_CMD) - 1;
    iov[1].iov_base = qlogin;
    iov[1].iov_len  = strlen(qlogin);
    iov[2].iov_base = " ";
    iov[2].iov_len  = sizeof(" ") - 1;
    iov[3].iov_base = qpass;
    iov[3].iov_len  = strlen(qpass);
    iov[4].iov_base = "\r\n";
    iov[4].iov_len  = sizeof("\r\n") - 1;

    if (flags & VERBOSE) {
	syslog(LOG_DEBUG, "auth_rimap: sending %s%s %s",
	       LOGIN_CMD, qlogin, qpass);
    }
    alarm(NETWORK_IO_TIMEOUT);
    rc = retry_writev(s, iov, 5);
    alarm(0);
    if (rc == -1) {
	syslog(LOG_WARNING, "auth_rimap: writev: %m");
	memset(qlogin, 0, strlen(qlogin));
	free(qlogin);
	memset(qpass, 0, strlen(qlogin));
	free(qpass);
	(void)close(s);
	return strdup(RESP_IERROR);
    }

    /* don't need these any longer */
    memset(qlogin, 0, strlen(qlogin));
    free(qlogin);
    memset(qpass, 0, strlen(qlogin));
    free(qpass);

    /* read and parse the LOGIN response */

    alarm(NETWORK_IO_TIMEOUT);
    rc = read(s, rbuf, sizeof(rbuf));
    alarm(0);
    (void) close(s);			/* we're done with the remote */
    if (rc == -1) {
	syslog(LOG_WARNING, "auth_rimap: read (response): %m");
	return strdup(RESP_IERROR);
    }

    rbuf[rc] = '\0';			/* tie off response */
    c = strpbrk(rbuf, "\r\n");
    if (c != NULL) {
	*c = '\0';			/* tie off line termination */
    }

     if (!strncmp(rbuf, TAG " OK", sizeof(TAG " OK")-1)) {
	if (flags & VERBOSE) {
	    syslog(LOG_DEBUG, "auth_rimap: [%s] %s", login, rbuf);
	}
	return strdup("OK remote authentication successful");
    }
    if (!strncmp(rbuf, TAG " NO", sizeof(TAG " NO")-1)) {
	if (flags & VERBOSE) {
	    syslog(LOG_DEBUG, "auth_rimap: [%s] %s", login, rbuf);
	}
	return strdup("NO remote server rejected your credentials");
    }
    syslog(LOG_WARNING, "auth_rimap: unexpected response to auth request: %s",
	   rbuf);
    return strdup(RESP_UNEXPECTED);
    
}

/* END FUNCTION: auth_rimap */

/* END MODULE: auth_rimap */
