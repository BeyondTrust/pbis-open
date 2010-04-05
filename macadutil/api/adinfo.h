#ifndef __ADINFO_H__
#define __ADINFO_H__


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
    PDWORD pdwGPTVersion
    );

DWORD
ADUSetGPTVersionNumber(
    PSTR  pszgPCFileSysPath,
    DWORD dwFileVersion
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
ADUGetAllMCXPolicies(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
    );

DWORD
ADUGetMCXPolicy(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszGPOName,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects
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
    DWORD dwVersion);

#endif /* __ADINFO_H__ */
