/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */   
/* Portions
 * Copyright 2000, John E. Schimmel, All rights reserved.
 * This software is not subject to any license of Mirapoint, Inc.
 *
 * This is free software; you can redistribute and use it
 * under the same terms as OpenLDAP itself.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Horrible Darwin/BSD hack */
#if (defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)) && defined(_POSIX_C_SOURCE)
#undef _POSIX_C_SOURCE
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#ifndef UUID_BUILD_STANDALONE      
#include <dce/dce.h>
#include <dce/dce_utils.h>   
#else
#include "uuid.h"
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
/* Bizarre hack for HP-UX ia64 where a system header
 * makes reference to a kernel-only data structure
 */
#if defined(__hpux) && defined(__ia64) && !defined(_DEFINED_MPINFOU)
union mpinfou {};
#endif
#include <net/if.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef notdef
#include <sys/sysctl.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
void dce_get_802_addr(dce_802_addr_t *addr, error_status_t *st)
{
	char buf[sizeof(struct ifreq) * 128];
	struct ifconf ifc;
	struct ifreq *ifr;
	int s, i;
	struct sockaddr *sa ATTRIBUTE_UNUSED;
#ifdef AF_LINK
	struct sockaddr_dl *sdl;
#endif
#if defined(HAVE_NET_IF_ARP_H) && defined(SIOCGARP) && !defined(SIOCGIFHWADDR)
        union
        {
            struct arpreq arpreq;
#ifdef AF_LINK
            struct
            {
                struct sockaddr pa;
                struct sockaddr_dl ha;
            } arpreq_dl;
#endif
        } u;
#endif
	struct ifreq ifreq;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		*st = utils_s_802_cant_read;
		return;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	memset(buf, 0, sizeof(buf));

	i = ioctl(s, SIOCGIFCONF, (char *)&ifc);
	if (i < 0) {
		*st = utils_s_802_cant_read;
		close(s);
		return;
	}

	for (i = 0; i < ifc.ifc_len; ) {
		ifr = (struct ifreq *)&ifc.ifc_buf[i];
		/* Initialize sa here because it is used under the next
		 * label, and the loopback check could jump directly
		 * to the next label.
		 */
#ifdef AF_LINK
		sa = &ifr->ifr_addr;
#else
		sa = NULL;
#endif

#ifdef SIOCGIFFLAGS
		/* Skip over loopback and point-to-point interfaces. */
		memcpy(&ifreq, ifr, sizeof(ifreq));
		if (ioctl(s, SIOCGIFFLAGS, &ifreq) == 0) {
			if (ifreq.ifr_flags & (IFF_POINTOPOINT|IFF_LOOPBACK)) {
			  goto next;
			}
		}
#endif

#ifdef AF_LINK
		if (sa->sa_family == AF_LINK) {
			sdl = (struct sockaddr_dl *)sa;
			if (sdl->sdl_alen == 6) {
				memcpy(addr->eaddr, (unsigned char *)sdl->sdl_data + sdl->sdl_nlen, 6);
				*st = error_status_ok;
				close(s);
				return;
			}
		}
#endif /* AF_LINK */

#if defined(SIOCGIFHWADDR)
		memcpy(&ifreq, ifr, sizeof(ifreq));
		if (ioctl(s, SIOCGIFHWADDR, &ifreq) == 0) {
			memcpy(addr->eaddr, &ifreq.ifr_hwaddr.sa_data, 6);
			*st = error_status_ok;
			close(s);
			return;
		}
#elif defined(SIOCGARP)
		memset(&u.arpreq, 0, sizeof(u.arpreq));
		u.arpreq.arp_pa = ifr->ifr_dstaddr;
		u.arpreq.arp_flags = 0;			
		if (ioctl(s, SIOCGARP, &u.arpreq) == 0) {
#ifdef AF_LINK
			sdl = &u.arpreq_dl.ha;
			memcpy(addr->eaddr, (unsigned const char *)&sdl->sdl_data + sdl->sdl_nlen, 6);
#else
			memcpy(addr->eaddr, (unsigned char*)&u.arpreq.arp_ha.sa_data[0], 6);
#endif
			*st = error_status_ok;
			close(s);
			return;
		}
#elif defined(SIOCGENADDR)
		memcpy(&ifreq, ifr, sizeof(ifreq));
		if (ioctl(s, SIOCGENADDR, &ifreq) == 0) {
			memcpy(addr->eaddr, ifreq.ifr_enaddr, 6);
			*st = error_status_ok;
			close(s);
			return;
		}
#else
		//#error Please implement dce_get_802_addr() for your operating system
#endif

	next:
#if !defined(__svr4__) && !defined(linux) && !defined(_HPUX) /* XXX FixMe to be portable */
		if (sa && sa->sa_len > sizeof(ifr->ifr_addr)) {
			i += sizeof(ifr->ifr_name) + sa->sa_len;
		} else
#endif
			i += sizeof(*ifr);

	}

	{
	    unsigned int seed = 1;
	    unsigned char* buf = (unsigned char*) addr->eaddr;
	    int i;
	    
	    for (i = 0; i < 6; i++)
	    {
		buf[i] = rand_r(&seed);
	    }

	    close(s);
	    return;
	}

	*st = utils_s_802_cant_read;
	close(s);
	return;
}

