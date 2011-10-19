#ifndef __DNSGSS_H__
#define __DNSGSS_H__

DWORD
DNSNegotiateSecureContext(
	HANDLE      hDNSServer,
	PCSTR       pszDomain,
	PCSTR       pszServerName,
	PCSTR       pszKeyName,
	PCtxtHandle pGSSContext
	);

void
lwdns_display_status(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    );

void
lwdns_display_status_1(
    PCSTR     pszId,
    OM_uint32 code,
    int type
    );

#endif /* __DNSGSS_H__ */
