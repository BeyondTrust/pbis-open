/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: AddrInfo.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OpenSOAP_IMPL_AddrInfo_H
#define OpenSOAP_IMPL_AddrInfo_H

#include "Object.h"

#if defined(NONGCC_WIN32)
#include <winsock.h>
#else
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
   
#if defined(HAVE_GETADDRINFO)
	typedef struct addrinfo OpenSOAPAddrInfo;
	typedef struct addrinfo *OpenSOAPAddrInfoPtr;
#else
	typedef struct tagOpenSOAPAddrInfo OpenSOAPAddrInfo;
	typedef OpenSOAPAddrInfo *OpenSOAPAddrInfoPtr;

	struct tagOpenSOAPAddrInfo {
		/* int ai_flags; */
		int ai_family;
		int ai_socktype;
		int ai_protocol;
		int ai_addrlen;
		struct sockaddr *ai_addr;
		/* char *ai_cononname; */
		OpenSOAPAddrInfoPtr ai_next;
	};
#endif

	extern
	int
	OPENSOAP_API
	OpenSOAPAddrInfoRelease(OpenSOAPAddrInfoPtr res);

	extern
	int
	OPENSOAP_API
	OpenSOAPAddrInfoCreate(const char * /* [in] */ node,
						   const char * /* [in] */ service,
						   OpenSOAPAddrInfoPtr * /* [out] */ res);
   
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_AddrInfo_H */
