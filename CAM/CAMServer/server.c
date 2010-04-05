#include "stdafx.h"

DWORD
RpcCamJoinComputeNode(
    handle_t hBinding,
	LPWSTR pszFQDN,
	LPWSTR *ppszMachinePassword
	)
{
	DWORD dwError = 0;
	LPWSTR pszMachinePassword = NULL;
	LPWSTR pszMachineName = NULL;
	DWORD dwLevel = 1;
	DWORD dwParmErr = 0;
	USER_INFO_1 ui = {0};
	

	dwError = CamValidateFQDNwithDomain(
					gpszDomainFQDN,
					pszFQDN
					);
	BAIL_ON_ERROR(dwError);

	dwError = CamGenerateNetBiosName(
					pszFQDN,
					&pszMachineName
					);
	BAIL_ON_ERROR(dwError);

	dwError = CamGenerateMachinePassword(
					pszFQDN,
					&pszMachinePassword
					);
	BAIL_ON_ERROR(dwError);

	ui.usri1_name = pszMachineName;
	ui.usri1_password = pszMachinePassword,
	ui.usri1_priv = USER_PRIV_USER;
	ui.usri1_home_dir = NULL;
	ui.usri1_comment = NULL;
	ui.usri1_flags = UF_SCRIPT;
	ui.usri1_script_path = NULL;
   
	
    dwError = NetUserAdd(
				gpszDomainController,
				dwLevel,
				(LPBYTE)&ui,
				&dwParmErr
				);                  
	BAIL_ON_ERROR(dwError);	

	*ppszMachinePassword = pszMachinePassword;	

cleanup:

	CAM_SAFE_FREE_MEMORY(pszMachineName);

	return dwError;

error:

	CAM_SAFE_FREE_MEMORY(pszMachinePassword);

	goto cleanup;
}


DWORD
CamValidateFQDNwithDomain(
	LPWSTR pszDomainFQDN,
	LPWSTR pszInputFQDN
	)
{
	DWORD dwError = 0;
	LPCWSTR pszInputDomainFQDN = NULL;
	LPWSTR pszName = NULL;	
	wchar_t ch = '.';
	LPCWSTR pszDotPartofName = NULL;

	BAIL_ON_INVALID_STRING(pszInputFQDN);

	pszInputDomainFQDN = wcsstr(pszInputFQDN, pszDomainFQDN);

	if (!wcscmp(pszDomainFQDN, pszInputFQDN) ||
		IsNullOrEmptyString(pszInputDomainFQDN) || 
		wcscmp(pszInputDomainFQDN, pszDomainFQDN))
	{
		dwError = ERROR_INVALID_PARAMETER;
		BAIL_ON_ERROR(dwError);
	}

	dwError = CamAllocateMemory((pszInputDomainFQDN-pszInputFQDN+1)*sizeof(*pszInputFQDN),
		                        (PVOID)&pszName);
	BAIL_ON_ERROR(dwError);

	memcpy(pszName, pszInputFQDN, (pszInputDomainFQDN-pszInputFQDN)*sizeof(*pszInputFQDN));

	pszDotPartofName = wcschr(pszName, ch);
	if (IsNullOrEmptyString(pszDotPartofName) || 
		(pszDotPartofName-pszName) != wcslen(pszName)-1)
	{	
		dwError = ERROR_INVALID_PARAMETER;
		BAIL_ON_ERROR(dwError);
	}
	
cleanup:
	CAM_SAFE_FREE_MEMORY(pszName);	

	return dwError;
	
error:
	goto cleanup;
}

DWORD
CamGenerateNetBiosName(
	LPWSTR pszFQDN,
	LPWSTR * ppszMachineName
	)
{
	DWORD dwError = 0;		
	wchar_t ch = '.';	
	size_t sMachineNameLen = 0;	
	LPWSTR pszMachineName = NULL;			

    sMachineNameLen = wcslen(pszFQDN) - wcslen(wcschr(pszFQDN, ch));

	dwError = CamAllocateMemory((sMachineNameLen+2)*sizeof(*pszFQDN),
		                        (PVOID)&pszMachineName);
	BAIL_ON_ERROR(dwError);

	memcpy(pszMachineName, pszFQDN, sMachineNameLen*sizeof(*pszFQDN));

	pszMachineName[sMachineNameLen] = (wchar_t)'$';

    *ppszMachineName = pszMachineName;
	
cleanup:
	return dwError;

error:
	CAM_SAFE_FREE_MEMORY(pszMachineName);	

	goto cleanup;
}

DWORD 
CamGenerateMachinePassword(
	LPWSTR pszFQDN,
	LPWSTR* ppszMachinePassword
	)
{
	DWORD dwError = 0;	
	wchar_t buffer[100] = {0};
	LPWSTR pszMachinePassword = NULL;

	BAIL_ON_INVALID_STRING(pszFQDN);

    srand((unsigned)time(NULL) + wcslen(pszFQDN));

	_itow_s(rand(), buffer, 100, 10);

	dwError = CamAllocateString(buffer, &pszMachinePassword);
	BAIL_ON_ERROR(dwError);

	*ppszMachinePassword = pszMachinePassword;

cleanup:

	return dwError;

error:	
    CAM_SAFE_FREE_MEMORY(pszMachinePassword);	

	goto cleanup;
}

DWORD
CamInitialize()
{
	DWORD dwError = 0;	
    LPWSTR pwszDomainName = NULL;
    NETSETUP_JOIN_STATUS camJoinStatus = NetSetupUnknownStatus; 
	PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;    


	dwError = NetGetJoinInformation(NULL, &pwszDomainName, &camJoinStatus);
	BAIL_ON_ERROR(dwError);

	if (camJoinStatus != NetSetupDomainName)
	{
		dwError = ERROR_INVALID_SERVER_STATE;
		BAIL_ON_ERROR(dwError);
	}

	dwError = DsGetDcName(NULL,
		                  pwszDomainName,
						  NULL,NULL,
						  DS_RETURN_DNS_NAME,
						  &pDcInfo);
	BAIL_ON_ERROR(dwError);  

	dwError = CamAllocateString(pDcInfo->DomainName, &gpszDomainFQDN);
	BAIL_ON_ERROR(dwError);  

	dwError = CamAllocateString(pDcInfo->DomainControllerName, &gpszDomainController);
	BAIL_ON_ERROR(dwError);  

cleanup:
	if (pDcInfo)
	{
	    NetApiBufferFree(pDcInfo);	
	}
	if (pwszDomainName)
	{
	    NetApiBufferFree((PVOID)pwszDomainName);	
	}

	return dwError;

error:
	goto cleanup;
}

DWORD
CamCreateMachineAccount(
	LPWSTR pszDomainName,
	LPWSTR pszFQDN,
	LPWSTR pszAccountOU
	)
{
	DWORD dwError = 0;
	


	return dwError;
}
