


DWORD
CamClientAllocateMemory(
    size_t sSize,
    LPVOID* ppMemory
    );

VOID
CamClientFreeMemory(
	LPVOID pMemory
	);

DWORD
CamClientAllocateString(
	LPWSTR pszString,
	LPWSTR * ppszNewString
	);

VOID
CamClientFreeString(
	LPWSTR pszString
	);

