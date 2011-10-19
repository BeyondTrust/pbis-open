#ifndef __LWKRB5_H__
#define __LWKRB5_H__

DWORD
DNSKrb5Init(
    PCSTR pszAccountName,
    PCSTR pszDomain
    );

DWORD
DNSKrb5Shutdown(
    VOID
    );

#endif
