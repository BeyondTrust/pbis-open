
#include "stdafx.h"

DWORD
CamClientCreateRpcBinding(
	LPWSTR pwszHostName,
    RPC_BINDING_HANDLE* phCamServiceHandle
	)
{
	RPC_STATUS status = 0;       
	RPC_BINDING_HANDLE CamService_IfHandle = NULL;
	LPWSTR pwszStringBinding = NULL;	
	
	status = RpcStringBindingCompose(NULL,
                                     CAM_PROTOCOL_SEQUENCE,
                                     NULL,
                                     CAM_END_PORT,									 
                                     NULL,
                                     &pwszStringBinding);
	BAIL_ON_ERROR(status);
    
    status = RpcBindingFromStringBinding(pwszStringBinding,
                                         &CamService_IfHandle);
 
    BAIL_ON_ERROR(status); 

#if 0
	status = RpcBindingSetAuthInfoW((BindingHandle), (srv_principal), (prot_level), (auth_mech), (IdHandle), (auth_svc));
	BAIL_ON_ERROR(status);
#endif

	*phCamServiceHandle = CamService_IfHandle;

cleanup:
	status = RpcStringFree(&pwszStringBinding); 
	BAIL_ON_ERROR(status);   
	

	return status;

error:
    RpcBindingFree(&CamService_IfHandle);	

	goto cleanup; 
}
