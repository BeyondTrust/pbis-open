DWORD
CamJoinComputeNode(
    RPC_BINDING_HANDLE hBinding,
	PWSTR pszFQDN,
	PWSTR * ppszMachinePassword
	);

DWORD
CamClientCreateRpcBinding(
	LPWSTR pwszHostName,
    RPC_BINDING_HANDLE* phCamServiceHandle
	);

size_t
CamClientGetErrorString(
    DWORD dwError,
	LPWSTR pszBuffer,
    size_t stBufSize
    );