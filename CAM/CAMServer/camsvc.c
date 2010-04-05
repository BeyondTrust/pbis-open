#include "stdafx.h"

DWORD
CamServerMain(
    RPC_BINDING_VECTOR ** server_binding
    )
{
	DWORD dwError = 0;

	dwError = CamInitialize();
	BAIL_ON_ERROR(dwError);

	wprintf(L"The gpszDomainFQDN is: \"%s\"\n",gpszDomainFQDN);
	wprintf(L"The gpszDomainController is: \"%s\"\n",gpszDomainController);
	
	dwError = CamStartRpcServer(server_binding);
	BAIL_ON_ERROR(dwError);

error:
	return dwError;
}

DWORD
CamServerCleanup(
    RPC_BINDING_VECTOR * pServerBinding
    )
{
    DWORD dwError = 0;

	dwError = CamStopRpcServer(pServerBinding);
	BAIL_ON_ERROR(dwError);

error:
	return dwError;
}



// for the purpose of debugging camserver
#if 0
int
main()
{
	DWORD dwError = 0;
	LPWSTR pszMachinePassword = NULL;

	dwError = CamInitialize();

    dwError = RpcCamJoinComputeNode(NULL,L"wfutest.corpqa.centeris.com",&pszMachinePassword);

	return dwError;
}
#endif


