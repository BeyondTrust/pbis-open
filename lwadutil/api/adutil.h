#ifndef __ADUTIL_H__
#define __ADUTIL_H__

DWORD
GetCurrentSettingsForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    PSTR 	  pszClientGUID,
    BOOLEAN * pfMachinePolicyExists,
    BOOLEAN * pfUserPolicyExists,
    PSTR *    ppszMachinePolicyPath,
    PSTR *    ppszUserPolicyPath,
    PSTR      pszPath
    );

#endif /* __MCXUTIL_H__ */
