/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        grouppolicy.h
 *
 * Abstract:
 *
 *        Likewise Group Policy (GPO)
 *
 *        Public Header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 */
#ifndef __GROUPPOLICY_H__
#define __GROUPPOLICY_H__

#ifndef _WIN32
#include <lw/types.h>
#include <lw/attrs.h>
#endif

#ifndef GPO_DEFINED
#define GPO_DEFINED 1
typedef struct _GROUP_POLICY_OBJECT {
    DWORD dwOptions;    // GPLink options - do we care?
    DWORD dwVersion;    // Version - extract this from the gpt.ini
    PSTR pszPolicyDN;
    PSTR pszDSPath;
    PSTR pszDisplayName;
    PSTR pszgPCFileSysPath;
    PSTR pszgPCMachineExtensionNames;
    PSTR pszgPCUserExtensionNames;
    DWORD gPCFunctionalityVersion;
    DWORD dwFlags;
    BOOLEAN bNewVersion;
    struct _GROUP_POLICY_OBJECT *pNext;
} GROUP_POLICY_OBJECT, *PGROUP_POLICY_OBJECT;
#endif

typedef struct __GPUSER {
    uid_t uid;
    PSTR  pszName;
    PSTR  pszUserPrincipalName;
    PSTR  pszSID;
    PSTR  pszHomeDir;
} GPUSER, *PGPUSER;

typedef struct __GPOCLIENTCONNECTIONCONTEXT {
    HANDLE handle;
    CHAR   szPath[256];
} GPOCLIENTCONNECTIONCONTEXT, *PGPOCLIENTCONNECTIONCONTEXT;

typedef struct __GPUSER_AD_ATTRS {
    /* Name attributes */
    PSTR  pszDisplayName;
    PSTR  pszFirstName;
    PSTR  pszLastName;
    PSTR  pszADDomain;
    PSTR  pszKerberosPrincipal;

    /* Email attributes */
    PSTR  pszEMailAddress;
    PSTR  pszMSExchHomeServerName;
    PSTR  pszMSExchHomeMDB;

    /* Phone attributes */
    PSTR  pszTelephoneNumber;
    PSTR  pszFaxTelephoneNumber;
    PSTR  pszMobileTelephoneNumber;

    /* Address attributes */
    PSTR  pszStreetAddress;
    PSTR  pszPostOfficeBox;
    PSTR  pszCity;
    PSTR  pszState;
    PSTR  pszPostalCode;
    PSTR  pszCountry;

    /* Work attributes */
    PSTR  pszTitle;
    PSTR  pszCompany;
    PSTR  pszDepartment;

    /* Network setting attributes */
    PSTR  pszHomeDirectory;
    PSTR  pszHomeDrive;
    PSTR  pszPasswordLastSet;
    PSTR  pszUserAccountControl;
    PSTR  pszMaxMinutesUntilChangePassword;
    PSTR  pszMinMinutesUntilChangePassword;
    PSTR  pszMaxFailedLoginAttempts;
    PSTR  pszAllowedPasswordHistory;
    PSTR  pszMinCharsAllowedInPassword;

} GPUSER_AD_ATTRS, *PGPUSER_AD_ATTRS;

typedef DWORD (*PFNPROCESSGROUPPOLICY)(
    DWORD                dwFlags,
    PGPUSER              pUser,
    PGROUP_POLICY_OBJECT pDeletedGPOList,
    PGROUP_POLICY_OBJECT pChangedGPOList
    );

typedef DWORD (*PFNRESETGROUPPOLICY)(
        PGPUSER pUser
        );

DWORD
GPOClientOpenContext(
    PHANDLE phGPConnection
    );

DWORD
GPOClientCloseContext(
    HANDLE hGPConnection
    );

DWORD
GPOClientProcessLogin(
    HANDLE hGPConnection,
    PCSTR pszUsername);

DWORD
GPOClientProcessLogout(
    HANDLE hGPConnection,
    PCSTR pszUsername);

DWORD
GPOClientRefresh(
    HANDLE hGPConnection
    );

DWORD
GPOClientSignalDomainJoin(
    HANDLE hGPConnection
    );

DWORD
GPOClientSignalDomainLeave(
    HANDLE hGPConnection
    );

DWORD
GPOSetLogLevel(
    HANDLE hGPConnection,
    DWORD dwLogLevel
    );

#endif /* __GROUPPOLICY_H__ */

