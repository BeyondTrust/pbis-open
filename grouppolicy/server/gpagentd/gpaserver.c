#include "includes.h"

const int LISTEN_Q = 5;

/* Group Policy Thread Routine */
static void*
group_policy_thread_routine(
    PVOID pData
    )
{
    PGPASERVERCONNECTIONCONTEXT pContext = NULL;

    if (pData != NULL) {
        pthread_detach(pthread_self());

        pContext = (PGPASERVERCONNECTIONCONTEXT)(pData);

        // This routine is responsible for closing the descriptor
        handle_group_policy(pContext);

    }

    return 0;
}


DWORD
gpa_open_connection(
    int fd,
    uid_t peerUID,
    gid_t peerGID,
    PGPASERVERCONNECTIONCONTEXT *phConnection
    )
{
    DWORD dwError = 0;
    PGPASERVERCONNECTIONCONTEXT pConnection = NULL;

    dwError = LwAllocateMemory(
                    sizeof(GPASERVERCONNECTIONCONTEXT),
                    (PVOID*)&pConnection);
    BAIL_ON_CENTERIS_ERROR(dwError);

    pConnection->handle = (HANDLE) (size_t) fd;
    pConnection->peerUID = peerUID;
    pConnection->peerGID = peerGID;
    pConnection->bConnected = TRUE;

    *phConnection = pConnection;

cleanup:

    return dwError;

error:

    *phConnection = NULL;

    goto cleanup;
}


PVOID
gpa_listener_main()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszCommPath = NULL;
    int sockfd = 0;
    uid_t peerUID = 0;
    gid_t peerGID = 0;
    PGPASERVERCONNECTIONCONTEXT pContext = NULL;
    pthread_t threadId;
    struct sockaddr_un cliaddr;
    SOCKLEN_T len = 0;
    int connfd = -1;
    struct sockaddr_un servaddr;


    ceError = LwAllocateStringPrintf(&pszCommPath,
                                     "%s" PATH_SEPARATOR_STR ".gpagentd",
                                     CACHEDIR);
    BAIL_ON_CENTERIS_ERROR(ceError);

    unlink(pszCommPath);

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Set our listener socket to close on exec */
    ceError = GPASetCloseOnExec(sockfd);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, pszCommPath,sizeof(servaddr.sun_path) - 1);

    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /*
     * Allow anyone to write to the socket.
     * We check the uids against the messages.
     */
    ceError = LwChangeOwnerAndPermissions(pszCommPath, 0, 0, S_IRWXU|S_IRWXG|S_IRWXO);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (listen(sockfd, LISTEN_Q) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while (!ProcessShouldExit())
    {
        memset(&cliaddr, 0, sizeof(cliaddr));
        len = sizeof(cliaddr);

        connfd = accept(sockfd, (struct sockaddr*)&cliaddr, &len);
        if (ProcessShouldExit())
        {
            goto done;
        }
        if (connfd < 0)
        {
            if (errno == EPROTO || errno == ECONNABORTED || errno == EINTR) {

                continue;

            } else {

                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);

            }
        }

        /* Set our connection socket to close on exec */
        ceError = GPASetCloseOnExec(connfd);
        BAIL_ON_CENTERIS_ERROR(ceError);

        
        ceError = gpa_open_connection(
                           connfd,
                           peerUID,
                           peerGID,
                           &pContext);
        BAIL_ON_CENTERIS_ERROR(ceError);

        connfd = -1;

        pthread_create(&threadId, NULL, &group_policy_thread_routine, (PVOID)pContext);
        pContext = NULL;
    }

done:
error:

    if (pContext) {
        if (pContext->handle) {
            close((int) (size_t) pContext->handle);
        }
        LwFreeMemory(pContext);
    }

    if (connfd >= 0) {
        close(connfd);
    }

    if (sockfd > 0)    {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
    }

    LW_SAFE_FREE_STRING(pszCommPath);

    GPA_LOG_INFO("Group Policy Listener stopped.");

    return NULL;
}

