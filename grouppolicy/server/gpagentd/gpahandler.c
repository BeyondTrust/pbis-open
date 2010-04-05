#include "includes.h"

static
int
process_connect_request(
    PGPASERVERCONNECTIONCONTEXT pContext
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PGPOMESSAGE pMessage = NULL;

    ceError = GPOBuildMessage(CONNECTED,
                              0, /* message length */
                              1, /* start          */
                              1, /* end            */
                              &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPOFreeMessage(&pMessage);

    return (ceError);

error:

    if (pMessage) {
        GPOFreeMessage(&pMessage);
    }

    return (ceError);
}

#if 0
static
CENTERROR
process_domain_join_signal(
        PGPASERVERCONNECTIONCONTEXT pContext
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GroupPolicyMessageType retval;
    PGPOMESSAGE pMessage = NULL;

    retval = handle_domain_join_signal();

    ceError = GPOBuildMessage(retval, 0, 1, 1, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
error:
    if(pMessage != NULL)
        GPOFreeMessage(&pMessage);

    return ceError;
}

static
CENTERROR
process_domain_leave_signal(
        PGPASERVERCONNECTIONCONTEXT pContext
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GroupPolicyMessageType retval;
    PGPOMESSAGE pMessage = NULL;

    retval = handle_domain_leave_signal();

    ceError = GPOBuildMessage(retval, 0, 1, 1, &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);
error:
    if(pMessage != NULL)
        GPOFreeMessage(&pMessage);

    return ceError;
}
#endif

static
CENTERROR
process_gpo_refresh_request(
    PGPASERVERCONNECTIONCONTEXT pContext
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD responseMsgType = GPO_REFRESH_SUCCESSFUL;
//    DWORD msgHandlingResult = 0;
    CHAR  szBuf[32];
    PGPOMESSAGE pMessage = NULL;

    ceError = GPARefreshMachinePolicies();
    if (ceError != CENTERROR_SUCCESS) {
        responseMsgType = GPO_REFRESH_FAILED;
    }
    sprintf(szBuf, "%d", ceError);
    ceError = GPOBuildMessage(responseMsgType,
                              strlen(szBuf), /* message length */
                              1, /* start          */
                              1, /* end            */
                              &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);
    strcpy(pMessage->pData, szBuf);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);

error:

    if (pMessage) {
        GPOFreeMessage(&pMessage);
    }

    return (ceError);
}

static
CENTERROR
process_gpo_login(
        PGPASERVERCONNECTIONCONTEXT pContext,
        const PGPOMESSAGE pMessage
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD responseMsgType = PROCESS_LOGIN_SUCCESS;
    CHAR  szBuf[32];
    PGPOMESSAGE pOutMessage = NULL;
    PSTR pszUsername = NULL;

    if (pMessage->header.messageLength > 0)
    {
        ceError = LwAllocateMemory(pMessage->header.messageLength+1,
                                   (PVOID*)&pszUsername);
        BAIL_ON_CENTERIS_ERROR(ceError);

        memcpy(pszUsername, pMessage->pData, pMessage->header.messageLength);

        ceError = GPAProcessLogin(pszUsername);
        
    } else {
        
        ceError = CENTERROR_GP_LOGIN_POLICY_FAILED;
        
    }
    
    if (!CENTERROR_IS_OK(ceError)) {
        responseMsgType = PROCESS_LOGIN_FAILED;
    }
    sprintf(szBuf, "%d", ceError);
    ceError = GPOBuildMessage(responseMsgType,
                              strlen(szBuf), /* message length */
                              1, /* start          */
                              1, /* end            */
                              &pOutMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);
    strcpy(pMessage->pData, szBuf);

    ceError = GPOWriteMessage(&pContext->handle, pOutMessage);

error:

    if (pOutMessage) {
        GPOFreeMessage(&pOutMessage);
    }

    LW_SAFE_FREE_STRING(pszUsername);

    return (ceError);
}

static
CENTERROR
process_gpo_logout(
        PGPASERVERCONNECTIONCONTEXT pContext,
        const PGPOMESSAGE pMessage
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD responseMsgType = PROCESS_LOGOUT_SUCCESS;
    CHAR  szBuf[32];
    PGPOMESSAGE pOutMessage = NULL;
    PSTR pszUsername = NULL;

    if (pMessage->header.messageLength > 0)
    {
        ceError = LwAllocateMemory(pMessage->header.messageLength+1,
                                   (PVOID*)&pszUsername);
        BAIL_ON_CENTERIS_ERROR(ceError);

        memcpy(pszUsername, pMessage->pData, pMessage->header.messageLength);

        ceError = GPAProcessLogout(pszUsername);
        
    } else {
        
        ceError = CENTERROR_GP_LOGOUT_POLICY_FAILED;
        
    }
    
    if (!CENTERROR_IS_OK(ceError)) {
        responseMsgType = PROCESS_LOGOUT_FAILED;
    }
    sprintf(szBuf, "%d", ceError);
    ceError = GPOBuildMessage(responseMsgType,
                              strlen(szBuf), /* message length */
                              1, /* start          */
                              1, /* end            */
                              &pOutMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);
    strcpy(pMessage->pData, szBuf);

    ceError = GPOWriteMessage(&pContext->handle, pOutMessage);

error:

    if (pOutMessage) {
        GPOFreeMessage(&pOutMessage);
    }

    LW_SAFE_FREE_STRING(pszUsername);

    return (ceError);
}

static
CENTERROR
process_set_log_level(
    PGPASERVERCONNECTIONCONTEXT pContext,
    PSTR pszData
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD responseMsgType = SET_LOG_LEVEL_SUCCESSFUL;
    CHAR  szBuf[32];
    PGPOMESSAGE pMessage = NULL;
    DWORD dwLogLevel = LOG_LEVEL_ERROR;

    if(pszData)
    {
        dwLogLevel = (DWORD)atoi(pszData);

        ceError = gpa_set_log_level(dwLogLevel);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        ceError = CENTERROR_GP_SETLOGLEVEL_FAILED;
    }

    if (!CENTERROR_IS_OK(ceError)) {
        responseMsgType = SET_LOG_LEVEL_FAILED;
    }

    sprintf(szBuf, "%d", ceError);

    ceError = GPOBuildMessage(responseMsgType,
                              strlen(szBuf), /* message length */
                              1, /* start          */
                              1, /* end            */
                              &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pMessage->pData, szBuf);

    ceError = GPOWriteMessage(&pContext->handle, pMessage);

error:

    if (pMessage) {
        GPOFreeMessage(&pMessage);
    }

    return (ceError);
}

/* Processes messages from the client */
static
CENTERROR
process_message(
    PGPASERVERCONNECTIONCONTEXT pContext,
    const PGPOMESSAGE pMessage)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    switch(pMessage->header.messageType)
    {
    case SET_LOG_LEVEL:
    {
        ceError = process_set_log_level(pContext,pMessage->pData);
    }
    break;
    case REQUEST_CONNECT:
    {
        ceError = process_connect_request(pContext);
    }
    break;
    case DISCONNECT:
    {
        if (pContext->handle) {
            close((int) (size_t) pContext->handle);
            pContext->handle = NULL;
        }
    }
    break;
    case REFRESH_GPO:
    {
        ceError = process_gpo_refresh_request(pContext);
    }
    break;
    case PROCESS_LOGIN:
    {
        ceError = process_gpo_login(pContext, pMessage);
    }
    break;
    case PROCESS_LOGOUT:
    {
        ceError = process_gpo_logout(pContext, pMessage);
    }
    break;
    default:
    {
        GPA_LOG_ERROR("Unexpected message type [%d]", pMessage->header.messageType);
        ceError = CENTERROR_INVALID_MESSAGE;
    }
    break;
    }
    return ceError;
}

