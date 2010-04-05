DWORD 
CamStartRpcServer(
    RPC_BINDING_VECTOR ** server_binding
	);

DWORD
CamStopRpcServer(
    RPC_BINDING_VECTOR * pServerBinding
	);


// internal cam server routines

DWORD
CamValidateFQDNwithDomain(
	LPWSTR pszDomainFQDN,
	LPWSTR pszInputFQDN
	);

DWORD
CamGenerateNetBiosName(
	LPWSTR pszFQDN,
	LPWSTR * ppszMachineName
	);

DWORD 
CamGenerateMachinePassword(
	LPWSTR pszFQDN,
	LPWSTR * ppszMachinePassword
	);

DWORD
CamCreateMachineAccount(
	LPWSTR pszDomainName,
	LPWSTR pszFQDN,
	LPWSTR pszAccountOU
	);

DWORD
CamInitialize(
	);
