#include "includes.h"

#define BAIL_ON_CONNECTION_INVALID(errCode)             \
    if (errCode != CENTERROR_CONNECTION_UNAVAIL) {      \
        BAIL_ON_CENTERIS_ERROR(errCode);                \
    }                                                   \
    else {                                              \
        goto error;                                     \
    }

#define SAFE_ALLOCATION_SIZE(dwSize)                                 \
    if (!(dwSize + 1)) {                                               \
        GPA_LOG_ERROR("Allocation size + 1 overflows, hence returning ..." );  \
        goto error;                                                     \
    }

CENTERROR
GPOCheckIncomingMessageAndSender(
    PGPOMESSAGE pMessage,
    uid_t* ppeerUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    switch(pMessage->header.messageType) {

    case REQUEST_CONNECT:
    case DISCONNECT:
    {
        /*
         * These are valid client messages
         * A client executing in a non-root context
         * can send these to the gpagent.
         */
        if (pMessage->header.messageLength != 0) {
            GPA_LOG_ERROR("Incoming client message has invalid length");
            ceError = LwMapErrnoToLwError(EINVAL);
        }

        break;
    }
    case PROCESS_LOGIN:
    case PROCESS_LOGOUT:
    case SET_LOG_LEVEL:
    {
        if (!pMessage->header.messageLength) {
            GPA_LOG_ERROR("Incoming client message has invalid length");
            ceError = LwMapErrnoToLwError(EINVAL);
        }

        break;
    }

    case REFRESH_GPO:
    {
        /*
         * These are valid client messages
         * Only a client executing in a root context
         * can send these to the gpagent
         */
        if (*ppeerUID != 0) {
            GPA_LOG_ERROR("Peer does not have adequate permissions for request.");
            ceError = LwMapErrnoToLwError(EPERM);
        }
        break;
    }

    default:
        /* CONNECTED
         * REQUEST_LICENSE: -- No longer used
         * LICENSE_VALID    -- No longer used
         * LICENSE_EXPIRED  -- No longer used
         * LICENSE_ERROR    -- No longer used
         * GPO_REFRESH_FAILED
         * GPO_REFRESH_SUCCESSFUL
         * ERROR
         */
    {
        /*
         * These messages are invalid client messages
         * These are messages only the gpagent can send
         */
        GPA_LOG_ERROR("Peer submitted an invalid request");
        ceError = LwMapErrnoToLwError(EINVAL);
        break;
    }
    }

    return ceError;
}

