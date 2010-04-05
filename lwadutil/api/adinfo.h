#ifndef __ADINFO_H__
#define __ADINFO_H__

DWORD
ADUGetAllPolicies(
    HANDLE hDirectory,
    DWORD  dwPolicyType,
    PCSTR  pszDN,
    PSTR   pszClientGUID,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    );

DWORD
ADUGetPolicy(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszGPOName,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    );

DWORD
ADUParseAndGetGPTVersion(
    PSTR   pszFilePath,
    PDWORD pdwGPTVersion
    );

DWORD
ADUParseAndSetGPTVersion(
    PSTR  pszFilePath,
    DWORD dwGPTVersion
    );

DWORD
ADUGetGPTFileAndVersionNumber(
    PSTR   pszgPCFileSysPath,
    PSTR * ppszGPTFilePath,
    PDWORD pdwGPTVersion,
    PSTR   pszPath
    );

DWORD
ADUSetGPTVersionNumber(
    PSTR  pszgPCFileSysPath,
    DWORD dwFileVersion,
    PSTR  pszPath
    );

VOID
ADUGetComputerAndUserVersionNumbers(
    DWORD dwVersion,
    PWORD pwUserVersion,
    PWORD pwComputerVersion
    );

DWORD
ADUGetVersionFromUserAndComputer(
    WORD wUser,
    WORD wComputer
    );

DWORD
ADUGetPolicyInformation(
    HANDLE hDirectory,
    PSTR   pszPolicyDN,
    PGROUP_POLICY_OBJECT pGroupPolicyObject
    );

DWORD
ADUSetPolicyVersionInAD(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD dwVersion
    );

#endif /* __ADINFO_H__ */
