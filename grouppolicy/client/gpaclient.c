#include "includes.h"


CENTERROR
GPOClientOpenContext(
    PHANDLE phGPConnection
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szClientPath[PATH_MAX+1];
    DWORD dwFd = -1;
    BOOLEAN bFileExists = FALSE;
    PGPOCLIENTCONNECTIONCONTEXT pContext = NULL;
    struct sockaddr_un unixaddr;

    if ((dwFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    memset(&unixaddr, 0, sizeof(unixaddr));
    unixaddr.sun_family = AF_UNIX;
    sprintf(szClientPath, "/var/tmp/.gpclient_%05ld", (long)getpid());
    strcpy(unixaddr.sun_path, szClientPath);

    ceError = GPACheckSockExists(unixaddr.sun_path, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
       ceError = LwRemoveFile(unixaddr.sun_path);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (bind(dwFd, (struct sockaddr*)&unixaddr, sizeof(unixaddr)) < 0) {
       ceError = LwMapErrnoToLwError(errno);
       goto error;
    }

    ceError = LwChangePermissions(unixaddr.sun_path, S_IRWXU);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset(&unixaddr, 0, sizeof(unixaddr));
    unixaddr.sun_family = AF_UNIX;
    sprintf(unixaddr.sun_path, "%s/.gpagentd", CACHEDIR);

    if (connect(dwFd, (struct sockaddr*)&unixaddr, sizeof(unixaddr)) < 0) {
        
       ceError = LwMapErrnoToLwError(errno);
       goto error;
    }

    ceError = LwAllocateMemory(sizeof(GPOCLIENTCONNECTIONCONTEXT),
                                (PVOID*)&pContext);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pContext->handle = (HANDLE) (size_t) dwFd;
    dwFd = -1;
    strcpy(pContext->szPath, szClientPath);

    //Call SendCreds()
    ceError = GPOSendCreds((DWORD) (size_t) pContext->handle);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOClientVerifyConnection(pContext);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *phGPConnection = (HANDLE)pContext;

    return (ceError);

  error:

    if (pContext) {
       GPOClientCloseContext((HANDLE)pContext);
    }
    *phGPConnection = (HANDLE)NULL;

    if (dwFd >= 0) {
        close(dwFd);
    }

    return (ceError);
}

CENTERROR
GPOSetLogLevel(
    HANDLE hGPConnection,
    DWORD dwLogLevel
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = (PGPOCLIENTCONNECTIONCONTEXT)hGPConnection;
    PGPOMESSAGE pMessage = NULL;
    PSTR pszData = NULL;

    ceError = LwAllocateStringPrintf(&pszData, "%d", dwLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOBuildMessage(SET_LOG_LEVEL,
                  sizeof(pszData),
                  1,
                  1,
                  &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pMessage->pData, pszData);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    pMessage = NULL;
    ceError = GPOReadNextMessage(&pContext->handle, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pMessage->header.messageType != SET_LOG_LEVEL_SUCCESSFUL) {
       ceError = CENTERROR_GP_SETLOGLEVEL_FAILED; 
    }

cleanup:

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    LW_SAFE_FREE_STRING(pszData);

    return (ceError);

 error:

    goto cleanup;

}

CENTERROR
GPOClientRefresh(
    HANDLE hGPConnection
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = (PGPOCLIENTCONNECTIONCONTEXT)hGPConnection;
    PGPOMESSAGE pMessage = NULL;

    ceError = GPOBuildMessage(REFRESH_GPO,
                  0,
                  1,
                  1,
                  &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    pMessage = NULL;
    ceError = GPOReadNextMessage(&pContext->handle, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pMessage->header.messageType != GPO_REFRESH_SUCCESSFUL) {
       ceError = CENTERROR_GP_REFRESH_FAILED;
    }

    if (pMessage != NULL) {
           GPOFreeMessage(&pMessage);
    }

    return (ceError);

 error:

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    return (ceError);
}

CENTERROR
GPOClientSignalDomainLeave(
    HANDLE hGPConnection
    )
{
    
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = (PGPOCLIENTCONNECTIONCONTEXT)hGPConnection;
    PGPOMESSAGE pMessage = NULL;

    ceError = GPOBuildMessage(LEAVE_DOMAIN, 0, 1, 1, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(pMessage != NULL)
    {
        GPOFreeMessage(&pMessage);
    }

    pMessage = NULL;
    ceError = GPOReadNextMessage(&pContext->handle, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(pMessage->header.messageType != LEAVE_SUCCESSFUL)
    {
        ceError = CENTERROR_DOMAIN_LEAVE_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

 error:
    if(pMessage != NULL)
    {
        GPOFreeMessage(&pMessage);
    }


    return ceError;
}

CENTERROR
GPOClientSignalDomainJoin(
    HANDLE hGPConnection
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = (PGPOCLIENTCONNECTIONCONTEXT)hGPConnection;
    PGPOMESSAGE pMessage = NULL;

    ceError = GPOBuildMessage(JOIN_DOMAIN, 0, 1, 1, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(pMessage != NULL)
    {
        GPOFreeMessage(&pMessage);
    }

    pMessage = NULL;
    ceError = GPOReadNextMessage(&pContext->handle, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(pMessage->header.messageType != JOIN_SUCCESSFUL)
    {
        ceError = CENTERROR_DOMAIN_JOIN_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

 error:
    if(pMessage != NULL)
    {
        GPOFreeMessage(&pMessage);
    }

    return ceError;
}

CENTERROR
GPOClientProcessLogin(
    HANDLE hGPConnection,
    PCSTR Username
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = (PGPOCLIENTCONNECTIONCONTEXT)hGPConnection;
    PGPOMESSAGE pMessage = NULL;

    if (IsNullOrEmptyString(Username)) {
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPOBuildMessage(PROCESS_LOGIN,
                  strlen(Username),
                  1,
                  1,
                  &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pMessage->pData, Username);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    pMessage = NULL;
    ceError = GPOReadNextMessage(&pContext->handle, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_LOG_VERBOSE("received message type is %d",
                    pMessage->header.messageType);

    if (pMessage->header.messageType != PROCESS_LOGIN_SUCCESS) {
       ceError = CENTERROR_GP_LOGIN_POLICY_FAILED;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
        pMessage = NULL;
    }

 error:

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    return (ceError);
}

CENTERROR
GPOClientProcessLogout(
    HANDLE hGPConnection,
    PCSTR  pszUsername)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = (PGPOCLIENTCONNECTIONCONTEXT)hGPConnection;
    PGPOMESSAGE pMessage = NULL;

    if (IsNullOrEmptyString(pszUsername)) {
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPOBuildMessage(PROCESS_LOGOUT,
                  strlen(pszUsername),
                  1,
                  1,
                  &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pMessage->pData, pszUsername);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    pMessage = NULL;
    ceError = GPOReadNextMessage(&pContext->handle, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_LOG_VERBOSE("received message type is %d",
                    pMessage->header.messageType);

    if (pMessage->header.messageType != PROCESS_LOGOUT_SUCCESS) {
       ceError = CENTERROR_GP_LOGOUT_POLICY_FAILED;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
        pMessage = NULL;
    }

 error:

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    return (ceError);
}

CENTERROR
GPOClientVerifyConnection(
    PGPOCLIENTCONNECTIONCONTEXT pContext
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOMESSAGE pMessage = NULL;

    ceError = GPOBuildMessage(REQUEST_CONNECT,
                              0,
                              1,
                              1,
                              &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPOFreeMessage(&pMessage);

    pMessage = NULL;
    ceError = GPOReadNextMessage(&pContext->handle, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pMessage->header.messageType != CONNECTED)
    {
        ceError = CENTERROR_INVALID_MESSAGE;
        GPA_LOG_ERROR("Unexpected message from the server. Expected [%d]; Received [%d].",
                      CONNECTED,
                      pMessage->header.messageType);
    }

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    return (ceError);

  error:

    if (pMessage != NULL) {
        GPOFreeMessage(&pMessage);
    }

    return ceError;
}


CENTERROR
GPOClientSendMessage(
    PGPOCLIENTCONNECTIONCONTEXT pContext,
    PGPOMESSAGE pMessage
    )
{
    return GPOWriteMessage(&pContext->handle, pMessage);
}

CENTERROR
GPOClientGetNextMessage(
    PGPOCLIENTCONNECTIONCONTEXT pContext,
    PGPOMESSAGE* ppMessage
    )
{
    return GPOReadNextMessage(&pContext->handle, ppMessage);
}

CENTERROR
GPOClientCloseContext(
    HANDLE hGPConnection
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = (PGPOCLIENTCONNECTIONCONTEXT)hGPConnection;
    BOOLEAN bFileExists = FALSE;

    if (pContext == NULL) {
       return ceError;
    }

    if (pContext->handle >= 0) {
        close((int) (size_t) pContext->handle);
    }

    if (strlen(pContext->szPath)) {
       ceError = GPACheckSockExists(pContext->szPath, &bFileExists);
       BAIL_ON_CENTERIS_ERROR(ceError);

       if (bFileExists) {
          LwRemoveFile(pContext->szPath);
       }
    }

    LwFreeMemory(pContext);

error:

    return ceError;
}
