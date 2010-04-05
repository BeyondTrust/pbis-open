#ifndef __GPAADINFO_H__
#define __GPAADINFO_H__

CENTERROR
GPAGetPolicyLinkInfo(
    HANDLE hDirectory,
    PSTR pszComputerDN,
    BOOLEAN bAddEnforcedOnly,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects,
    DWORD * pdwOptions
    );

CENTERROR
GPAGetPolicyInformation(
    PGPUSER pUser,
    HANDLE hDirectory,
    PSTR   pszPolicyDN,
    PGROUP_POLICY_OBJECT pGroupPolicyObject
    );


#endif /* __GPAADINFO_H__ */
