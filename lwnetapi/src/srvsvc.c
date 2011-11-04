#include "includes.h"

NET_API_STATUS
NetApiBufferAllocate(
    DWORD  dwCount,
    PVOID* ppBuffer
    )
{
    NET_API_STATUS status = 0;
    PVOID pBuffer = NULL;

    if (!ppBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = LwAllocateMemory(dwCount, &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}


NET_API_STATUS
NetServerGetInfoA(
    PSTR   pszServername,  /* IN    OPTIONAL */
    DWORD  dwInfoLevel,    /* IN             */
    PBYTE* ppBuffer        /*    OUT         */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PBYTE pBuffer        = NULL;

    if (!ppBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetServerGetInfoW(pwszServername, dwInfoLevel, &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetServerGetInfoW(
    PWSTR  pwszServername, /* IN    OPTIONAL */
    DWORD  dwInfoLevel,    /* IN             */
    PBYTE* ppBuffer        /*    OUT         */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE pBuffer = NULL;

    if (!ppBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrServerGetInfo(
                    pContext,
                    pwszServername,
                    dwInfoLevel,
                    &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    *ppBuffer = NULL;

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetServerSetInfoA(
    PSTR   pszServername,  /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE  pBuffer,        /* IN              */
    PDWORD pdwParmError    /*    OUT OPTIONAL */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    DWORD dwParmError = 0;

    if (!pBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetServerSetInfoW(
                pwszServername,
                dwInfoLevel,
                pBuffer,
                pdwParmError ? &dwParmError : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    if (pdwParmError)
    {
        *pdwParmError = 0;
    }

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    return status;

error:

    if (pdwParmError)
    {
        *pdwParmError = dwParmError;
    }

    goto cleanup;
}

NET_API_STATUS
NetServerSetInfoW(
    PWSTR  pwszServername, /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE  pBuffer,        /* IN              */
    PDWORD pdwParmError    /*    OUT OPTIONAL */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    DWORD dwParmError = 0;

    if (!pBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrServerSetInfo(
                    pContext,
                    pwszServername,
                    dwInfoLevel,
                    pBuffer,
                    &dwParmError);
    BAIL_ON_NETAPI_ERROR(status);

    *pdwParmError = 0;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (pdwParmError)
    {
        *pdwParmError = dwParmError;
    }

    goto cleanup;
}

NET_API_STATUS
NetShareEnumA(
    PSTR   pszServername,  /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer,       /*    OUT          */
    DWORD  dwPrefmaxLen,   /* IN              */
    PDWORD pdwEntriesRead, /*    OUT          */
    PDWORD pdwTotalEntries,/*    OUT          */
    PDWORD pdwResumeHandle /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PBYTE pBuffer = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetShareEnumW(
                pwszServername,
                dwInfoLevel,
                &pBuffer,
                dwPrefmaxLen,
                &dwEntriesRead,
                &dwTotalEntries,
                pdwResumeHandle ? &dwResumeHandle : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer        = pBuffer;
    *pdwEntriesRead  = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer        = NULL;
    }

    if (pdwEntriesRead)
    {
        *pdwEntriesRead  = 0;
    }

    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetShareEnumW(
    PWSTR  pwszServername, /* IN     OPTIONAL */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer,       /*    OUT          */
    DWORD  dwPrefmaxLen,   /* IN              */
    PDWORD pdwEntriesRead, /*    OUT          */
    PDWORD pdwTotalEntries,/*    OUT          */
    PDWORD pdwResumeHandle /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE pBuffer        = NULL;
    DWORD dwEntriesRead  = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrShareEnum(
                    pContext,
                    pwszServername,
                    dwInfoLevel,
                    &pBuffer,
                    dwPrefmaxLen,
                    &dwEntriesRead,
                    &dwTotalEntries,
                    &dwResumeHandle);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;
    *pdwEntriesRead = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;
    *pdwResumeHandle = dwResumeHandle;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }
    if (pdwEntriesRead)
    {
        *pdwEntriesRead = 0;
    }
    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetShareGetInfoA(
    PSTR   pszServername,  /* IN     OPTIONAL */
    PSTR   pszNetname,     /* IN              */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer        /*    OUT          */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername  = NULL;
    PWSTR pwszNetname     = NULL;
    PBYTE pBuffer         = NULL;

    if (!ppBuffer || !pszNetname)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = LwMbsToWc16s(pszNetname, &pwszNetname);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetShareGetInfoW(
                pwszServername,
                pwszNetname,
                dwInfoLevel,
                &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer        = pBuffer;

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    if (pwszNetname)
    {
        LwFreeMemory(pwszNetname);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer        = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetShareGetInfoW(
    PWSTR  pwszServername, /* IN     OPTIONAL */
    PWSTR  pwszNetname,    /* IN              */
    DWORD  dwInfoLevel,    /* IN              */
    PBYTE* ppBuffer        /*    OUT          */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE pBuffer = NULL;

    if (!ppBuffer || !pwszNetname)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrShareGetInfo(
                    pContext,
                    pwszServername,
                    pwszNetname,
                    dwInfoLevel,
                    &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetShareSetInfoA(
    PSTR    pszServername,  /* IN     OPTIONAL */
    PSTR    pszNetname,     /* IN              */
    DWORD   dwInfoLevel,    /* IN              */
    PBYTE   pBuffer,        /* IN              */
    PDWORD  pdwParmError    /*    OUT          */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PWSTR pwszNetname = NULL;
    DWORD dwParmError = 0;

    if (!pBuffer || !pszNetname)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = LwMbsToWc16s(pszNetname, &pwszNetname);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetShareSetInfoW(
                pwszServername,
                pwszNetname,
                dwInfoLevel,
                pBuffer,
                pdwParmError ? &dwParmError : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    if (pdwParmError)
    {
        *pdwParmError = 0;
    }

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    if (pwszNetname)
    {
        LwFreeMemory(pwszNetname);
    }

    return status;

error:

    if (pdwParmError)
    {
        *pdwParmError = dwParmError;
    }

    goto cleanup;
}

NET_API_STATUS
NetShareSetInfoW(
    PWSTR   pwszServername, /* IN     OPTIONAL */
    PWSTR   pwszNetname,    /* IN              */
    DWORD   dwInfoLevel,    /* IN              */
    PBYTE   pBuffer,        /* IN              */
    PDWORD  pdwParmError    /*    OUT          */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    DWORD dwParmError = 0;

    if (!pBuffer || !pwszNetname)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrShareSetInfo(
                    pContext,
                    pwszServername,
                    pwszNetname,
                    dwInfoLevel,
                    pBuffer,
                    &dwParmError);
    BAIL_ON_NETAPI_ERROR(status);

    if (pdwParmError)
    {
        *pdwParmError = 0;
    }

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (pdwParmError)
    {
        *pdwParmError = dwParmError;
    }

    goto cleanup;
}

NET_API_STATUS
NetShareAddA(
    PSTR    pszServername,  /* IN              */
    DWORD   dwInfoLevel,    /* IN              */
    PBYTE   pBuffer,        /* IN              */
    PDWORD  pdwParmError    /*    OUT          */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    DWORD dwParmError = 0;

    if (!pBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetShareAddW(
                pwszServername,
                dwInfoLevel,
                pBuffer,
                pdwParmError ? &dwParmError : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    if (pdwParmError)
    {
        *pdwParmError = 0;
    }

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    return status;

error:

    if (pdwParmError)
    {
        *pdwParmError = dwParmError;
    }

    goto cleanup;
}

NET_API_STATUS
NetShareAddW(
    PWSTR   pwszServername, /* IN     OPTIONAL */
    DWORD   dwInfoLevel,    /* IN              */
    PBYTE   pBuffer,        /* IN              */
    PDWORD  pdwParmError    /*    OUT          */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    DWORD dwParmError = 0;

    if (!pBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrShareAdd(
                    pContext,
                    pwszServername,
                    dwInfoLevel,
                    pBuffer,
                    &dwParmError);
    BAIL_ON_NETAPI_ERROR(status);

    *pdwParmError = 0;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (pdwParmError)
    {
        *pdwParmError = dwParmError;
    }

    goto cleanup;
}

NET_API_STATUS
NetShareDelA(
    PSTR    pszServername,  /* IN     OPTIONAL */
    PSTR    pszNetname,     /* IN              */
    DWORD   dwReserved      /* IN              */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PWSTR pwszNetname = NULL;

    if (!pszNetname)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = LwMbsToWc16s(pszNetname, &pwszNetname);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetShareDelW(
                pwszServername,
                pwszNetname,
                dwReserved);
    BAIL_ON_NETAPI_ERROR(status);

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    if (pwszNetname)
    {
        LwFreeMemory(pwszNetname);
    }

    return status;

error:

    goto cleanup;
}

NET_API_STATUS
NetShareDelW(
    PWSTR   pwszServername, /* IN     OPTIONAL */
    PWSTR   pwszNetname,    /* IN              */
    DWORD   dwReserved      /* IN              */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;

    if (!pwszNetname)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrShareDel(
                    pContext,
                    pwszServername,
                    pwszNetname,
                    dwReserved);
    BAIL_ON_NETAPI_ERROR(status);

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    goto cleanup;
}

NET_API_STATUS
NetSessionEnumA(
    PSTR    pszServername,    /* IN     OPTIONAL */
    PSTR    pszUncClientname, /* IN     OPTIONAL */
    PSTR    pszUsername,      /* IN     OPTIONAL */
    DWORD   dwInfoLevel,      /* IN              */
    PBYTE*  ppBuffer,         /*    OUT          */
    DWORD   dwPrefmaxLen,     /* IN              */
    PDWORD  pdwEntriesRead,   /*    OUT          */
    PDWORD  pdwTotalEntries,  /*    OUT          */
    PDWORD  pdwResumeHandle   /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PWSTR pwszUncClientname = NULL;
    PWSTR pwszUsername = NULL;
    PBYTE pBuffer = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszUncClientname)
    {
        status = LwMbsToWc16s(pszUncClientname, &pwszUncClientname);
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszUsername)
    {
        status = LwMbsToWc16s(pszUsername, &pwszUsername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetSessionEnumW(
                pwszServername,
                pwszUncClientname,
                pwszUsername,
                dwInfoLevel,
                &pBuffer,
                dwPrefmaxLen,
                &dwEntriesRead,
                &dwTotalEntries,
                pdwResumeHandle ? &dwResumeHandle : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer        = pBuffer;
    *pdwEntriesRead  = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    if (pwszUncClientname)
    {
        LwFreeMemory(pwszUncClientname);
    }

    if (pwszUsername)
    {
        LwFreeMemory(pwszUsername);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pdwEntriesRead)
    {
        *pdwEntriesRead  = 0;
    }

    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetSessionEnumW(
    PWSTR   pwszServername,    /* IN     OPTIONAL */
    PWSTR   pwszUncClientname, /* IN     OPTIONAL */
    PWSTR   pwszUsername,      /* IN     OPTIONAL */
    DWORD   dwInfoLevel,       /* IN              */
    PBYTE*  ppBuffer,          /*    OUT          */
    DWORD   dwPrefmaxLen,      /* IN              */
    PDWORD  pdwEntriesRead,    /*    OUT          */
    PDWORD  pdwTotalEntries,   /*    OUT          */
    PDWORD  pdwResumeHandle    /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS status = 0;
    NET_API_STATUS ret = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE pBuffer        = NULL;
    DWORD dwEntriesRead  = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pdwResumeHandle)
    {
        dwResumeHandle = *pdwResumeHandle;
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrSessionEnum(
                    pContext,
                    pwszServername,
                    pwszUncClientname,
                    pwszUsername,
                    dwInfoLevel,
                    &pBuffer,
                    dwPrefmaxLen,
                    &dwEntriesRead,
                    &dwTotalEntries,
                    &dwResumeHandle);
    if (status == ERROR_MORE_DATA)
    {
        ret = status;
        status = ERROR_SUCCESS;
    }
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;
    *pdwEntriesRead = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }

cleanup:
    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    if (status == ERROR_SUCCESS &&
        ret != ERROR_SUCCESS)
    {
        status = ret;
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }
    if (pdwEntriesRead)
    {
        *pdwEntriesRead = 0;
    }
    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetSessionDelA(
    PSTR    pszServername,     /* IN     OPTIONAL */
    PSTR    pszUncClientname,  /* IN     OPTIONAL */
    PSTR    pszUsername        /* IN     OPTIONAL */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PWSTR pwszUncClientname = NULL;
    PWSTR pwszUsername = NULL;

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszUncClientname)
    {
        status = LwMbsToWc16s(pszUncClientname, &pwszUncClientname);
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszUsername)
    {
        status = LwMbsToWc16s(pszUsername, &pwszUsername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetSessionDelW(
                pwszServername,
                pwszUncClientname,
                pwszUsername);
    BAIL_ON_NETAPI_ERROR(status);

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    if (pwszUncClientname)
    {
        LwFreeMemory(pwszUncClientname);
    }

    if (pwszUsername)
    {
        LwFreeMemory(pwszUsername);
    }

    return status;

error:

    goto cleanup;
}

NET_API_STATUS
NetSessionDelW(
    PWSTR   pwszServername,    /* IN     OPTIONAL */
    PWSTR   pwszUncClientname, /* IN     OPTIONAL */
    PWSTR   pwszUsername       /* IN     OPTIONAL */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrSessionDel(
                    pContext,
                    pwszServername,
                    pwszUncClientname,
                    pwszUsername);
    BAIL_ON_NETAPI_ERROR(status);

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    goto cleanup;
}

NET_API_STATUS
NetConnectionEnumA(
    PSTR    pszServername,     /* IN     OPTIONAL */
    PSTR    pszQualifier,      /* IN              */
    DWORD   dwInfoLevel,       /* IN              */
    PBYTE*  ppBuffer,          /*    OUT          */
    DWORD   dwPrefmaxlen,      /* IN              */
    PDWORD  pdwEntriesRead,    /*    OUT          */
    PDWORD  pdwTotalEntries,   /*    OUT          */
    PDWORD  pdwResumeHandle    /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PWSTR pwszQualifier = NULL;
    PBYTE pBuffer = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszQualifier)
    {
        status = LwMbsToWc16s(pszQualifier, &pwszQualifier);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetConnectionEnumW(
                pwszServername,
                pwszQualifier,
                dwInfoLevel,
                &pBuffer,
                dwPrefmaxlen,
                &dwEntriesRead,
                &dwTotalEntries,
                pdwResumeHandle ? &dwResumeHandle : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer        = pBuffer;
    *pdwEntriesRead  = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    if (pwszQualifier)
    {
        LwFreeMemory(pwszQualifier);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pdwEntriesRead)
    {
        *pdwEntriesRead  = 0;
    }

    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetConnectionEnumW(
    PWSTR   pwszServername,    /* IN     OPTIONAL */
    PWSTR   pwszQualifier,     /* IN              */
    DWORD   dwInfoLevel,       /* IN              */
    PBYTE*  ppBuffer,          /*    OUT          */
    DWORD   dwPrefmaxLen,      /* IN              */
    PDWORD  pdwEntriesRead,    /*    OUT          */
    PDWORD  pdwTotalEntries,   /*    OUT          */
    PDWORD  pdwResumeHandle    /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE pBuffer        = NULL;
    DWORD dwEntriesRead  = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrConnectionEnum(
                    pContext,
                    pwszServername,
                    pwszQualifier,
                    dwInfoLevel,
                    &pBuffer,
                    dwPrefmaxLen,
                    &dwEntriesRead,
                    &dwTotalEntries,
                    &dwResumeHandle);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;
    *pdwEntriesRead = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;
    *pdwResumeHandle = dwResumeHandle;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }
    if (pdwEntriesRead)
    {
        *pdwEntriesRead = 0;
    }
    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetFileEnumA(
    PSTR   pszServername,      /* IN     OPTIONAL */
    PSTR   pszBasepath,        /* IN     OPTIONAL */
    PSTR   pszUsername,        /* IN     OPTIONAL */
    DWORD  dwInfoLevel,        /* IN              */
    PBYTE* ppBuffer,           /*    OUT          */
    DWORD  dwPrefmaxlen,       /* IN              */
    PDWORD pdwEntriesRead,     /*    OUT          */
    PDWORD pdwTotalEntries,    /*    OUT          */
    PDWORD pdwResumeHandle     /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PWSTR pwszBasepath = NULL;
    PWSTR pwszUsername = NULL;
    PBYTE pBuffer = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszBasepath)
    {
        status = LwMbsToWc16s(pszBasepath, &pwszBasepath);
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszUsername)
    {
        status = LwMbsToWc16s(pszUsername, &pwszUsername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetFileEnumW(
                pwszServername,
                pwszBasepath,
                pwszUsername,
                dwInfoLevel,
                &pBuffer,
                dwPrefmaxlen,
                &dwEntriesRead,
                &dwTotalEntries,
                pdwResumeHandle ? &dwResumeHandle : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer        = pBuffer;
    *pdwEntriesRead  = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    if (pwszBasepath)
    {
        LwFreeMemory(pwszBasepath);
    }

    if (pwszUsername)
    {
        LwFreeMemory(pwszUsername);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pdwEntriesRead)
    {
        *pdwEntriesRead  = 0;
    }

    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }

    if (pdwResumeHandle)
    {
        *pdwResumeHandle = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetFileEnumW(
    PCWSTR pwszServername,     /* IN    OPTIONAL  */
    PCWSTR pwszBasepath,       /* IN    OPTIONAL  */
    PCWSTR pwszUsername,       /* IN    OPTIONAL  */
    DWORD  dwInfoLevel,        /* IN              */
    PBYTE* ppBuffer,           /*    OUT          */
    DWORD  dwPrefmaxLen,       /* IN              */
    PDWORD pdwEntriesRead,     /*    OUT          */
    PDWORD pdwTotalEntries,    /*    OUT          */
    PDWORD pdwResumeHandle     /* IN OUT OPTIONAL */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE pBuffer        = NULL;
    DWORD dwEntriesRead  = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    if (!ppBuffer || !pdwEntriesRead || !pdwTotalEntries)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrFileEnum(
                    pContext,
                    pwszServername,
                    pwszBasepath,
                    pwszUsername,
                    dwInfoLevel,
                    &pBuffer,
                    dwPrefmaxLen,
                    &dwEntriesRead,
                    &dwTotalEntries,
                    pdwResumeHandle ? &dwResumeHandle : NULL);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;
    *pdwEntriesRead = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }
    if (pdwEntriesRead)
    {
        *pdwEntriesRead = 0;
    }
    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetFileGetInfoA(
    PSTR          pszServername,      /* IN    OPTIONAL  */
    DWORD         dwFileId,           /* IN              */
    DWORD         dwInfoLevel,        /* IN              */
    PBYTE*        ppBuffer            /*    OUT          */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PBYTE pBuffer = NULL;

    if (!ppBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetFileGetInfoW(
                pwszServername,
                dwFileId,
                dwInfoLevel,
                &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetFileGetInfoW(
    PCWSTR          pwszServername,    /* IN    OPTIONAL  */
    DWORD           dwFileId,          /* IN              */
    DWORD           dwInfoLevel,       /* IN              */
    PBYTE*          ppBuffer           /*    OUT          */
    )
{
    NET_API_STATUS  status   = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE           pBuffer  = NULL;

    if (!ppBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrFileGetInfo(
                pContext,
                pwszServername,
                dwFileId,
                dwInfoLevel,
                &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetFileCloseA(
    PSTR   pszServername,      /* IN    OPTIONAL  */
    DWORD  dwFileId            /* IN              */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetFileCloseW(pwszServername, dwFileId);
    BAIL_ON_NETAPI_ERROR(status);

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    return status;

error:

    goto cleanup;
}

NET_API_STATUS
NetFileCloseW(
    PCWSTR pwszServername,     /* IN    OPTIONAL  */
    DWORD  dwFileId            /* IN              */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrFileClose(pContext, pwszServername, dwFileId);
    BAIL_ON_NETAPI_ERROR(status);

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    goto cleanup;
}

NET_API_STATUS
NetRemoteTODA(
    PSTR   pszServername,      /* IN    OPTIONAL  */
    PBYTE* ppBuffer            /*    OUT          */
    )
{
    NET_API_STATUS status = 0;
    PWSTR pwszServername = NULL;
    PBYTE pBuffer = NULL;

    if (!ppBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    if (pszServername)
    {
        status = LwMbsToWc16s(pszServername, &pwszServername);
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = NetRemoteTODW(pwszServername, &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    if (pwszServername)
    {
        LwFreeMemory(pwszServername);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
NetRemoteTODW(
    PWSTR  pwszServername,     /* IN    OPTIONAL  */
    PBYTE* ppBuffer            /*    OUT          */
    )
{
    NET_API_STATUS  status = 0;
    PSRVSVC_CONTEXT pContext = NULL;
    PBYTE           pBuffer = NULL;

    if (!ppBuffer)
    {
        status = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETAPI_ERROR(status);
    }

    status = SrvSvcCreateContext(pwszServername, &pContext);
    BAIL_ON_NETAPI_ERROR(status);

    status = NetrRemoteTOD(pContext, pwszServername, &pBuffer);
    BAIL_ON_NETAPI_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:

    if (pContext)
    {
        SrvSvcCloseContext(pContext);
    }

    return status;

error:

    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    goto cleanup;
}