CENTERROR
GPOBuildMessage(
    GroupPolicyMessageType msgType,
    uint32_t msgLen,
    uint16_t iData,
    uint16_t nData,
    PGPOMESSAGE* ppMessage)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PGPOMESSAGE pMessage = NULL;
    PSTR pData = NULL;

    ceError = LwAllocateMemory(sizeof(GPOMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset(pMessage, 0, sizeof(GPOMESSAGE));

    pMessage->header.messageType = msgType;
    pMessage->header.version = 1;
    pMessage->header.reserved[0] = iData;
    pMessage->header.reserved[1] = nData;
    pMessage->header.messageLength = msgLen;

    if (pMessage->header.messageLength > 0)
    {
        ceError = LwAllocateMemory(pMessage->header.messageLength + 1, (PVOID*)&pData);
        BAIL_ON_CENTERIS_ERROR(ceError);
        pMessage->pData = pData;
    }

    *ppMessage = pMessage;

    return (ceError);

error:

    if (pData) {
        LwFreeMemory(pData);
    }

    if (pMessage) {
        LwFreeMemory(pMessage);
    }

    return (ceError);
}

void
GPOFreeMessage(
    PGPOMESSAGE *ppMessage
    )
{
//  CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOMESSAGE pMessage = NULL;

    if (ppMessage != NULL)
    {
        pMessage = *ppMessage;
        if (pMessage != NULL) {
            if (pMessage->pData != NULL) {
                LwFreeMemory(pMessage->pData);
            }

            LwFreeMemory(pMessage);
        }
        *ppMessage = NULL;
    }
}

CENTERROR
GPOReadNextMessage(
    PHANDLE pHandle,
    PGPOMESSAGE* ppMessage
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOMESSAGE pMessage = NULL;
    PSTR pData = NULL;
    DWORD dwFd = -1;
    DWORD dwBytesRead = 0;
    DWORD msgLength = 0;

    ceError = LwAllocateMemory(sizeof(GPOMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);
    memset(pMessage, 0, sizeof(GPOMESSAGE));

    dwFd = (DWORD) (size_t) *(pHandle);

    ceError = ReadData(dwFd, (PSTR)&(pMessage->header), sizeof(GPOMESSAGEHEADER), &dwBytesRead);
    BAIL_ON_CONNECTION_INVALID(ceError);

    if (dwBytesRead != sizeof(GPOMESSAGEHEADER)) {
        ceError = CENTERROR_INVALID_MESSAGE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    msgLength = pMessage->header.messageLength;
    if (msgLength > 0) {

        ceError = LwAllocateMemory(msgLength + 1, (PVOID*)&pData);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = ReadData(dwFd, pData, msgLength, &dwBytesRead);
        BAIL_ON_CONNECTION_INVALID(ceError);

        if (msgLength != dwBytesRead) {
            ceError = CENTERROR_INVALID_MESSAGE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        // We don't null-terminate the data, because it may not be a plain string
        // We should treat it as a byte stream
        pMessage->pData = pData;
    }

    *ppMessage = pMessage;

    return (ceError);

error:

    if (pData) {
        LwFreeMemory(pData);
    }

    if (pMessage) {
        LwFreeMemory(pMessage);
    }

    return (ceError);
}

CENTERROR
GPOSecureReadNextMessage(
    PHANDLE pHandle,
    PGPOMESSAGE* ppMessage,
    uid_t* ppeerUID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOMESSAGE pMessage = NULL;
    PSTR pData = NULL;
    DWORD dwFd = -1;
    DWORD dwBytesRead = 0;
    DWORD msgLength = 0;

    ceError = LwAllocateMemory(sizeof(GPOMESSAGE), (PVOID*)&pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);
    memset(pMessage, 0, sizeof(GPOMESSAGE));

    dwFd = (DWORD) (size_t) *(pHandle);

    ceError = ReadData(dwFd, (PSTR)&(pMessage->header), sizeof(GPOMESSAGEHEADER), &dwBytesRead);
    BAIL_ON_CONNECTION_INVALID(ceError);

    if (dwBytesRead != sizeof(GPOMESSAGEHEADER)) {
        ceError = CENTERROR_INVALID_MESSAGE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPOCheckIncomingMessageAndSender(pMessage, ppeerUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    msgLength = pMessage->header.messageLength;
    if (msgLength > 0) {

        //Fix for Bug#6707
        SAFE_ALLOCATION_SIZE(msgLength);

        ceError = LwAllocateMemory(msgLength + 1, (PVOID*)&pData);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = ReadData(dwFd, pData, msgLength, &dwBytesRead);
        BAIL_ON_CONNECTION_INVALID(ceError);

        if (msgLength != dwBytesRead) {
            ceError = CENTERROR_INVALID_MESSAGE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        // We don't null-terminate the data, because it may not be a plain string
        // We should treat it as a byte stream
        pMessage->pData = pData;
    }

    *ppMessage = pMessage;

    return (ceError);

error:

    if (pData) {
        LwFreeMemory(pData);
    }

    if (pMessage) {
        LwFreeMemory(pMessage);
    }

    return (ceError);
}

CENTERROR
GPOWriteMessage(
    PHANDLE pHandle,
    const PGPOMESSAGE pMessage)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwFd = (DWORD) (size_t) *pHandle;

    ceError = WriteData(dwFd,
                        (const PSTR)&pMessage->header,
                        sizeof(GPOMESSAGEHEADER));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = WriteData(dwFd,
                        pMessage->pData,
                        pMessage->header.messageLength);
    BAIL_ON_CENTERIS_ERROR(ceError);

    return (ceError);

error:

    return (ceError);
}


#ifndef CMSG_ALIGN
#    if defined(_CMSG_DATA_ALIGN)
#        define CMSG_ALIGN _CMSG_DATA_ALIGN
#    elif defined(_CMSG_ALIGN)
#        define CMSG_ALIGN _CMSG_ALIGN
#    endif
#endif

#ifndef CMSG_SPACE
#    define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif

#ifndef CMSG_LEN
#    define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif


CENTERROR
SendMsg(
    DWORD dwFd,
    const struct msghdr *pMsg
    )
{
    DWORD dwError = 0;
    ssize_t result = -1;

    do
    {
        result = sendmsg(dwFd, pMsg, 0);

    } while (result < 0 && (errno == EAGAIN || errno == EINTR));

    if (result < 0)
    {
        dwError = errno;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

error:

    return (dwError);
}



CENTERROR
RecvMsg(
    DWORD dwFd,
    struct msghdr *pMsg
    )
{
    DWORD dwError = 0;
    ssize_t result = -1;

    do
    {
        result = recvmsg(dwFd, pMsg, 0);
    } while (result < 0 && (errno == EAGAIN || errno == EINTR));

    if (result < 0)
    {
        dwError = errno;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

error:

    return (dwError);
}



CENTERROR
GPOSendCreds(
    int fd
    )
{
    DWORD dwError = 0;
    char payload = 0xff;
    int credFd = fd;
    struct iovec payload_vec = {0};
    struct msghdr msg = {0};

#ifdef MSGHDR_HAS_MSG_CONTROL
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(credFd))];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
#endif

    /* Set up dummy payload */
    payload_vec.iov_base = &payload;
    payload_vec.iov_len = sizeof(payload);
    msg.msg_iov = &payload_vec;
    msg.msg_iovlen = 1;

#ifdef MSGHDR_HAS_MSG_CONTROL
    /* Set up ancillary data */
    msg.msg_control = buf_un.buf;
    msg.msg_controllen = sizeof(buf_un.buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(credFd));

    memcpy(CMSG_DATA(cmsg), &credFd, sizeof(credFd));
#else
    msg.msg_accrights = (char*) &credFd;
    msg.msg_accrightslen = sizeof(credFd);
#endif

    /* Send message */
    dwError = SendMsg(fd, &msg);
    BAIL_ON_CENTERIS_ERROR(dwError);

error:

    return dwError;
}

CENTERROR
GPORecvCreds(
    int fd,
    uid_t* pUid,
    uid_t* pGid
    )
{
    DWORD dwError = 0;
    char payload = 0;
    int credFd = -1;
    struct iovec payload_vec = {0};
    struct msghdr msg = {0};
#ifdef MSGHDR_HAS_MSG_CONTROL
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(credFd))];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
#endif
    struct stat statbuf = {0};
    struct sockaddr_un localAddr;
    SOCKLEN_T localAddrLen = sizeof(localAddr);
    struct sockaddr_un credPeerAddr;
    SOCKLEN_T credPeerAddrLen = sizeof(credPeerAddr);

    /* Set up area to receive dummy payload */
    payload_vec.iov_base = &payload;
    payload_vec.iov_len = sizeof(payload);
    msg.msg_iov = &payload_vec;
    msg.msg_iovlen = 1;

#ifdef MSGHDR_HAS_MSG_CONTROL
    /* Set up area to receive ancillary data */
    msg.msg_control = buf_un.buf;
    msg.msg_controllen = sizeof(buf_un.buf);
#else
    msg.msg_accrights = (char*) &credFd;
    msg.msg_accrightslen = sizeof(credFd);
#endif

    dwError = RecvMsg(fd, &msg);
    BAIL_ON_CENTERIS_ERROR(dwError);

    /* Extract credential fd */
    
#ifdef MSGHDR_HAS_MSG_CONTROL
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
        {
            memcpy(&credFd, CMSG_DATA(cmsg), sizeof(credFd));
            break;
        }
    }
#endif

    /* Fail if we couldn't extract a valid fd from message */
    if (credFd == -1)
    {
        dwError = EBADF;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

    /* Stat the fd to find the uid/gid of the peer socket */
    if (fstat(credFd, &statbuf) != 0)
    {
        dwError = errno;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

    /* Get the path of our unix socket */
    if (getsockname(fd, (struct sockaddr*) &localAddr, &localAddrLen))
    {
        dwError = errno;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

    /* Get the path that the peer unix socket is connected to */
    if (getpeername(credFd, (struct sockaddr*) &credPeerAddr, &credPeerAddrLen))
    {
        dwError = errno;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

    /* Fail if these paths are not the same */
    if (strcmp(localAddr.sun_path, credPeerAddr.sun_path))
    {
        dwError = EPERM;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

    *pUid = statbuf.st_uid;
    *pGid = statbuf.st_gid;

cleanup:

    if (credFd != -1)
        close(credFd);

    return dwError;

error:
	
	*pUid = 0;
	*pGid = 0;

	goto cleanup;
}


