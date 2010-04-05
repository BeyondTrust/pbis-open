/*++
    Linux DNS client library implementation
    Copyright (C) 2006 Krishna Ganugapati

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

++*/

#include "includes.h"

DWORD
DNSOpen(
    PCSTR   pszNameServer,
    DWORD   dwType,
    PHANDLE phDNSServer
    )
{
    DWORD  dwError = 0;
    HANDLE hDNSServer = (HANDLE)NULL;

    BAIL_ON_INVALID_STRING(pszNameServer);

    switch (dwType)
    {
        case DNS_TCP:

            dwError = DNSTCPOpen(
                            pszNameServer,
                            &hDNSServer
                            );

            break;

        case DNS_UDP:

            dwError = DNSUDPOpen(
                            pszNameServer,
                            &hDNSServer
                            );

            break;

        default:

            dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LWDNS_ERROR(dwError);

    *phDNSServer = hDNSServer;

cleanup:

    return dwError;

error:

    *phDNSServer = (HANDLE)NULL;

    goto cleanup;
}

DWORD
DNSTCPOpen(
    PCSTR   pszNameServer,
    PHANDLE phDNSServer
    )
{
    DWORD dwError = 0;
    unsigned long ulAddress = 0;
    struct hostent * pHost = NULL;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;

    dwError = DNSAllocateMemory(
                    sizeof(DNS_CONNECTION_CONTEXT),
                    (PVOID *)&pDNSContext);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDNSContext->s = -1;

    pDNSContext->hType = DNS_TCP;

    ulAddress = inet_addr (pszNameServer);
    
    if (INADDR_NONE == ulAddress)
    {
         pHost = gethostbyname (pszNameServer);
         if (!pHost)
         {
            dwError = h_errno;
            BAIL_ON_HERRNO_ERROR(dwError);
         }
         memcpy((char *)&ulAddress, pHost->h_addr, pHost->h_length);
    }

    // create the socket
    //
    pDNSContext->s = socket (PF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == pDNSContext->s) {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    pDNSContext->RecvAddr.sin_family = AF_INET;
    pDNSContext->RecvAddr.sin_addr.s_addr = ulAddress;
    pDNSContext->RecvAddr.sin_port = htons (DNS_TCP_PORT);

    // connect to remote endpoint
    //
    dwError = connect(
                pDNSContext->s,
                (PSOCKADDR) &pDNSContext->RecvAddr,
                sizeof(pDNSContext->RecvAddr));
    if (dwError == SOCKET_ERROR) {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    *phDNSServer = (HANDLE)pDNSContext;

cleanup:

    return dwError;

error:

    *phDNSServer = (HANDLE)NULL;
    
    if (pDNSContext)
    {
        DNSClose((HANDLE)pDNSContext);
    }

    goto cleanup;
}

DWORD
DNSUDPOpen(
    PCSTR   pszNameServer,
    PHANDLE phDNSServer
    )
{
    DWORD dwError = 0;
    unsigned long ulAddress;
    struct hostent *pHost = NULL;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;

    dwError = DNSAllocateMemory(
                    sizeof(DNS_CONNECTION_CONTEXT),
                    (PVOID *)&pDNSContext);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDNSContext->hType = DNS_UDP;

    ulAddress = inet_addr (pszNameServer);
    
    if (INADDR_NONE == ulAddress)
    {
        pHost = gethostbyname (pszNameServer);
        if (NULL == pHost)
        {
            dwError = h_errno;
            BAIL_ON_HERRNO_ERROR(dwError);
        }
        memcpy((char*)&ulAddress, pHost->h_addr, pHost->h_length);
    }

    // Create a socket for sending data
    pDNSContext->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    //-------------------------------------------
    // Set up the RecvAddr structure with the IP address of
    // the receiver (in this example case "123.456.789.1")
    // and the specified port number.
    //
    pDNSContext->RecvAddr.sin_family = AF_INET;
    pDNSContext->RecvAddr.sin_port = htons(DNS_UDP_PORT);
    pDNSContext->RecvAddr.sin_addr.s_addr = ulAddress;

    *phDNSServer = (HANDLE)pDNSContext;
    
cleanup:

    return dwError;

error:

    *phDNSServer = (HANDLE)NULL;
    
    if (pDNSContext)
    {
        DNSClose((HANDLE)pDNSContext);
    }

    goto cleanup;
}

DWORD
DNSClose(
    HANDLE hDNSServer
    )
{
    DWORD dwError = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;

    pDNSContext = (PDNS_CONNECTION_CONTEXT)hDNSServer;

    switch(pDNSContext->hType)
    {
        case DNS_TCP:

            DNSTCPClose(hDNSServer);

            break;

        case DNS_UDP:

            DNSUDPClose(hDNSServer);

            break;
    }

    return dwError;
}

DWORD
DNSTCPClose(
    HANDLE hBindServer
    )
{
    DWORD dwError = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;
    
    pDNSContext = (PDNS_CONNECTION_CONTEXT)hBindServer;

    if (pDNSContext->s >= 0)
    {
        close(pDNSContext->s);
    }

    DNSFreeMemory(pDNSContext);

    return dwError;
}

DWORD
DNSUDPClose(
    HANDLE hBindServer
    )
{
    DWORD dwError = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;
    
    pDNSContext = (PDNS_CONNECTION_CONTEXT)hBindServer;

    if (pDNSContext->s >= 0)
    {
        close(pDNSContext->s);
    }

    DNSFreeMemory(pDNSContext);

    return dwError;
}

DWORD
DNSSendTCPRequest(
    HANDLE hDNSHandle,
    PBYTE  pDNSSendBuffer,
    DWORD  dwBufferSize,
    PDWORD pdwBytesSent
    )
{
    DWORD dwError = 0;
    DWORD dwBytesSent = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;

    pDNSContext = (PDNS_CONNECTION_CONTEXT)hDNSHandle;

    dwBytesSent = send(
                    pDNSContext->s,
                    pDNSSendBuffer,
                    dwBufferSize,
                    0);
    if (dwBytesSent == SOCKET_ERROR) {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    *pdwBytesSent = dwBytesSent;

cleanup:

    return dwError;

error:

    *pdwBytesSent = 0;
    
    goto cleanup;
}

DWORD
DNSSendUDPRequest(
    HANDLE hDNSHandle,
    PBYTE pDNSSendBuffer,
    DWORD dwBufferSize,
    PDWORD pdwBytesSent
    )
{
    DWORD dwError = 0;
    DWORD dwBytesSent = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;

    pDNSContext = (PDNS_CONNECTION_CONTEXT)hDNSHandle;

    dwBytesSent = sendto(
                    pDNSContext->s,
                    pDNSSendBuffer,
                    dwBufferSize,
                    0,
                    (SOCKADDR *)&pDNSContext->RecvAddr,
                    sizeof(pDNSContext->RecvAddr));
    if (dwBytesSent == SOCKET_ERROR)
    {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    *pdwBytesSent = dwBytesSent;

cleanup:

    return dwError;

error:

    *pdwBytesSent = 0;
    
    goto cleanup;
}

DWORD
DNSTCPReceiveBufferContext(
    HANDLE hDNSHandle,
    HANDLE hDNSRecvBuffer,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    DWORD dwRead = 0;
    WORD wBytesToRead = 0;
    WORD wnBytesToRead = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;
    PDNS_RECEIVEBUFFER_CONTEXT pDNSRecvContext = NULL;
    pDNSContext = (PDNS_CONNECTION_CONTEXT)hDNSHandle;
    pDNSRecvContext = (PDNS_RECEIVEBUFFER_CONTEXT)hDNSRecvBuffer;

    dwRead = recv(
                pDNSContext->s,
                (char *)&wnBytesToRead,
                sizeof(WORD),
                0);
    if (dwRead == SOCKET_ERROR) {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    wBytesToRead = ntohs(wnBytesToRead);

    dwRead = recv(
                pDNSContext->s,
                (char *)pDNSRecvContext->pRecvBuffer,
                wBytesToRead,
                0);
    if (dwRead == SOCKET_ERROR) {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    pDNSRecvContext->dwBytesRecvd =  dwRead;

    *pdwBytesRead = (DWORD)dwRead;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
DNSUDPReceiveBufferContext(
    HANDLE hDNSHandle,
    HANDLE hDNSRecvBuffer,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    DWORD dwRead = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;
    PDNS_RECEIVEBUFFER_CONTEXT pDNSRecvContext = NULL;
    pDNSContext = (PDNS_CONNECTION_CONTEXT)hDNSHandle;
    pDNSRecvContext = (PDNS_RECEIVEBUFFER_CONTEXT)hDNSRecvBuffer;

    dwRead = recv(
                pDNSContext->s,
                (char *)pDNSRecvContext->pRecvBuffer,
                512,
                0);
    if (dwRead == SOCKET_ERROR) {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    pDNSRecvContext->dwBytesRecvd = dwRead;

    *pdwBytesRead = (DWORD)dwRead;

cleanup:

    return dwError;
    
error:

    *pdwBytesRead = 0;
    
    goto cleanup;
}

DWORD
DNSReceiveBufferContext(
    HANDLE hDNSHandle,
    HANDLE hDNSRecvBuffer,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;
    pDNSContext = (PDNS_CONNECTION_CONTEXT)hDNSHandle;

    switch (pDNSContext->hType)
    {
        case DNS_TCP:
            
            dwError = DNSTCPReceiveBufferContext(
                        hDNSHandle, 
                        hDNSRecvBuffer, 
                        pdwBytesRead);
            
            break;
            
        case DNS_UDP:
            
            dwError = DNSUDPReceiveBufferContext(
                        hDNSHandle, 
                        hDNSRecvBuffer, 
                        pdwBytesRead);
            
            break;
    }
    
    return dwError;
}

DWORD
DNSCreateSendBuffer(
    HANDLE * phDNSSendBuffer
    )
{
    DWORD dwError = 0;
    PDNS_SENDBUFFER_CONTEXT pDNSContext = NULL;

    dwError = DNSAllocateMemory(
                sizeof(DNS_SENDBUFFER_CONTEXT),
                (PVOID *)&pDNSContext);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                SENDBUFFER_SIZE,
                (PVOID *)&pDNSContext->pSendBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSContext->dwBufferSize = SENDBUFFER_SIZE;

    //
    // We will offset into the buffer by 2 bytes
    // If we are doing a TCP write; we will fill in these
    // two bytes and send + 2 bytes
    // If we are doing a UDP write; we will start our send
    // +2 bytes and only send dwWritten;
    //
    pDNSContext->dwBufferOffset += 2;

    *phDNSSendBuffer = (HANDLE)pDNSContext;

cleanup:

    return dwError;

error:
    
    if (pDNSContext) {
        DNSFreeSendBuffer((HANDLE)pDNSContext);
    }
    
    *phDNSSendBuffer = (HANDLE)NULL;

    goto cleanup;
}

DWORD
DNSMarshallBuffer(
    HANDLE hDNSSendBuffer,
    PBYTE  pDNSSendBuffer,
    DWORD  dwBufferSize,
    PDWORD pdwBytesWritten
    )
{
    DWORD dwError = 0;
    PBYTE pTemp = NULL;
    PDNS_SENDBUFFER_CONTEXT pDNSContext = NULL;
    DWORD dwRemaining = 0;

    pDNSContext = (PDNS_SENDBUFFER_CONTEXT)hDNSSendBuffer;
    
    dwRemaining = pDNSContext->dwBufferSize - pDNSContext->dwBufferOffset;
    
    if (dwRemaining < dwBufferSize)
    {
        DWORD dwNewSize = 
            (pDNSContext->dwBufferSize + 
            (dwBufferSize - dwRemaining) + 256);
        
        dwError = DNSReallocMemory(
                    pDNSContext->pSendBuffer,
                    (PVOID*)&pDNSContext->pSendBuffer,
                    dwNewSize);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        pDNSContext->dwBufferSize = dwNewSize;
    }

    pTemp = pDNSContext->pSendBuffer + pDNSContext->dwBufferOffset;

    memcpy(pTemp, pDNSSendBuffer, dwBufferSize);

    pDNSContext->dwBytesWritten += dwBufferSize;
    pDNSContext->dwBufferOffset += dwBufferSize;

    *pdwBytesWritten = dwBufferSize;
    
cleanup:

    return dwError;
    
error:

    *pdwBytesWritten = 0;

    goto cleanup;
}

VOID
DNSFreeSendBuffer(
    HANDLE hDNSSendBuffer
    )
{
    PDNS_SENDBUFFER_CONTEXT pDNSContext = NULL;

    pDNSContext = (PDNS_SENDBUFFER_CONTEXT)hDNSSendBuffer;

    if (pDNSContext){
        DNSFreeMemory(pDNSContext->pSendBuffer);

        DNSFreeMemory(pDNSContext);
    }
    return;
}

DWORD
DNSSendBufferContext(
    HANDLE hDNSServer,
    HANDLE hSendBuffer,
    PDWORD pdwBytesSent

    )
{
    DWORD dwError = 0;
    PDNS_CONNECTION_CONTEXT pDNSContext = NULL;
    pDNSContext = (PDNS_CONNECTION_CONTEXT)hDNSServer;
    
    switch(pDNSContext->hType)
    {
        case DNS_TCP:
            
            dwError = DNSTCPSendBufferContext(
                            hDNSServer,
                            hSendBuffer,
                            pdwBytesSent);
            
            break;

        case DNS_UDP:
            
            dwError = DNSUDPSendBufferContext(
                            hDNSServer,
                            hSendBuffer,
                            pdwBytesSent);
            
            break;
    }

    return dwError;
}

DWORD
DNSTCPSendBufferContext(
    HANDLE hDNSServer,
    HANDLE hSendBuffer,
    PDWORD pdwBytesSent

    )
{
    DWORD dwError = 0;
    PDNS_SENDBUFFER_CONTEXT pSendBufferContext = NULL;
    WORD wBytesWritten = 0;
    WORD wnBytesWritten = 0;

    pSendBufferContext = (PDNS_SENDBUFFER_CONTEXT)hSendBuffer;

    wBytesWritten = (WORD)pSendBufferContext->dwBytesWritten;
    wnBytesWritten = htons(wBytesWritten);

    memcpy(pSendBufferContext->pSendBuffer, &wnBytesWritten, sizeof(WORD));

    dwError = DNSSendTCPRequest(
                hDNSServer,
                pSendBufferContext->pSendBuffer,
                pSendBufferContext->dwBytesWritten + 2,
                pdwBytesSent);

    return dwError;
}

DWORD
DNSUDPSendBufferContext(
    HANDLE hDNSServer,
    HANDLE hSendBuffer,
    PDWORD pdwBytesSent
    )
{
    DWORD dwError = 0;
    PDNS_SENDBUFFER_CONTEXT pSendBufferContext = NULL;

    pSendBufferContext = (PDNS_SENDBUFFER_CONTEXT)hSendBuffer;

    //
    // Now remember to send 2 bytes ahead of pSendBuffer; because
    // we ignore the 2 bytes size field.
    //
    dwError = DNSSendUDPRequest(
                hDNSServer,
                pSendBufferContext->pSendBuffer + 2,
                pSendBufferContext->dwBytesWritten,
                pdwBytesSent);

    return dwError;
}

DWORD
DNSDumpSendBufferContext(
    HANDLE hSendBuffer
    )
{
    DWORD dwError = 0;
#if 0
    PDNS_SENDBUFFER_CONTEXT pSendBufferContext = NULL;
    DWORD dwCurLine = 0;
    DWORD i = 0;

    pSendBufferContext = (PDNS_SENDBUFFER_CONTEXT)hSendBuffer;
    printf("\n");
    printf("Buffer Size is: %d\n", pSendBufferContext->dwBytesWritten);
    while (i < pSendBufferContext->dwBytesWritten)
    {
        if ((i/16) > dwCurLine){
            printf("\n");
            dwCurLine++;
        }
        if ((i%8) == 0){
            printf("  ");
        }
        printf("%.2x ", pSendBufferContext->pSendBuffer[i]);
        i++;
    }
#endif
    return dwError;
}

DWORD
DNSDumpRecvBufferContext(
    HANDLE hRecvBuffer
    )
{
    DWORD dwError = 0;
#if 0
    PDNS_RECEIVEBUFFER_CONTEXT pRecvBufferContext = NULL;
    DWORD dwCurLine = 0;
    DWORD i = 0;

    pRecvBufferContext = (PDNS_RECEIVEBUFFER_CONTEXT)hRecvBuffer;

    printf("\n");
    printf("Buffer Size is: %d\n", pRecvBufferContext->dwBytesRecvd);

    while (i < pRecvBufferContext->dwBytesRecvd)
    {
        if ((i/16) > dwCurLine){
            printf("\n");
            dwCurLine++;
        }
        if ((i%8) == 0){
            printf("  ");
        }
        printf("%.2x ", pRecvBufferContext->pRecvBuffer[i]);
        i++;
    }
#endif
    return dwError;
}

DWORD
DNSCreateReceiveBuffer(
    HANDLE * phDNSRecvBuffer
    )
{
    DWORD dwError = 0;
    PDNS_RECEIVEBUFFER_CONTEXT pDNSContext = NULL;

    dwError = DNSAllocateMemory(
                sizeof(DNS_RECEIVEBUFFER_CONTEXT),
                (PVOID *)&pDNSContext);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                RECVBUFFER_SIZE,
                (PVOID *)&pDNSContext->pRecvBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSContext->dwBufferSize = RECVBUFFER_SIZE;

    *phDNSRecvBuffer = (HANDLE)pDNSContext;

cleanup:

    return dwError;

error:

    if (pDNSContext) {
        DNSFreeReceiveBufferContext((HANDLE)pDNSContext);
    }
    
    *phDNSRecvBuffer = (HANDLE)NULL;

    goto cleanup;
}


DWORD
DNSUnmarshallBuffer(
    HANDLE hDNSRecvBuffer,
    PBYTE  pDNSRecvBuffer,
    DWORD  dwBufferSize,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    PBYTE pTemp = NULL;
    PDNS_RECEIVEBUFFER_CONTEXT pDNSContext = NULL;
    DWORD dwRemaining = 0;

    pDNSContext = (PDNS_RECEIVEBUFFER_CONTEXT)hDNSRecvBuffer;
    
    dwRemaining = pDNSContext->dwBufferSize - pDNSContext->dwBytesRead;
    
    if (dwRemaining < dwBufferSize)
    {
        DWORD dwNewSize = 
            (pDNSContext->dwBufferSize + 
             (dwBufferSize - dwRemaining) + 256);
        
        dwError = DNSReallocMemory(
                    pDNSContext->pRecvBuffer,
                    (PVOID*)&pDNSContext->pRecvBuffer,
                    dwNewSize);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        pDNSContext->dwBufferSize = dwNewSize;
    }

    pTemp = pDNSContext->pRecvBuffer + pDNSContext->dwBytesRead;

    memcpy(pDNSRecvBuffer, pTemp, dwBufferSize);

    pDNSContext->dwBytesRead += dwBufferSize;

    *pdwBytesRead = dwBufferSize;
    
cleanup:

    return dwError;
    
error:

    *pdwBytesRead = 0;
    
    goto cleanup;
}

DWORD
DNSUnmarshallDomainNameAtOffset(
    HANDLE hRecvBuffer,
    WORD wOffset,
    PDNS_DOMAIN_NAME * ppDomainName
    )
{
    DWORD dwError = 0;
    PDNS_DOMAIN_LABEL pLabel = NULL;
    PDNS_DOMAIN_LABEL pLabelList = NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;
    PSTR  pszLabel = NULL;
    BYTE  uLen = 0;
    DWORD dwCurrent = 0;
    PDNS_RECEIVEBUFFER_CONTEXT pRecvContext = NULL;

    pRecvContext = (PDNS_RECEIVEBUFFER_CONTEXT)hRecvBuffer;
    dwCurrent = wOffset;

    while(1)
    {
        CHAR szLabel[65];
        
        memcpy(&uLen,
               pRecvContext->pRecvBuffer+dwCurrent,
               sizeof(char));
        
        dwCurrent++;

        if (uLen == 0) {
            break;
        }

        memset(szLabel, 0, sizeof(szLabel));
        memcpy(szLabel, pRecvContext->pRecvBuffer+dwCurrent, uLen);
        dwCurrent += uLen;

        dwError = DNSAllocateString(
                    szLabel, 
                    &pszLabel);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSAllocateMemory(
                    sizeof(DNS_DOMAIN_LABEL),
                    (PVOID *)&pLabel);
        BAIL_ON_LWDNS_ERROR(dwError);

        pLabel->pszLabel = pszLabel;
        pszLabel = NULL;
        
        dwError = DNSAppendLabel(
                        pLabelList,
                        pLabel,
                        &pLabelList);
        BAIL_ON_LWDNS_ERROR(dwError);
        
        pLabel = NULL;
    }

    dwError = DNSAllocateMemory(
                sizeof(DNS_DOMAIN_NAME),
                (PVOID *)&pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    pDomainName->pLabelList = pLabelList;
    pLabelList = NULL;

    *ppDomainName = pDomainName;
    
cleanup:

    return dwError;

error:

    *ppDomainName = NULL;
    
    LWDNS_SAFE_FREE_STRING(pszLabel);
    
    if (pLabel)
    {
        DNSFreeLabel(pLabel);
    }
    
    if (pLabelList)
    {
        DNSFreeLabelList(pLabelList);
    }
    
    if (pDomainName)
    {
        DNSFreeDomainName(pDomainName);
    }
    
    goto cleanup;
}

DWORD
DNSReceiveBufferMoveBackIndex(
    HANDLE hRecvBuffer,
    WORD wOffset
    )
{
    DWORD dwError = 0;
    PDNS_RECEIVEBUFFER_CONTEXT pDNSContext = NULL;

    pDNSContext = (PDNS_RECEIVEBUFFER_CONTEXT)hRecvBuffer;
    pDNSContext->dwBytesRead -= wOffset;

    return dwError;
}

VOID
DNSFreeSendBufferContext(
    HANDLE hSendBuffer
    )
{
    PDNS_SENDBUFFER_CONTEXT pSendBufferContext = NULL;

    pSendBufferContext = (PDNS_SENDBUFFER_CONTEXT)hSendBuffer;

    if (pSendBufferContext->pSendBuffer) {
        DNSFreeMemory(pSendBufferContext->pSendBuffer);
    }
    if (pSendBufferContext) {
        DNSFreeMemory(pSendBufferContext);
    }
}

DWORD
DNSGetSendBufferContextSize(
    HANDLE hSendBuffer
    )
{
    PDNS_SENDBUFFER_CONTEXT pSendBufferContext = NULL;

    pSendBufferContext = (PDNS_SENDBUFFER_CONTEXT)hSendBuffer;

    return (pSendBufferContext ? (pSendBufferContext->dwBytesWritten) : 0);

}

PBYTE
DNSGetSendBufferContextBuffer(
    HANDLE hSendBuffer
    )
{
    PDNS_SENDBUFFER_CONTEXT pSendBufferContext = NULL;

    pSendBufferContext = (PDNS_SENDBUFFER_CONTEXT)hSendBuffer;

    return (pSendBufferContext ? (pSendBufferContext->pSendBuffer + 2) : NULL);
}

VOID
DNSFreeReceiveBufferContext(
    HANDLE hRecvBuffer
    )
{
    PDNS_RECEIVEBUFFER_CONTEXT pRecvBufferContext = NULL;

    pRecvBufferContext = (PDNS_RECEIVEBUFFER_CONTEXT)hRecvBuffer;

    if (pRecvBufferContext->pRecvBuffer) {
        DNSFreeMemory(pRecvBufferContext->pRecvBuffer);
    }
    if (pRecvBufferContext) {
        DNSFreeMemory(pRecvBufferContext);
    }
}

