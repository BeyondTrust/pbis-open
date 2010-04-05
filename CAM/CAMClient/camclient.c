#include "stdafx.h"


DWORD
CamJoinComputeNode(
    RPC_BINDING_HANDLE hBinding,
	PWSTR pszFQDN,
	PWSTR * ppszMachinePassword
	)
{
	DWORD dwError = 0;

	dwError = RpcCamJoinComputeNode( 
		            hBinding,
					pszFQDN,
					ppszMachinePassword
					);
    return dwError;
}

