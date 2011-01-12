#ifndef LWAUTOENROLL_LDAP_H
#define LWAUTOENROLL_LDAP_H

#include <bail.h>

#include <ldap.h>

#define BAIL_WITH_LDAP_ERROR(_ldapError) \
    do { \
        _BAIL_WITH_LW_ERROR(LwMapLdapErrorToLwError(_ldapError), \
                ": LDAP error %d (%s)", _ldapError, \
                    ldap_err2string(_ldapError)); \
    } while (0)

#define BAIL_ON_LDAP_ERROR(_ldapError) \
    do { \
        if ((_ldapError) != 0) \
        { \
            BAIL_WITH_LDAP_ERROR(_ldapError); \
        } \
    } while (0)

DWORD
LwAutoEnrollFindDomainDn(
        PCSTR dn,
        PCSTR* pDomainDn
        );

DWORD
LwAutoEnrollLdapSearch(
        IN HANDLE ldapConnection,
        IN PCSTR domainDn,
        IN PCSTR searchBase,
        IN int scope,
        IN PCSTR queryString,
        IN PSTR *pAttributeNames,
        OUT LDAPMessage **ppLdapResults
        );

DWORD
LwAutoEnrollLdapConnect(
        IN PCSTR domainDnsName,
        OUT HANDLE *pLdapConnection
        );

#endif /* LWAUTOENROLL_LDAP_H */
