#define BAIL_ON_ERROR(dwError) if(dwError) goto error;

#define IsNullOrEmptyString(str) (!(str) || !(*(str)))

#define BAIL_ON_INVALID_STRING(str)            \
    do {                                       \
        if (IsNullOrEmptyString(str)) {        \
            dwError = ERROR_INVALID_PARAMETER; \
            BAIL_ON_ERROR(dwError);            \
        }                                      \
    } while(0)

#define CAM_SAFE_FREE_MEMORY(mem)              \
	do {                                       \
	    if (mem)                               \
	    {                                      \
	        CamFreeMemory(mem);                \
		    (mem) = NULL;                      \
		}                                      \
	} while(0)


DWORD
CamAllocateMemory(
    size_t sSize,
    LPVOID* ppMemory
    );

VOID
CamFreeMemory(
	LPVOID pMemory
	);

DWORD
CamAllocateString(
	LPWSTR pwszInputString,
	LPWSTR* ppwszOutputString
	);

VOID
CamFreeString(
	LPWSTR pszString
	);

void __RPC_USER midl_user_free(void __RPC_FAR * p);

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes);
