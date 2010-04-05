/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        gpauthsvc.h
 *
 * Abstract:
 *
 *        Likewise Group Policy
 *
 *        Authentication Service Interface
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __GPAUTHSVC_H__
#define __GPAUTHSVC_H__

CENTERROR
GPAInitAuthService();

CENTERROR
GPAGetCurrentDomain(
	PSTR* ppszDomain
	);

CENTERROR
GPAIsJoinedToAD(
	PBOOLEAN pbIsJoined
	);

CENTERROR
GPAInitKerberos(
    DWORD dwPollingInterval
    );

CENTERROR
GPAGetPrincipalFromKrb5Cache(
    PCSTR pszCachePath,
    PSTR * ppszPrincipal
    );

CENTERROR
GPAGetShortDomainName(
    PCSTR pszDomainName,
    PSTR* ppszShortDomainName
    );

CENTERROR
GPAGetPreferredDC(
	PSTR* ppszDC
	);

CENTERROR
GPAGetPreferredDomainController(
	PCSTR pszDomain,
	PSTR* ppszDomainController
	);

CENTERROR
GPAGetDomainController(
	PCSTR pszDomain,
	PSTR* ppszDomainController
	);

CENTERROR
GPAGetDnsSystemNames(
        PSTR* ppszHostName,
        PSTR* ppszMachineName,
        PSTR* ppszDomain
        );

CENTERROR
GPAEnsureAuthIsRunning(
    VOID
    );

CENTERROR
GPAGetDomainSID(
	PSTR* ppszDomainSID
	);

CENTERROR
GPAFindUserByName(
    PCSTR pszLoginId,
    PGPUSER* ppGPUser
    );

CENTERROR
GPAFindUserById(
    PCSTR    pszLoginId,
	uid_t    uid,
	PGPUSER* ppGPUser
	);

VOID
GPAFreeUser(
    PGPUSER pUser
    );

#endif /* __GPAUTHSVC_H__ */

