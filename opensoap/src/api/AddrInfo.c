/*-----------------------------------------------------------------------------
 * $RCSfile: AddrInfo.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: AddrInfo.c,v 1.10.4.1 2004/10/12 01:27:23 okada Exp $";
#endif  /* _DEBUG */

#include "AddrInfo.h"
#include <stdlib.h>
#include <string.h>

/*
=begin
= OpenSOAP AddrInfo class
=end
*/

#if defined(HAVE_GETADDRINFO)

extern
int
OPENSOAP_API
OpenSOAPAddrInfoRelease(OpenSOAPAddrInfoPtr res) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (res) {
		freeaddrinfo(res);
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

extern
int
OPENSOAP_API
OpenSOAPAddrInfoCreate(const char * /* [in] */ node,
					   const char * /* [in] */ service,
					   OpenSOAPAddrInfoPtr * /* [out] */ res) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (res) {
		int     gai_error;
		OpenSOAPAddrInfo hints;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family   = AF_UNSPEC; /* IPv4 IPv6 */
		hints.ai_socktype = SOCK_STREAM; /* socket type */
		hints.ai_protocol = IPPROTO_TCP; /* protocol */

		gai_error
			= getaddrinfo(node, service, &hints, res);
		if (gai_error == 0) {
			ret = OPENSOAP_NO_ERROR;
		}
		else {
            /* *****display error details
              fprintf(stderr, "%d:%s\n", gai_error, gai_strerror(gai_error));
            */
            /* gai_error: see manpage of getaddrinfo(3) */
            if (ret == EAI_NONAME || EAI_FAIL || EAI_AGAIN) {
                /* Host not found */
                ret = OPENSOAP_TRANSPORT_HOST_NOT_FOUND;
            } else {
                /* unknown error */
                ret = OPENSOAP_IO_ERROR;
            }
		}
	}

	return ret;
}

#else /* HAVE_GETADDRINFO */

/* lock and unlock declare */
extern
void
OpenSOAPSocketSystemLock();

extern
void
OpenSOAPSocketSystemUnlock();

/* OpenSOAPAddrInfo */

static
OpenSOAPAddrInfoPtr
OpenSOAPAddrInfoAllocate(unsigned short port,
						 const char *h_a,
						 size_t h_length) {
	OpenSOAPAddrInfoPtr ret = malloc(sizeof(OpenSOAPAddrInfo));

	if (ret) {
		memset(ret, 0, sizeof(OpenSOAPAddrInfo));
		ret->ai_addrlen = sizeof(struct sockaddr_in);
		ret->ai_addr = malloc(ret->ai_addrlen);
		if (ret->ai_addr) {
			struct sockaddr_in *sin
				= (struct sockaddr_in *)ret->ai_addr;
			memset(sin, 0, ret->ai_addrlen);
			ret->ai_family = AF_INET;
			ret->ai_socktype = SOCK_STREAM;
			ret->ai_protocol = IPPROTO_TCP;
			sin->sin_family = AF_INET;
			sin->sin_port = port;
			memcpy(&sin->sin_addr, h_a, h_length);
		}
		else {
			free(ret);
			ret = NULL;
		}
	}

	return ret;
}

typedef struct {
	const char *service;
	unsigned short port;
} ServicePortMapItem;

static
const ServicePortMapItem
ServicePortMap[] = {
	{ "http", 80},
	{ NULL, 0}
};

static
unsigned short
getPortNumber(/* [in] */ const char *service) {
	const char PROTO_NAME[] = "tcp";
	unsigned short ret = 0;
	struct servent *sEnt = NULL;
	/* lock */
	OpenSOAPSocketSystemLock();
	/* get port no */
	sEnt = getservbyname(service, PROTO_NAME);
	if (sEnt) {
		ret = sEnt->s_port;
	}
	/* unlock */
	OpenSOAPSocketSystemUnlock();
	if (!sEnt) {
		const ServicePortMapItem *i = ServicePortMap;
		for (;i->service
				 && strcmp(i->service, service) != 0; ++i) {
		}
		ret = (i->service) ? (i->port) : atoi(service);
		ret = htons(ret);
	}

	return ret;
}

extern
int
OPENSOAP_API
OpenSOAPAddrInfoRelease(OpenSOAPAddrInfoPtr res) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (res) {
		while (res) {
			OpenSOAPAddrInfoPtr prev = res;
			res = res->ai_next;
			free(prev->ai_addr);
			free(prev);
		}

		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

extern
int
OPENSOAP_API
OpenSOAPAddrInfoCreate(const char * /* [in] */ node,
					   const char * /* [in] */ service,
					   OpenSOAPAddrInfoPtr * /* [out] */ res) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (res) {
		unsigned short port = getPortNumber(service);
		struct hostent *hp = NULL;
		/* lock */
		OpenSOAPSocketSystemLock();
		/* get host */
		hp = gethostbyname(node);
		if (hp) {
			size_t h_length = hp->h_length;
			OpenSOAPAddrInfoPtr *ai_i = res;
			int i = 0;
			ret = OPENSOAP_NO_ERROR;
			for (; hp->h_addr_list[i]; ++i) {
				*ai_i = OpenSOAPAddrInfoAllocate(port,
												 hp->h_addr_list[i],
												 h_length);
				if (*ai_i) {
					ai_i = &((*ai_i)->ai_next);
				}
				else {
					ret = OPENSOAP_MEM_BADALLOC;
					break;
				}
			}
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPAddrInfoRelease(*res);
			}
		}
#ifndef _WIN32
		/* error handling for gethostbyname() on UNIX ? */
		else {
            if (h_errno == HOST_NOT_FOUND
                || h_errno == NO_RECOVERY
                || h_errno == NO_DATA
                || h_errno == NO_ADDRESS
                || h_errno == TRY_AGAIN) {
                ret = OPENSOAP_TRANSPORT_HOST_NOT_FOUND;
            }
        }
#endif
		/* unlock */
		OpenSOAPSocketSystemUnlock();
		if (!hp) { /* cannot find node */
			ret = OPENSOAP_IO_ERROR;
		}
	}

	return ret;
}

#endif /* HAVE_GETADDRINFO */