static
CENTERROR
PrepareHandle(
    int fd
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    DWORD dwFlags = fcntl(fd, F_GETFL, 0);
    if (dwFlags < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    dwFlags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, dwFlags) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    return ceError;
}



CENTERROR
VerifyConnection(
    PGPASERVERCONNECTIONCONTEXT pContext
    )
{
    DWORD dwError = 0;
    uid_t uid;
    gid_t gid;

    dwError = GPORecvCreds((int) (size_t) pContext->handle, &uid,&gid);
    BAIL_ON_CENTERIS_ERROR(dwError);

    pContext->peerUID = uid;
    pContext->peerGID = gid;

cleanup:
    return dwError;

error:

    pContext->peerUID = 0;
    pContext->peerGID = 0;

    GPA_LOG_ERROR("Local socket client authentication failed");

    goto cleanup;
}


void
handle_group_policy(
    PGPASERVERCONNECTIONCONTEXT pContext
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOMESSAGE pMessage = NULL;

    ceError = VerifyConnection(pContext);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = PrepareHandle((int) (size_t) pContext->handle);
    BAIL_ON_CENTERIS_ERROR(ceError);

    do
    {
        if (pMessage) {
            GPOFreeMessage(&pMessage);
        }

        ceError = GPOSecureReadNextMessage(&pContext->handle, &pMessage, &pContext->peerUID);
        if (ceError == CENTERROR_CONNECTION_UNAVAIL || ceError == CENTERROR_INVALID_OPERATION) {
            goto error;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pMessage) {
            ceError = process_message(pContext, pMessage);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    } while (pMessage != NULL);

error:

    if (pMessage) {
        GPOFreeMessage(&pMessage);
    }

    if (pContext) {
        if (pContext->handle) {
            close((int) (size_t) pContext->handle);
        }
        LwFreeMemory(pContext);
    }

    return;
}
