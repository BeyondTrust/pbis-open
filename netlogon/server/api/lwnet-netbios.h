#ifndef _NBFUNC_H_
#define _NBFUNC_H_

#include <arpa/inet.h>

#define LWNB_QUERY_BROADCAST 1
#define LWNB_QUERY_WINS      0
#define LWNB_OPCODE_QUERY    0 
#define LWNB_OPCODE_RESPONSE 1

#define LWNB_RESOLVE_WORKSTATION  0x00
#define LWNB_RESOLVE_DC           0x1C
#define LWNB_RESOLVE_FILE_SERVICE 0x20

#define LWNB_SHIFT_FLAG_BROADCAST         4
#define LWNB_SHIFT_FLAG_RECURSION_DESIRED 8
#define LWNB_SHIFT_OPCODE                 11
#define LWNB_SHIFT_REQUEST_TYPE           15
#define LWNB_NAME_QUERY_HEADER_SIZE 12
#define LWNB_NAME_MAX_LENGTH 16
#define LWNB_NETBIOS_ENCNAME_LEN 32
#define LWNB_NETBIOS_UDP_MAX 1500 // MTU

void 
LWNetNbHexDumpBuf(
    UINT8 *buf,
    int len);

DWORD
LWNetNbConstructNameQuery(
    IN PSTR pszNetBiosHost,
    IN UINT8 bBroadcast,
    IN UINT8 queryType,
    OUT UINT16 *RetTransactionId,
    OUT UINT8 *NetBiosQuery,
    OUT PDWORD NetBiosQueryLen);

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
    OUT PDWORD retAddrsLen);

void
LWNetNbAddressListFree(
    IN struct in_addr *retAddrs);

void
LWNetNbStrToNbName(
    IN PSTR pszHost, 
    IN UINT8 suffix,
    OUT PBYTE nbBuf);

void 
LWNetNbNameToStr(
    IN PBYTE nbBuf, 
    OUT PSTR pszHost,
    OUT OPTIONAL PUINT8 pSuffix);

DWORD
LWNetNbName2ToParts(
    IN PBYTE NbNameL2,
    OUT PSTR **retNbNameParts,
    OUT PDWORD retNbNamePartsLen);

DWORD
LWNetNbStrToNbName2(
    IN PSTR Fqdn,
    IN UINT8 suffix,
    OUT PBYTE *retNbNameL2,
    OUT PDWORD retNbNameL2Len);

DWORD
LWNetNbName2ToStr(
    IN PBYTE buf,
    OUT PSTR *ppNbName,
    OUT PUINT8 pSuffix,
    OUT PDWORD dwBytesConsumed);

DWORD
LWNetSrvNetBiosInit(
    VOID
    );

VOID
LWNetSrvNetBiosCleanup(
    VOID
    );

#endif
