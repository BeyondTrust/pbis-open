#include "includes.h"

typedef struct _LWNET_SRV_NETBIOS_CONTEXT
{
    BOOLEAN bNbRepsponse;
    BOOLEAN bAck;
    BOOLEAN bShutdown;
    UINT16 transactionId;
    UINT16 nextTransactionId;
    pthread_cond_t cv;
    pthread_mutex_t mutex;
    pthread_cond_t cvAck;
    pthread_mutex_t mutexAck;
    pthread_mutex_t mutexTransactionId;
    int sock;
    struct in_addr *addrs;
    DWORD respError;
    DWORD addrsLen;
    DWORD udpTimeout;
} LWNET_SRV_NETBIOS_CONTEXT, *PLWNET_SRV_NETBIOS_CONTEXT;

static PLWNET_SRV_NETBIOS_CONTEXT gpNbCtx;

static UINT8 NetBiosQueryFooter[] = 
{
    0x00, 0x20,
    0x00, 0x01,
};


static DWORD
LWNetNbConstructNameQueryHeader(
    UINT16 TransactionId,
    UINT8 bBroadcast,  /* 1=Broadcast; 0=WINS */
    UINT8 opcode,      /* 6 values defined    */
    INT16 QDCount,
    DWORD retNetBiosHeaderLen,
    UINT8 *retNetBiosHeader)
{
    DWORD i = 0;
    DWORD dwError = 0;
    UINT16 OpcodeFlags = 0;

/*
From RFC 1002:
4.2.12. NAME QUERY REQUEST

    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         NAME_TRN_ID           |0|  0x0  |0|0|1|0|0 0|B|  0x0  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          0x0001               |           0x0000              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          0x0000               |           0x0000              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   /                         QUESTION_NAME                         /
   /                                                               /
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           NB (0x0020)         |        IN (0x0001)            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Apparently all values are in Network Byte order, including bytes 3/4
(opcode/flags...).
*/

    if (retNetBiosHeaderLen < LWNB_NAME_QUERY_HEADER_SIZE)
    {
        dwError = ERROR_BUFFER_OVERFLOW;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* Construct header up to "QUESTION NAME" */
    TransactionId = htons(TransactionId);
    memcpy(&retNetBiosHeader[i], &TransactionId, sizeof(TransactionId));
    i += sizeof(TransactionId);

    OpcodeFlags |= (bBroadcast & 1) << LWNB_SHIFT_FLAG_BROADCAST;
    OpcodeFlags |= 1                << LWNB_SHIFT_FLAG_RECURSION_DESIRED;
    OpcodeFlags |= (opcode & 4)     << LWNB_SHIFT_OPCODE;

    OpcodeFlags = htons(OpcodeFlags);
    memcpy(&retNetBiosHeader[i], &OpcodeFlags, sizeof(OpcodeFlags));
    i += sizeof(OpcodeFlags);

    QDCount = htons(QDCount);
    memcpy(&retNetBiosHeader[i], &QDCount, sizeof(QDCount));
    i += sizeof(QDCount);

cleanup:
    return dwError;

error:
    goto cleanup;
}


/* Private "internal" functions; exposed for unit tests */
void 
LWNetNbHexDumpBuf(
    UINT8 *buf, 
    int len)
{
    int i = 0;
    for (i=0; i<len; i++)
    {
        if (i && (i%8) == 0)
        {
            printf("\n");
        }
        printf("%02x ", buf[i]);
    }
    printf("\n");
}


/* "First Level" NetBIOS name encoding */
void
LWNetNbStrToNbName(
    PSTR pszHost, 
    UINT8 suffix,
    PBYTE nbBuf)
{
    UINT8 nibble1;
    UINT8 nibble2;
    CHAR c;
    DWORD i = 0;
    DWORD count = 0;

    while (*pszHost && count < 15)
    {
        /*
         * There may be some contraversy as to who should be case insensitive,
         * the client or server. Apparently requests sent in upper case are
         * universally accepted.
         */
        c = toupper((int) *pszHost);
        nibble1 = (0xf0 & c) >> 4;
        nibble2 = (0x0f & c);
        nbBuf[i]   = nibble1 + 'A';
        nbBuf[i+1] = nibble2 + 'A';
        pszHost++;
        i += 2;
        count++;
    }

    /* Pad the rest of the NetBIOS name with encoded space sequences */
    while (count < 15)
    {
        nbBuf[i]   = 'C';
        nbBuf[i+1] = 'A';
        i += 2;
        count++;
    }
    c = suffix;
    nibble1 = (0xf0 & c) >> 4;
    nibble2 = (0x0f & c);
    nbBuf[i]   = nibble1 + 'A';
    nbBuf[i+1] = nibble2 + 'A';
    i += 2;
    nbBuf[i] = '\0';
}


/* "First Level" NetBIOS name decoding */
void 
LWNetNbNameToStr(
    PBYTE nbBuf, 
    PSTR pszHost,
    PUINT8 pQueryType)
{
    UINT8 nibble1;
    UINT8 nibble2;
    DWORD i = 0;

    while (nbBuf[i] && i<LWNB_NETBIOS_ENCNAME_LEN)
    {
        nibble1 = nbBuf[i]   - 'A';
        nibble2 = nbBuf[i+1] - 'A';
        *pszHost = (nibble1 << 4) | nibble2;
        if (*pszHost == ' ' && i<LWNB_NETBIOS_ENCNAME_LEN-2)
        {
            /*
             * NULL terminate at space, as this is illegal in NetBIOS 
             * names. However, preserve the last value, which is the
             * query type, which could happen to be 0x20 (' ').
             */
            *pszHost = '\0';
        }
        pszHost++;
        i += 2;
    }
    if (pQueryType)
    {
        *pQueryType = (UINT8) pszHost[-1];
    }
    pszHost[-1] = '\0';
}
    

/* "Second Level" NetBIOS name encoding */
DWORD
LWNetNbStrToNbName2(
    PSTR Fqdn,
    UINT8 suffix,
    PBYTE *retNbNameL2,
    PDWORD retNbNameL2Len)
{
    DWORD dwError = 0;
    DWORD len = 0;
    PSTR p = NULL;
    PSTR token = NULL;
    PSTR tokenPtr = NULL;
    UINT8 *retName = NULL;
    UINT8 *up = NULL;

    /* 
     * 32=Max NetBIOS First Level encoding length 
     * All . separators are replaced by 1 byte count, so
     * don't need to count number of dots to determine buffer length
     * 2=FirstLevel encoding length + ending length (by definition 0)
     */
    len = LWNB_NETBIOS_ENCNAME_LEN + strlen(Fqdn) + 2;
    dwError = LWNetAllocateMemory(sizeof(CHAR) * len,
                                  (PVOID*)&retName);
    BAIL_ON_LWNET_ERROR(dwError);

    up = retName;
    
    dwError = LWNetAllocateString(
                  Fqdn,
                  &token);
    BAIL_ON_LWNET_ERROR(dwError);

    p = strchr(token, '.');
    if (p)
    {
        *p++ = '\0';
        Fqdn = token;
    }

    /* Store length of NetBIOS name component before that name */
    *up++ = (UINT8) LWNB_NETBIOS_ENCNAME_LEN;
    LWNetNbStrToNbName(Fqdn, suffix, up);
    up += LWNB_NETBIOS_ENCNAME_LEN;
    
    /* Work through rest of Fqdn, replacing . with length of stuff before . */
    if (p)
    {
        p = strtok_r(p, ".", &tokenPtr);
    }
    while (p)
    {
        len = strlen(p);
        *up++ = (UINT8) len;
        strncat((char *) up, p, sizeof(CHAR)*len - (up-retName));
        up += len;
        p = strtok_r(NULL, ".", &tokenPtr);
    }
    *up++ = '\0';

    *retNbNameL2 = (UINT8 *) retName;
    *retNbNameL2Len = up - retName;

cleanup:
    LWNET_SAFE_FREE_MEMORY(token);
    return dwError;

error:
    LWNET_SAFE_FREE_MEMORY(retName);
    goto cleanup;
}


/* Decompose a NetBIOS Second Layer name into an array of name components */
DWORD
LWNetNbName2ToParts(
    PBYTE NbNameL2,
    PSTR **retNbNameParts,
    PDWORD retNbNamePartsLen)
{
    DWORD dwError = 0;
    DWORD numParts = 0;
    DWORD len = 0;
    DWORD i = 0;
    UINT8 *ptr = NULL;
    PSTR *NBNameParts = NULL;

    /* Count number of parts in NetBIOS name */
    ptr = NbNameL2;
    while (*ptr)
    {
        len = (int) *ptr++;
        ptr += len;
        numParts++;
    }
    len++;
    
    dwError = LWNetAllocateMemory((numParts+1) * sizeof(*NBNameParts),
                                  (PVOID*)&NBNameParts);
    BAIL_ON_LWNET_ERROR(dwError);

    /* Reset and allocate strings for the various parts */
    ptr = NbNameL2;
    i = 0;
    while (*ptr)
    {
        len = (int) *ptr++;
        
        dwError = LWNetAllocateMemory((len + 1) * sizeof(char),
                      (PVOID*)&NBNameParts[i]);
        BAIL_ON_LWNET_ERROR(dwError);

        strncat(NBNameParts[i++], (char *) ptr, len);
        ptr += len;
    }
    ptr++;
    *retNbNameParts = NBNameParts;
    *retNbNamePartsLen = ptr - NbNameL2;

cleanup:
    return dwError;

error:
    if (NBNameParts)
    {
        for (i=0; NBNameParts[i]; i++)
        {
            LWNET_SAFE_FREE_MEMORY(NBNameParts[i]);
        }
        LWNET_SAFE_FREE_MEMORY(NBNameParts);
    }
    goto cleanup;
}

static DWORD
LWNetNbResolveNameUdp(
    PSTR pszHostName,
    PSTR winsServer,
    UINT8 queryType,
    DWORD commType,
    OUT struct in_addr **retAddrs,
    OUT PDWORD retAddrsLen)
{
    DWORD dwError = 0;
    UINT16 transactionId = 0;
    UINT8 *NetBiosQuery = NULL;
    DWORD NetBiosQueryLen = 0;
    struct sockaddr_in dgAddr;
    struct timespec cvTimeout = {0};
    struct timeval  tp = {0};
    struct in_addr *resAddrs = NULL;
    struct in_addr *tmpResAddrs = NULL;
    int sts = 0;
    DWORD i = 0;
    DWORD resAddrsLen = 0;
    DWORD resAddrsAllocLen = 128;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bHaveQuery = FALSE;

    dwError = LWNetAllocateMemory(
                  LWNB_NETBIOS_UDP_MAX,
                  (PVOID*) &NetBiosQuery);
    BAIL_ON_LWNET_ERROR(dwError);

    memset(&dgAddr, 0, sizeof(dgAddr));
    dgAddr.sin_family = AF_INET;
    dgAddr.sin_port = htons(137);

                      
    if (commType == LWNB_QUERY_WINS && winsServer && *winsServer)
    {
        sts = inet_aton(winsServer, &dgAddr.sin_addr);
        if (sts == -1)
        {
            dwError = LwErrnoToWin32Error(ERROR_INCORRECT_ADDRESS);
            BAIL_ON_LWNET_ERROR(dwError);
        }
   
        dwError = LWNetNbConstructNameQuery(
                      pszHostName,
                      commType,
                      queryType,
                      &transactionId,
                      NetBiosQuery,
                      &NetBiosQueryLen);
        BAIL_ON_LWNET_ERROR(dwError);
        bHaveQuery = TRUE;
    }
    else if (commType == LWNB_QUERY_BROADCAST)
    {
        dgAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        commType = LWNB_QUERY_BROADCAST;
        dwError = LWNetNbConstructNameQuery(
                      pszHostName,
                      commType,
                      queryType,
                      &transactionId,
                      NetBiosQuery,
                      &NetBiosQueryLen);
        BAIL_ON_LWNET_ERROR(dwError);
        bHaveQuery = TRUE;
    }

    if (!bHaveQuery)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    // If this is not enough memory, realloc below fixes that
    dwError = LWNetAllocateMemory(
                  resAddrsAllocLen * sizeof(struct in_addr),
                  (PVOID*) &resAddrs);
    BAIL_ON_LWNET_ERROR(dwError);

    pthread_mutex_lock(&gpNbCtx->mutex);
    bLocked = TRUE;
    sts = sendto(gpNbCtx->sock,
                 NetBiosQuery,
                 NetBiosQueryLen,
                 0,  /* flags */
                 (struct sockaddr *) &dgAddr,
                 sizeof(dgAddr));
    if (sts == -1)
    {
        dwError = ERROR_NET_WRITE_FAULT;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    do
    {
        gettimeofday(&tp, NULL);
        cvTimeout.tv_sec = tp.tv_sec + gpNbCtx->udpTimeout;
        cvTimeout.tv_nsec = tp.tv_usec * 1000;
        do {
            sts = pthread_cond_timedwait(
                      &gpNbCtx->cv,
                      &gpNbCtx->mutex,
                      &cvTimeout);
        } while (!gpNbCtx->bNbRepsponse && sts != ETIMEDOUT);

        if (sts == 0 && gpNbCtx->transactionId == transactionId)
        {
            gpNbCtx->bNbRepsponse = FALSE;


            if ((commType == LWNB_QUERY_WINS && gpNbCtx->respError == 0) || 
                (commType == LWNB_QUERY_BROADCAST))
            {
                if ((gpNbCtx->addrsLen + resAddrsLen) > resAddrsAllocLen)
                {
                    resAddrsAllocLen = resAddrsAllocLen * 2 + gpNbCtx->addrsLen;
                    tmpResAddrs = LwRtlMemoryRealloc(
                                      resAddrs,
                                      resAddrsAllocLen);
                    if (!tmpResAddrs)
                    {
                        dwError = ERROR_NOT_ENOUGH_MEMORY;
                        BAIL_ON_LWNET_ERROR(dwError);
                    }
                    resAddrs = tmpResAddrs;
                    tmpResAddrs = NULL;
                }
                for (i=0; i<gpNbCtx->addrsLen; i++)
                {
                    memcpy(&resAddrs[resAddrsLen++],
                           &gpNbCtx->addrs[i], 
                           sizeof(struct in_addr));
                }
            }
            pthread_mutex_lock(&gpNbCtx->mutexAck);
            gpNbCtx->bAck = TRUE;
            pthread_mutex_unlock(&gpNbCtx->mutexAck);
            pthread_cond_signal(&gpNbCtx->cvAck);

            if (commType == LWNB_QUERY_WINS && gpNbCtx->respError)
            {
                // Bail when WINS query fails to resolve the hostname
                dwError = gpNbCtx->respError;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
    } while (commType == LWNB_QUERY_BROADCAST && sts != ETIMEDOUT);
    bLocked = FALSE;
    pthread_mutex_unlock(&gpNbCtx->mutex);

    if (resAddrsLen == 0)
    {
        dwError = ERROR_BAD_NET_NAME;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    *retAddrs = resAddrs;
    *retAddrsLen = resAddrsLen;

cleanup:
    if (bLocked)
    {
        pthread_mutex_unlock(&gpNbCtx->mutex);
    }
    LWNET_SAFE_FREE_MEMORY(NetBiosQuery);
    return dwError;

error:
    LWNET_SAFE_FREE_MEMORY(resAddrs);
    goto cleanup;
}


/* public API */

DWORD
LWNetNbResolveName(
    IN PSTR pszHostName,
    IN UINT16 flags,
    OUT struct in_addr **retAddrs,
    OUT PDWORD retAddrsLen)
{
    DWORD dwError = 0;
    /* Derive this value from flags; hard code to FILE_SERVICE for now */
    unsigned char queryType = LWNB_RESOLVE_FILE_SERVICE;
    PSTR winsPrimary = NULL;
    PSTR winsSecondary = NULL;

    if (!gpNbCtx)
    {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (flags)
    {
        if (flags & LWNB_NETBIOS_FLAGS_RESOLVE_FILE_SERVICE)
        {
            queryType = LWNB_RESOLVE_FILE_SERVICE;
        }
        else if (flags & LWNB_NETBIOS_FLAGS_RESOLVE_WORKSTATION)
        {
            queryType = LWNB_RESOLVE_WORKSTATION;
        }
        else if (flags & LWNB_NETBIOS_FLAGS_RESOLVE_DC)
        {
            queryType = LWNB_RESOLVE_DC;
        }
    }

    if (flags & LWNB_NETBIOS_FLAGS_MODE_WINS)
    {
        LwNetConfigGetWinsServers(&winsPrimary, &winsSecondary);
        dwError = LWNetNbResolveNameUdp(
                      pszHostName,
                      winsPrimary,
                      queryType,
                      LWNB_QUERY_WINS,
                      retAddrs,
                      retAddrsLen);
        if (dwError)
        {
            dwError = LWNetNbResolveNameUdp(
                          pszHostName,
                          winsSecondary,
                          queryType,
                          LWNB_QUERY_WINS,
                          retAddrs,
                          retAddrsLen);
        }
    }
    else if (flags & LWNB_NETBIOS_FLAGS_MODE_BROADCAST)
    {
        dwError = LWNetNbResolveNameUdp(
                      pszHostName,
                      NULL,
                      queryType,
                      LWNB_QUERY_BROADCAST,
                      retAddrs,
                      retAddrsLen);
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


UINT16
LWNetNbNextTransactionId(
    PLWNET_SRV_NETBIOS_CONTEXT pNbCtx)
{
    UINT16 retId = 0;

    pthread_mutex_lock(&pNbCtx->mutexTransactionId);
    retId = pNbCtx->nextTransactionId++;
    pthread_mutex_unlock(&pNbCtx->mutexTransactionId);

    return retId;
}


DWORD
LWNetNbConstructNameQuery(
    IN PSTR pszNetBiosHost,
    IN UINT8 bBroadcast,
    IN UINT8 queryType,
    OUT UINT16 *RetTransactionId,
    OUT UINT8 *NetBiosQuery,
    OUT PDWORD NetBiosQueryLen)
{
    DWORD dwError = 0;
    DWORD len = 0;
    DWORD NbNameLevel2Len = 0;
    UINT8 *NbNameLevel2 = NULL;
    UINT8 BuiltNBHeader[LWNB_NAME_MAX_LENGTH] = {0};
    UINT16 TransactionId = 0;

    dwError = LWNetNbStrToNbName2(
                  pszNetBiosHost,
                  queryType,
                  &NbNameLevel2,
                  &NbNameLevel2Len);
    BAIL_ON_LWNET_ERROR(dwError);

    TransactionId = LWNetNbNextTransactionId(gpNbCtx);
    dwError = LWNetNbConstructNameQueryHeader(
              TransactionId,
              bBroadcast ? LWNB_QUERY_BROADCAST : LWNB_QUERY_WINS,
              LWNB_OPCODE_QUERY,
              1,           // QDCOUNT: Number of names being looked up
              sizeof(BuiltNBHeader),
              BuiltNBHeader);
    BAIL_ON_LWNET_ERROR(dwError);

    /* Assemble the complete marshaled NetBIOS query packet */
    memcpy(&NetBiosQuery[len], BuiltNBHeader, LWNB_NAME_QUERY_HEADER_SIZE);
    len += LWNB_NAME_QUERY_HEADER_SIZE;
    memcpy(&NetBiosQuery[len], NbNameLevel2, NbNameLevel2Len);
    len += NbNameLevel2Len;
    memcpy(&NetBiosQuery[len], NetBiosQueryFooter, sizeof(NetBiosQueryFooter));
    len += sizeof(NetBiosQueryFooter);

    *NetBiosQueryLen = len;
    *RetTransactionId = TransactionId;

cleanup:
    LWNET_SAFE_FREE_MEMORY(NbNameLevel2);
    return dwError;

error:
    goto cleanup;
}


/* Convert NetBIOS level 2 encoded name to a string */
DWORD
LWNetNbName2ToStr(
    IN PBYTE buf,
    OUT PSTR *ppNbName,
    OUT PUINT8 pSuffix,
    OUT PDWORD dwBytesConsumed)
{
    DWORD dwError = 0;
    PSTR *NbNameParts2 = NULL;
    PSTR NbName = NULL;
    CHAR netBiosName[LWNB_NAME_MAX_LENGTH] = {0};
    DWORD addrsLen = 0;
    DWORD nbNameOffset = 0;
    DWORD i = 0;

    dwError = LWNetNbName2ToParts(&buf[i], &NbNameParts2, &addrsLen);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateMemory(
                  addrsLen * sizeof(char),
                  (PVOID*) &NbName);
    BAIL_ON_LWNET_ERROR(dwError);

    nbNameOffset = 0;
    for (i=0; NbNameParts2[i]; i++)
    {
        if (i==0)
        {
            /* 
             * Convert first part of name, which is in NetBIOS layer 1 encoding
             * to a null-terminated string.
             */
            LWNetNbNameToStr((UINT8 *) NbNameParts2[i], netBiosName, pSuffix);
            strncat(NbName, netBiosName, addrsLen - nbNameOffset);
            nbNameOffset += strlen(netBiosName);
        }
        else
        {
            /* The rest of the name is just text, so copy it */
            strncat(NbName, ".", addrsLen - nbNameOffset);
            nbNameOffset++;

            strncat(NbName,
                    NbNameParts2[i],
                    addrsLen - nbNameOffset);
            nbNameOffset += strlen(NbNameParts2[i]);
        }     
    }
    *ppNbName = NbName;
    *dwBytesConsumed = addrsLen;

cleanup:
    for (i=0; NbNameParts2[i]; i++)
    {
        LWNET_SAFE_FREE_MEMORY(NbNameParts2[i]);
    }
    LWNET_SAFE_FREE_MEMORY(NbNameParts2);
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(NbName);
    goto cleanup;
}


DWORD 
LWNetNbParseNameQueryResponse(
    IN PBYTE buf,
    IN DWORD len,
    IN UINT16 TransactionId,
    OUT UINT16 *retTransactionId,
    OUT PSTR *retNbName,
    OUT UINT32 *retTTL,
    OUT UINT8 *retFlags,
    OUT struct in_addr **retAddrs,
    OUT PDWORD retAddrsLen)
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD j = 0;
    UINT16 parseTransactionId = 0;
    UINT16 opCodeFlags = 0;
    UINT16 bResponse = 0;
    UINT16 nbQDCount = 0;
    UINT16 nbANCount = 0;
    UINT16 nbNSCount = 0;
    UINT16 nbARCount = 0;
    UINT16 nbNB = 0;
    UINT16 nbIN = 0;
    UINT32 nbTTL = 0;
    UINT16 nbNB_Flags = 0;
    UINT32 IpAddress = 0;
    UINT16 RDLength = 0;
    UINT8 queryType = 0;
    PSTR NetBiosName = NULL;
    PSTR NbName = NULL;
    DWORD numAddrs = 0;
    DWORD nbNameLen = 0;
    struct in_addr *addrs = NULL;

    memcpy(&parseTransactionId, &buf[i], sizeof(parseTransactionId));
    i += sizeof(parseTransactionId);

    parseTransactionId = ntohs(parseTransactionId);
    if (retTransactionId)
    {
        *retTransactionId = parseTransactionId;
    }
    
    memcpy(&opCodeFlags, &buf[i], sizeof(opCodeFlags));
    i += sizeof(opCodeFlags);

    opCodeFlags = ntohs(opCodeFlags);
    bResponse = opCodeFlags & (1 << LWNB_SHIFT_REQUEST_TYPE) ? 1 : 0;

#if 0
    /* Deal with other flags later... Not sure it matters at this point */

    opCodeFlags |= (bBroadcast & 1) << LWNB_SHIFT_FLAG_BROADCAST;
    opCodeFlags |= 1                << LWNB_SHIFT_FLAG_RECURSION_DESIRED;
    opCodeFlags |= (opcode & 4)     << LWNB_SHIFT_OPCODE;
#endif

    if (bResponse != LWNB_OPCODE_RESPONSE)
    {
        /* Something is seriously wrong if this is not a NetBIOS response */
        dwError = ERROR_BAD_NET_RESP;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    memcpy(&nbQDCount, &buf[i], sizeof(nbQDCount));
    i += sizeof(nbQDCount);
    nbQDCount = ntohs(nbQDCount);

    memcpy(&nbANCount, &buf[i], sizeof(nbANCount));
    i += sizeof(nbANCount);
    nbANCount = ntohs(nbANCount);
    if (nbANCount != 1)
    {
        /* There must be exactly 1 answer to the sent query */
        dwError = ERROR_BAD_NET_RESP;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    memcpy(&nbNSCount, &buf[i], sizeof(nbNSCount));
    i += sizeof(nbNSCount);
    nbNSCount = ntohs(nbNSCount);

    memcpy(&nbARCount, &buf[i], sizeof(nbARCount));
    i += sizeof(nbARCount);
    nbARCount = ntohs(nbARCount);

    dwError = LWNetNbName2ToStr(
                 &buf[i],
                 &NetBiosName,
                 &queryType,
                 &nbNameLen);
    BAIL_ON_LWNET_ERROR(dwError);
    i += nbNameLen;

    memcpy(&nbNB, &buf[i], sizeof(nbNB));
    i += sizeof(nbNB);
    nbNB = ntohs(nbNB);
    
    memcpy(&nbIN, &buf[i], sizeof(nbIN));
    i += sizeof(nbIN);
    nbIN = ntohs(nbIN);
    
    if (nbNB != 0x0020 || nbIN != 0x0001)
    {
        /* RFC 1002 says these must be the values for these fields */
        dwError = ERROR_BAD_NET_RESP;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    memcpy(&nbTTL, &buf[i], sizeof(nbTTL));
    i += sizeof(nbTTL);
    nbTTL = ntohl(nbTTL);
    if (retTTL)
    {
        *retTTL = nbTTL;
    }

    memcpy(&RDLength, &buf[i], sizeof(RDLength));
    i += sizeof(RDLength);
    RDLength = ntohs(RDLength);
    numAddrs = RDLength / (sizeof(nbNB_Flags) + sizeof(IpAddress)); 
    dwError = LWNetAllocateMemory(
                  numAddrs * sizeof(struct in_addr),
                  (PVOID*) &addrs);
    BAIL_ON_LWNET_ERROR(dwError);

    j = 0;
    do
    {
        memcpy(&nbNB_Flags, &buf[i], sizeof(nbNB_Flags));
        i += sizeof(nbNB_Flags);
        nbNB_Flags = ntohs(nbNB_Flags);
        nbNB_Flags >>= 13;
        *retFlags = nbNB_Flags;
    
        memcpy(&IpAddress, &buf[i], sizeof(IpAddress));
        i += sizeof(IpAddress);
        addrs[j++].s_addr = IpAddress;
        RDLength -= sizeof(nbNB_Flags) + sizeof(IpAddress);
    }
    while (RDLength > 0);
    *retNbName = NetBiosName;
    *retAddrs = addrs;
    *retAddrsLen = j;

cleanup:
    return dwError;

error:
    LWNET_SAFE_FREE_MEMORY(NetBiosName);
    LWNET_SAFE_FREE_MEMORY(NbName);
    LWNET_SAFE_FREE_MEMORY(addrs);
    goto cleanup;
}

VOID 
LWNetNbAddressListFree(
    IN struct in_addr *retAddrs)
{
    LWNET_SAFE_FREE_MEMORY(retAddrs);
}



VOID *LWNetSrvStartNetBiosThreadRoutine(VOID *ctx)
{
    DWORD dwError = 0;
    struct pollfd pollfds[1];
    int sts = 0;
    int sock = 0;
    UINT8 *NetBiosReply = NULL;
    struct sockaddr_in dgAddr;
    int NetBiosReplyAddrLen = 0;
    void *pNetBiosReplyAddrLen = &NetBiosReplyAddrLen;
    PLWNET_SRV_NETBIOS_CONTEXT pNbCtx = (PLWNET_SRV_NETBIOS_CONTEXT) ctx;
    int allowBroadcast = 1;
    UINT8 Flags = 0;
    UINT16 respTransactionId = 0;
    PSTR NbName = NULL;
    struct in_addr *addrs = NULL;
    DWORD addrsLen = 0;
    struct timeval  tp = {0};
    struct timespec cvTimeout = {0};

    dwError = LWNetAllocateMemory(
                  LWNB_NETBIOS_UDP_MAX,
                  (PVOID*) &NetBiosReply);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    sts = setsockopt(sock,
              SOL_SOCKET,
              SO_BROADCAST,
              &allowBroadcast,
              sizeof(allowBroadcast));
    if (sts == -1)
    {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_LWNET_ERROR(dwError);
    }


    pthread_mutex_lock(&pNbCtx->mutex);
    pNbCtx->sock = sock;
    pthread_mutex_unlock(&pNbCtx->mutex);

    do
    {
        memset(pollfds, 0, sizeof(pollfds));
        pollfds[0].fd = sock;
        pollfds[0].events = POLLIN;

        sts = poll(pollfds, 1, -1);
        if (sts > 0 && pollfds[0].revents)
        {
            sts = recvfrom(
                  sock,
                  NetBiosReply,
                  LWNB_NETBIOS_UDP_MAX,
                  0,  /* flags */
                  (struct sockaddr *) &dgAddr,
                  pNetBiosReplyAddrLen);
            if (sts > 0)
            {
                dwError = LWNetNbParseNameQueryResponse(
                              NetBiosReply,
                              sts,
                              0, // remove this argument
                              &respTransactionId,
                              &NbName,
                              NULL,
                              &Flags,
                              &addrs,
                              &addrsLen);
                pthread_mutex_lock(&pNbCtx->mutex);
                pNbCtx->respError = dwError;
                pNbCtx->transactionId = respTransactionId;
                pNbCtx->addrs = addrs;
                pNbCtx->addrsLen = addrsLen;
                pNbCtx->bNbRepsponse = TRUE;
                pthread_mutex_lock(&pNbCtx->mutexAck);
                pthread_mutex_unlock(&pNbCtx->mutex);
                pthread_cond_broadcast(&pNbCtx->cv);

                // Wait for LWNetNbResolveName ack address received
                do
                {
                    gettimeofday(&tp, NULL);
                    cvTimeout.tv_sec = 
                        tp.tv_sec + pNbCtx->udpTimeout;
                    cvTimeout.tv_nsec = tp.tv_usec * 1000;
                    sts = pthread_cond_timedwait(
                              &pNbCtx->cvAck,
                              &pNbCtx->mutexAck,
                              &cvTimeout);
                } while (!pNbCtx->bAck && sts != ETIMEDOUT);
                pNbCtx->bAck = FALSE;
                pthread_mutex_unlock(&pNbCtx->mutexAck);
            }
        }
    } while (!pNbCtx->bShutdown);
    LWNET_LOG_INFO("Stopping NetBIOS listener thread");

cleanup:
    LWNET_SAFE_FREE_MEMORY(NetBiosReply);
    LWNET_SAFE_FREE_STRING(NbName);
    if (sock != -1)
    {
        close(sock);
    }
    return NULL;

error:
    goto cleanup;
}


DWORD
LWNetSrvNetBiosInit(VOID)
{
    struct {
        unsigned mutexInit : 1;
        unsigned mutexAckInit : 1;
        unsigned cvInit : 1;
        unsigned cvAckInit : 1;
    } initialized = {0};

    DWORD dwError = 0;
    PLWNET_SRV_NETBIOS_CONTEXT pNbCtx = NULL;

    dwError = LWNetAllocateMemory(
                  sizeof(*pNbCtx),
                  (PVOID*) &pNbCtx);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwErrnoToWin32Error(pthread_mutex_init(&pNbCtx->mutex, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    initialized.mutexInit = TRUE;

    dwError = LwErrnoToWin32Error(pthread_cond_init(&pNbCtx->cv, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    initialized.cvInit = TRUE;

    dwError = LwErrnoToWin32Error(pthread_mutex_init(&pNbCtx->mutexAck, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    initialized.mutexAckInit = TRUE;

    dwError = LwErrnoToWin32Error(pthread_cond_init(&pNbCtx->cvAck, NULL));
    BAIL_ON_LWNET_ERROR(dwError);
    initialized.cvAckInit = TRUE;

    dwError = LwErrnoToWin32Error(pthread_mutex_init(
                  &pNbCtx->mutexTransactionId, NULL));
    BAIL_ON_LWNET_ERROR(dwError);

    gpNbCtx = pNbCtx;

cleanup:
    return dwError;

error:
    if (initialized.mutexInit)
    {
        pthread_mutex_destroy(&pNbCtx->mutex);
    }
    if (initialized.mutexAckInit)
    {
        pthread_mutex_destroy(&pNbCtx->mutexAck);
    }
    if (initialized.cvInit)
    {
        pthread_cond_destroy(&pNbCtx->cv);
    }
    if (initialized.cvAckInit)
    {
        pthread_cond_destroy(&pNbCtx->cvAck);
    }
    LWNET_SAFE_FREE_MEMORY(pNbCtx);
    goto cleanup;
}


VOID
LWNetSrvNetBiosCleanup(
    VOID
    )
{
    if (!gpNbCtx)
    {
        return;
    }
    pthread_mutex_destroy(&gpNbCtx->mutex);
    pthread_mutex_destroy(&gpNbCtx->mutexAck);
    pthread_cond_destroy(&gpNbCtx->cv);
    pthread_cond_destroy(&gpNbCtx->cvAck);
    LWNET_SAFE_FREE_MEMORY(gpNbCtx);
}


DWORD
LWNetSrvStartNetBiosThread(
    VOID)
{
    DWORD dwError = 0;
    pthread_t thread;
    pthread_attr_t attrib;

    gpNbCtx->udpTimeout = LWNetConfigIsNetBiosUdpTimeout();

    dwError = LwErrnoToWin32Error(pthread_attr_init(&attrib));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwErrnoToWin32Error(pthread_attr_setdetachstate(
                  &attrib, PTHREAD_CREATE_DETACHED));
    BAIL_ON_LWNET_ERROR(dwError);
    dwError = LwErrnoToWin32Error(pthread_create(&thread,
                         &attrib,
                         LWNetSrvStartNetBiosThreadRoutine,
                         (void *) gpNbCtx));
    BAIL_ON_LWNET_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
LWNetSrvStartNetBios(
    VOID)
{
    DWORD dwError = 0;

    if (LWNetConfigIsNetBiosEnabled())
    {
        dwError = LWNetSrvNetBiosInit();
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetSrvStartNetBiosThread();
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    LWNetSrvNetBiosCleanup();
    LWNET_LOG_ERROR("Failed initializing NetBIOS listener thread %s",
                    LwErrnoToName(dwError));
    goto cleanup;
}


VOID
LWNetStopNetBios(
    VOID)
{
    if (!LWNetConfigIsNetBiosEnabled())
    {
        return;
    }
    pthread_mutex_lock(&gpNbCtx->mutexAck);
    gpNbCtx->bShutdown = TRUE;
    pthread_mutex_unlock(&gpNbCtx->mutexAck);
    shutdown(gpNbCtx->sock, 0);  // 0 = SHUT_RD; more portable?
    LWNetSrvNetBiosCleanup();
}
