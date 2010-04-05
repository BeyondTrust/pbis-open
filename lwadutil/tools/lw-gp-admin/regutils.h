/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#ifndef ___LWUTIL_REG_H___
#define ____LWUTIL_REG_H___

#include <uuid/uuid.h>
#include <lw/types.h>
#include <reg/lwreg.h>
typedef struct __GPUSER {
    uid_t uid;
    PSTR  pszName;
    PSTR  pszUserPrincipalName;
    PSTR  pszSID;
    PSTR  pszHomeDir;
} GPUSER, *PGPUSER;

typedef struct _GROUP_POLICY_CLIENT_EXTENSION {
    PSTR pszName;
    PSTR pszGUID;
    DWORD dwNoMachinePolicy;
    struct _GROUP_POLICY_CLIENT_EXTENSION * pNext;
} GROUP_POLICY_CLIENT_EXTENSION, *PGROUP_POLICY_CLIENT_EXTENSION;

DWORD
GetCSEListFromRegistry(
    PSTR pszKeyPath,
    PGROUP_POLICY_CLIENT_EXTENSION * ppGroupPolicyClientExtensions
    );

void
FreeClientExtensionList(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtensionList
    );

#endif /* __LWUTIL_REG_H__ */
