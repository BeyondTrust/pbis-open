#ifndef __MCXUTIL_H__
#define __MCXUTIL_H__

void
FreeMCXValueList(
    PMCXVALUE pValueList
    );

DWORD
ReadMCXValueFromFile(
    PSTR pszMCXValueFile,
    PMCXVALUE * ppMCXValue
    );

DWORD
ConvertMCXValuesToMCXSettings(
    PMCXVALUE pMCXValueList,
    PSTR      pszPolicyPath,
    DWORD     dwPolicyType
    );

DWORD
GetFileNameForMCXSettings(
    DWORD  dwPolicyType,
    PSTR * ppszFileName
    );

DWORD
GetCurrentMCXSettingsForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    BOOLEAN * pfMachinePolicyExists,
    BOOLEAN * pfUserPolicyExists,
    PSTR *    ppszMachinePolicyPath,
    PSTR *    ppszUserPolicyPath
    );

#endif /* __MCXUTIL_H__ */
