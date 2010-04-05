#include "stdafx.h"


int
main()
{
    RPC_STATUS status = 0;      
	RPC_BINDING_HANDLE hCamService_handle = 0;	
	LPWSTR pwszMachinePassword = NULL;
	unsigned long ulExpCode = 0;
	wchar_t szError[128] = {0};
 
    status = CamClientCreateRpcBinding(NULL, &hCamService_handle);
	BAIL_ON_ERROR(status);
 
    RpcTryExcept  
    {
        status = CamJoinComputeNode(hCamService_handle, L"wfutest.corpqa.centeris.com", &pwszMachinePassword);
		BAIL_ON_ERROR(status);		

		wprintf(L"The password we get from CamRpcServer is: \"%s\"\n",pwszMachinePassword);

    }
    RpcExcept(1) 
    {
        ulExpCode = RpcExceptionCode();
        printf("CamClientTest - Runtime reported exception 0x%lx = %ld\n", ulExpCode, ulExpCode);
    }
    RpcEndExcept

cleanup:
 
    status = RpcBindingFree(&hCamService_handle);
	BAIL_ON_ERROR(status);

	return status;
    
error:

    CamClientGetErrorString(status,
		                    szError,
						    sizeof(szError)-1);
	wprintf(L"CamClientTest Error - 0x%lx = %ld [%s]\n",status, status, szError);	

	goto cleanup;	
}