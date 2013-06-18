/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef LWAUTOENROLL_LDAP_H
#define LWAUTOENROLL_LDAP_H

#include <bail.h>

#include <ldap.h>

#define BAIL_WITH_LDAP_ERROR(_ldapError, ...) \
    do { \
        BAIL_WITH_LW_ERROR(LwMapLdapErrorToLwError(_ldapError), \
            _BAIL_FORMAT_STRING(__VA_ARGS__) ": LDAP error %d (%s)", \
            _BAIL_FORMAT_ARGS(__VA_ARGS__), _ldapError, \
                ldap_err2string(_ldapError)); \
    } while (0)

#define BAIL_ON_LDAP_ERROR(_ldapError, ...) \
    do { \
        if ((_ldapError) != 0) \
        { \
            BAIL_WITH_LDAP_ERROR(_ldapError, ## __VA_ARGS__); \
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
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
