#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <ctype.h>

#include "config.h"
#include "lwnet-system.h"
#include "lwnet-def.h"
#include "lwnet.h"
#include "lwnet-utils.h"
#include "lwnet-netbios.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


void HexDumpBuf(UINT8 *buf, int len);


void PrintNbFlags(uint8_t NB_Flags)
{
    unsigned char OwnerNodeType = 0;
    unsigned char Group = 0;

    OwnerNodeType = NB_Flags & 0x3;
    Group = (NB_Flags & 0x4) ? 1 : 0;
    switch (OwnerNodeType)
    {
        case 0:
            printf("Node broadcast\n");
            break;
        case 1:
            printf("Node Point-to-point\n");
            break;
        case 2:
            printf("Node Mixed\n");
            break;
        default:
            printf("Node Reserved\n");
            break;
    }

    printf("Group Type: %s\n", Group ? "Group" : "Unique");
}

int main(int argc, char *argv[])
{
    int sock = -1;
    int sts = 0;
    DWORD dwError = 0;
    int respLen = 0;
    int allow_broadcast = 1;
    socklen_t NetBiosReplyAddrLen = 0;
    unsigned char NetBiosQuery[1024] = {0};
    unsigned char NetBiosReply[1024] = {0};
    char *NetBiosHost = NULL;
    struct sockaddr_in dgaddr;
    DWORD NetBiosQueryLen = 0;
    unsigned short TransactionId = 0;
    unsigned short respTransactionId = 0;
    struct in_addr *addrs = NULL;
    DWORD addrsLen = 0;
    int i = 0;
    char *NbName = NULL;
    uint32_t TTL = 0;
    uint8_t Flags = 0;
    unsigned char queryType = LWNB_RESOLVE_WORKSTATION;
    struct pollfd pollfds[1];
    int first = 1;
    int addrCnt = 1;

    memset(&pollfds, 0, sizeof(pollfds));
    if (argc == 1)
    {
        printf("usage %s NetBIOS_name [queryType]\n", argv[0]);
        return 1;
    }

    if (argc > 2)
    {
        if (!strcasecmp("workstation", argv[2]) ||
            !strcasecmp("group", argv[2]))
        {
            queryType = LWNB_RESOLVE_WORKSTATION;
        }
        else if (!strcasecmp("file", argv[2]))
        {
            queryType = LWNB_RESOLVE_FILE_SERVICE;
        }
        else if (!strcasecmp("dc", argv[2]))
        {
            queryType = LWNB_RESOLVE_DC;
        }
        else
        {
            queryType = (unsigned char) strtoul(argv[2], NULL, 0);
        }
    }

    NetBiosHost = argv[1];

    LWNetSrvNetBiosInit();

    dwError = LWNetNbConstructNameQuery(
              NetBiosHost,
              LWNB_QUERY_BROADCAST,
              queryType,
              &TransactionId,
              NetBiosQuery,
              &NetBiosQueryLen);
    BAIL_ON_LWNET_ERROR(dwError);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("socket(SOCK_DGRAM)");
        return 1;
    }

    sts = setsockopt(sock, 
                     SOL_SOCKET, 
                     SO_BROADCAST, 
                     &allow_broadcast, 
                     sizeof(allow_broadcast));
    if (sts == -1)
    {
       perror("setsockopt failed!");
       return 1;
    }
    
    memset(&dgaddr, 0, sizeof(dgaddr));
    dgaddr.sin_family = AF_INET;
    dgaddr.sin_port = htons(137);
    dgaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    sts = sendto(sock,
                 NetBiosQuery,
                 NetBiosQueryLen,
                 0,  /* flags */
                 (struct sockaddr *) &dgaddr,
                 sizeof(dgaddr));
    if (sts == -1)
    {
        perror("sendto failed!");
        return 1;
    }


    do
    {
        respLen = 0;
        pollfds[0].fd = sock;
        pollfds[0].events = POLLIN;
        sts = poll(pollfds, 1, 2000);
        if (sts > 0)
        {
            respLen = recvfrom(
                          sock,
                          NetBiosReply,
                          sizeof(NetBiosReply),
                          0,  /* flags */
                          (struct sockaddr *) &dgaddr,
                          (void *) &NetBiosReplyAddrLen);
            if (respLen > 0)
            {
//              printf("recvfrom got %d bytes\n", respLen);
//              HexDumpBuf(NetBiosReply, respLen);
//              printf("\n");
                dwError = LWNetNbParseNameQueryResponse(
                              NetBiosReply,
                              respLen,
                              TransactionId,
                              &respTransactionId,
                              &NbName,
                              &TTL,
                              &Flags,
                              &addrs,
                              &addrsLen);
                BAIL_ON_LWNET_ERROR(dwError);
                if (first)
                {
                    first = 0;
                    printf("TransID = %d NbName = %.15s<%02x>\n\n",
                           TransactionId,
                           NbName, NbName[15]);
                }
                if (addrsLen > 0)
                {
                    /* Deal with space padding of NetBIOS name */
                    for (i=0; NbName[i] && !isspace((int) NbName[i]); i++)
                        ;
                    NbName[i] = '\0';

//                    PrintNbFlags(Flags);
//                    printf("TTL=%d\n", (int) TTL);
                    for (i=0; i<addrsLen; i++)
                    {
                        printf("IpAddress[%d]=%-16s %s<%02x>\n", 
                               addrCnt++, 
                               inet_ntoa(addrs[i]), 
                               NbName, 
                               NbName[15]);
                    }
                }
                LWNetNbAddressListFree(addrs);
                LWNET_SAFE_FREE_STRING(NbName)
            }
        }
    } while (respLen > 0);
    printf("\n");
                
cleanup:
    LWNetSrvNetBiosCleanup();
    return dwError;


error:
    printf("nbquery: failed %d\n", dwError);
    goto cleanup;
}
