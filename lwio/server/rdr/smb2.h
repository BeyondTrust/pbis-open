/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        smb2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        Common SMB2 Code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __RDR_SMB2_H__
#define __RDR_SMB2_H__

#define PACKET_LENGTH_REMAINING(pPacket, pOrigin) \
    ((pPacket)->bufferLen - ((PBYTE) (pOrigin)- (pPacket)->pRawBuffer))

#define PACKET_USED_REMAINING(pPacket, pOrigin) \
    ((pPacket)->bufferUsed - ((PBYTE) (pOrigin)- (pPacket)->pRawBuffer))

#define PACKET_HEADER_OFFSET(pPacket, pPointer) \
    ((PBYTE) (pPointer) - (PBYTE) (pPacket)->pSMB2Header)

#define RDR_SMB2_SECMODE_SIGNING_ENABLED 0x1
#define RDR_SMB2_SECMODE_SIGNING_REQUIRED 0x2

#define RDR_SMB2_CAP_DFS 0x1

#define RDR_SMB2_PACKET_HEADER_SIZE \
    (sizeof(NETBIOS_HEADER) + sizeof (SMB2_HEADER))

#define RDR_SMB2_STUB_SIZE \
    (RDR_SMB2_PACKET_HEADER_SIZE + 4)

#define RDR_SMB2_PACKET_BASE_SIZE(type) \
    (RDR_SMB2_PACKET_HEADER_SIZE + sizeof(RDR_SMB2_ ## type ## _REQUEST_HEADER))

#define RDR_SMB2_SESSION_SETUP_SIZE(ulBlobLen) \
    (RDR_SMB2_PACKET_BASE_SIZE(SESSION_SETUP) + (ulBlobLen))

#define RDR_SMB2_TREE_CONNECT_SIZE(ulPathLen) \
    (RDR_SMB2_PACKET_BASE_SIZE(TREE_CONNECT) + sizeof(WCHAR) * (ulPathLen))

#define RDR_SMB2_CREATE_BASE_SIZE(ulPathLen) \
    (RDR_SMB2_PACKET_BASE_SIZE(CREATE) + sizeof(WCHAR) * (ulPathLen) + 2)

#define RDR_SMB2_CLOSE_SIZE \
    (RDR_SMB2_PACKET_BASE_SIZE(CLOSE))

#define RDR_SMB2_QUERY_INFO_SIZE(ulInputLen) \
    (RDR_SMB2_PACKET_BASE_SIZE(QUERY_INFO) + (ulInputLen))

#define RDR_SMB2_SET_INFO_SIZE(ulInputLen) \
    (RDR_SMB2_PACKET_BASE_SIZE(SET_INFO) + (ulInputLen))

#define RDR_SMB2_QUERY_DIRECTORY_SIZE(ulPatternLen) \
    (RDR_SMB2_PACKET_BASE_SIZE(QUERY_DIRECTORY) + sizeof(WCHAR) * ulPatternLen)

#define RDR_SMB2_READ_SIZE \
    (RDR_SMB2_PACKET_BASE_SIZE(READ) + 1)

#define RDR_SMB2_WRITE_SIZE(ulLength) \
    (RDR_SMB2_PACKET_BASE_SIZE(WRITE) + (ulLength))

#define RDR_SMB2_IOCTL_SIZE(ulLength) \
    (RDR_SMB2_PACKET_BASE_SIZE(IOCTL) + (ulLength))

#define RDR_SMB2_MAX_SHARE_PATH_LENGTH 256

typedef struct _RDR_SMB2_NEGOTIATE_RESPONSE_HEADER
{
    USHORT usLength;
    UCHAR  ucSecurityMode;
    BYTE   ucPad1;
    USHORT usDialect;
    USHORT usPad2;
    BYTE   ServerGuid[16];
    ULONG  ulCapabilities;
    ULONG  ulMaxTransactionSize;
    ULONG  ulMaxReadSize;
    ULONG  ulMaxWriteSize;
    LONG64 llTime;
    LONG64 llBootTime;
    USHORT usHintOffset;
    USHORT usHintLength;
} __attribute__((__packed__))
RDR_SMB2_NEGOTIATE_RESPONSE_HEADER, *PRDR_SMB2_NEGOTIATE_RESPONSE_HEADER;

typedef struct _RDR_SMB2_SESSION_SETUP_REQUEST_HEADER
{
    USHORT  usLength;
    UCHAR   ucVcNumber;
    UCHAR   ucSecurityMode;
    ULONG   ulCapabilities;
    ULONG   ulChannel;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;
    ULONG64 ullPrevSessionId;
} __attribute__((__packed__))
RDR_SMB2_SESSION_SETUP_REQUEST_HEADER, *PRDR_SMB2_SESSION_SETUP_REQUEST_HEADER;

typedef struct _RDR_SMB2_SESSION_SETUP_RESPONSE_HEADER
{
    USHORT  usLength;
    USHORT  usSessionFlags;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;
} __attribute__((__packed__))
RDR_SMB2_SESSION_SETUP_RESPONSE_HEADER, *PRDR_SMB2_SESSION_SETUP_RESPONSE_HEADER;

typedef struct _RDR_SMB2_TREE_CONNECT_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usPad;
    USHORT usPathOffset;
    USHORT usPathLength;
} __attribute__((__packed__))
RDR_SMB2_TREE_CONNECT_REQUEST_HEADER, *PRDR_SMB2_TREE_CONNECT_REQUEST_HEADER;

typedef struct _RDR_SMB2_TREE_CONNECT_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usShareType;
    ULONG  ulShareFlags;
    ULONG  ulShareCapabilities;
    ULONG  ulShareAccessMask;
} __attribute__((__packed__))
RDR_SMB2_TREE_CONNECT_RESPONSE_HEADER, *PRDR_SMB2_TREE_CONNECT_RESPONSE_HEADER;

typedef struct _RDR_SMB2_CREATE_REQUEST_HEADER
{
    USHORT  usLength;
    UCHAR   ucSecurityFlags;
    UCHAR   ucOplockLevel;
    ULONG   ulImpersonationLevel;
    ULONG64 ullCreateFlags;
    ULONG64 ullReserved;
    ULONG   ulDesiredAccess;
    ULONG   ulFileAttributes;
    ULONG   ulShareAccess;
    ULONG   ulCreateDisposition;
    ULONG   ulCreateOptions;
    USHORT  usNameOffset;
    USHORT  usNameLength;
    ULONG   ulCreateContextOffset;
    ULONG   ulCreateContextLength;
} __attribute__((__packed__))
RDR_SMB2_CREATE_REQUEST_HEADER, *PRDR_SMB2_CREATE_REQUEST_HEADER;

typedef struct _RDR_SMB2_CREATE_RESPONSE_HEADER
{
    USHORT   usLength;
    UCHAR    ucOplockLevel;
    UCHAR    ucReserved;
    ULONG    ulCreateAction;
    ULONG64  ullCreationTime;
    ULONG64  ullLastAccessTime;
    ULONG64  ullLastWriteTime;
    ULONG64  ullLastChangeTime;
    ULONG64  ullAllocationSize;
    ULONG64  ullEndOfFile;
    ULONG    ulFileAttributes;
    ULONG    ulReserved2;
    RDR_SMB2_FID fid;
    ULONG    ulCreateContextOffset;
    ULONG    ulCreateContextLength;
} __attribute__((__packed__))
RDR_SMB2_CREATE_RESPONSE_HEADER, *PRDR_SMB2_CREATE_RESPONSE_HEADER;

typedef struct _RDR_SMB2_CLOSE_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    ulReserved;
    RDR_SMB2_FID fid;
} __attribute__((__packed__))
RDR_SMB2_CLOSE_REQUEST_HEADER, *PRDR_SMB2_CLOSE_REQUEST_HEADER;

typedef struct _RDR_SMB2_QUERY_INFO_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoType;
    UCHAR    ucInfoClass;
    ULONG    ulOutputBufferLen;
    USHORT   usInputBufferOffset;
    USHORT   usReserved;
    ULONG    ulInputBufferLen;
    ULONG    ulAdditionalInfo;
    ULONG    ulFlags;
    RDR_SMB2_FID fid;
} __attribute__((__packed__))
RDR_SMB2_QUERY_INFO_REQUEST_HEADER, *PRDR_SMB2_QUERY_INFO_REQUEST_HEADER;

typedef struct _RDR_SMB2_QUERY_INFO_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usOutBufferOffset;
    ULONG  ulOutBufferLength;
} __attribute__((__packed__))
RDR_SMB2_QUERY_INFO_RESPONSE_HEADER, *PRDR_SMB2_QUERY_INFO_RESPONSE_HEADER;

typedef struct _RDR_SMB2_QUERY_DIRECTORY_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoClass;
    UCHAR    ucSearchFlags;
    ULONG    ulFileIndex;
    RDR_SMB2_FID fid;
    USHORT   usFilenameOffset;
    USHORT   usFilenameLength;
    ULONG    ulOutBufferLength;
} __attribute__((__packed__))
RDR_SMB2_QUERY_DIRECTORY_REQUEST_HEADER, *PRDR_SMB2_QUERY_DIRECTORY_REQUEST_HEADER;

typedef struct _RDR_SMB2_QUERY_DIRECTORY_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usOutBufferOffset;
    ULONG    ulOutBufferLength;
} __attribute__((__packed__))
RDR_SMB2_QUERY_DIRECTORY_RESPONSE_HEADER, *PRDR_SMB2_QUERY_DIRECTORY_RESPONSE_HEADER;

typedef struct _RDR_SMB2_READ_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucDataOffset;
    UCHAR    ucReserved;
    ULONG    ulDataLength;
    ULONG64  ullFileOffset;
    RDR_SMB2_FID fid;
    ULONG    ulMinimumCount;
    ULONG    ulChannel;
    ULONG    ulRemainingBytes;
    USHORT   usReadChannelInfoOffset;
    USHORT   usReadChannelInfoLength;
} __attribute__((__packed__))
RDR_SMB2_READ_REQUEST_HEADER, *PRDR_SMB2_READ_REQUEST_HEADER;

typedef struct _RDR__SMB2_READ_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usDataOffset;
    ULONG  ulDataLength;
    ULONG  ulRemaining;
    ULONG  ulReserved;
} __attribute__((__packed__))
RDR_SMB2_READ_RESPONSE_HEADER, *PRDR_SMB2_READ_RESPONSE_HEADER;

typedef struct _RDR_SMB2_WRITE_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usDataOffset;
    ULONG    ulDataLength;
    ULONG64  ullFileOffset;
    RDR_SMB2_FID fid;
    ULONG    ulChannel;
    ULONG    ulRemainingBytes;
    USHORT   usWriteChannelInfoOffset;
    USHORT   usWriteChannelInfoLength;
    ULONG    ulFlags;
} __attribute__((__packed__))
RDR_SMB2_WRITE_REQUEST_HEADER, *PRDR_SMB2_WRITE_REQUEST_HEADER;

typedef struct _RDR_SMB2_WRITE_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulBytesWritten;
    ULONG    ulBytesRemaining;
    USHORT   usWriteChannelInfoOffset;
    USHORT   usWriteChannelInfoLength;
} __attribute__((__packed__))
RDR_SMB2_WRITE_RESPONSE_HEADER, *PRDR_SMB2_WRITE_RESPONSE_HEADER;

typedef struct _RDR_SMB2_SET_INFO_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoType;
    UCHAR    ucInfoClass;
    ULONG    ulInputBufferLen;
    USHORT   usInputBufferOffset;
    USHORT   usReserved;
    ULONG    ulAdditionalInfo;
    RDR_SMB2_FID fid;
} __attribute__((__packed__))
RDR_SMB2_SET_INFO_REQUEST_HEADER, *PRDR_SMB2_SET_INFO_REQUEST_HEADER;

typedef struct _RDR_SMB2_IOCTL_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulFunctionCode;
    RDR_SMB2_FID fid;
    ULONG    ulInOffset;
    ULONG    ulInLength;
    ULONG    ulMaxInLength;
    ULONG    ulOutOffset;
    ULONG    ulOutLength;
    ULONG    ulMaxOutLength;
    ULONG    ulFlags;
    ULONG    ulReserved;
} __attribute__((__packed__))
RDR_SMB2_IOCTL_REQUEST_HEADER, *PRDR_SMB2_IOCTL_REQUEST_HEADER;

typedef struct _RDR_SMB2_IOCTL_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulFunctionCode;
    RDR_SMB2_FID fid;
    ULONG    ulInOffset;
    ULONG    ulInLength;
    ULONG    ulOutOffset;
    ULONG    ulOutLength;
    ULONG    ulFlags;
    ULONG    ulReserved;
} __attribute__((__packed__))
RDR_SMB2_IOCTL_RESPONSE_HEADER, *PRDR_SMB2_IOCTL_RESPONSE_HEADER;

typedef struct _RDR_SMB2_FILE_RENAME_INFORMATION
{
    UCHAR     ucReplaceIfExists;
    UCHAR     ucReserved[7];
    ULONG64   ullRootDir;
    ULONG     ulFileNameLength;
} __attribute__((__packed__))
RDR_SMB2_FILE_RENAME_INFO_HEADER, *PRDR_SMB2_FILE_RENAME_INFO_HEADER;

typedef struct _RDR_SMB2_FILE_FS_SIZE_INFORMATION {
    LONG64  llTotalAllocationUnits;
    LONG64  llAvailableAllocationUnits;
    ULONG   ulSectorsPerAllocationUnit;
    ULONG   ulBytesPerSector;
} RDR_SMB2_FILE_FS_SIZE_INFORMATION, *PRDR_SMB2_FILE_FS_SIZE_INFORMATION;

BOOLEAN
RdrSmb2ShouldSignPacket(
    PSMB_PACKET pPacket,
    BOOLEAN bServerSigningEnabled,
    BOOLEAN bServerSigningRequired,
    BOOLEAN bClientSigningEnabled,
    BOOLEAN bClientSigningRequired
    );

BOOLEAN
RdrSmb2ShouldVerifyPacket(
    PSMB_PACKET pPacket,
    BOOLEAN bClientSigningRequired
    );

NTSTATUS
RdrSmb2DecodeHeader(
    PSMB_PACKET pPacket,
    BOOLEAN bVerifySignature,
    PBYTE pSessionKey,
    DWORD dwSessionKeyLength
    );

NTSTATUS
RdrSmb2BeginPacket(
    PSMB_PACKET pPacket
    );

NTSTATUS
RdrSmb2EncodeHeader(
    PSMB_PACKET pPacket,
    USHORT usCommand,
    ULONG ulFlags,
    ULONG ulPid,
    ULONG ulTid,
    ULONG64 ullSessionId,
    PBYTE* ppCursor,
    PULONG pulRemaining
    );

NTSTATUS
RdrSmb2FinishCommand(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining
    );

NTSTATUS
RdrSmb2Sign(
    PSMB_PACKET pPacket,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    );

NTSTATUS
RdrSmb2VerifySignature(
    PSMB_PACKET pPacket,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    );

NTSTATUS
RdrSmb2EncodeStubRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining
    );

NTSTATUS
RdrSmb2DecodeNegotiateResponse(
    PSMB_PACKET pPacket,
    PRDR_SMB2_NEGOTIATE_RESPONSE_HEADER* ppHeader,
    PBYTE* ppNegHint,
    PULONG pulNegHintLength
    );

NTSTATUS
RdrSmb2EncodeSessionSetupRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    BOOLEAN bSigningEnabled,
    BOOLEAN bSigningRequired,
    BOOLEAN bDfsEnabled,
    PBYTE pBlob,
    ULONG ulBlobLength
    );

NTSTATUS
RdrSmb2DecodeSessionSetupResponse(
    PSMB_PACKET pPacket,
    PUSHORT pusSessionFlags,
    PBYTE* ppBlob,
    PULONG pulBlobLength
    );

NTSTATUS
RdrSmb2EncodeTreeConnectRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    PCWSTR pwszPath
    );

NTSTATUS
RdrSmb2DecodeTreeConnectResponse(
    PSMB_PACKET pPacket,
    PRDR_SMB2_TREE_CONNECT_RESPONSE_HEADER* ppHeader
    );

NTSTATUS
RdrSmb2EncodeCreateRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucOplockLevel,
    ULONG ulImpersonationLevel,
    ULONG ulDesiredAccess,
    ULONG ulFileAttributes,
    ULONG ulShareAccess,
    ULONG ulCreateDisposition,
    ULONG ulCreateOptions,
    PCWSTR pwszPath,
    PULONG* ppulCreateContextsOffset,
    PULONG* ppulCreateContextsLength
    );

NTSTATUS
RdrSmb2DecodeCreateResponse(
    PSMB_PACKET pPacket,
    PRDR_SMB2_CREATE_RESPONSE_HEADER* ppHeader
    );

NTSTATUS
RdrSmb2EncodeCloseRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    USHORT usFlags,
    PRDR_SMB2_FID pFid
    );

NTSTATUS
RdrSmb2EncodeQueryInfoRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucInfoType,
    UCHAR ucFileInfoClass,
    ULONG ulOutputBufferLen,
    ULONG ulAdditionalInfo,
    ULONG ulFlags,
    PRDR_SMB2_FID pFid,
    PULONG* ppulInputBufferLen
    );

NTSTATUS
RdrSmb2DecodeQueryInfoResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppOutputBuffer,
    PULONG pulOutputBufferLen
    );

NTSTATUS
RdrSmb2EncodeQueryDirectoryRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucFileInfoClass,
    UCHAR ucFlags,
    ULONG ulFileIndex,
    PRDR_SMB2_FID pFid,
    PCWSTR pwszPattern,
    ULONG ulOutputBufferLength
    );

NTSTATUS
RdrSmb2DecodeQueryDirectoryResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppOutputBuffer,
    PULONG pulOutputBufferLen
    );

NTSTATUS
RdrSmb2EncodeReadRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    ULONG ulDataLength,
    LONG64 llDataOffset,
    PRDR_SMB2_FID pFid,
    ULONG ulMinimumCount,
    ULONG ulRemainingBytes
    );

NTSTATUS
RdrSmb2DecodeReadResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppDataBuffer,
    PULONG pulDataLength
    );

NTSTATUS
RdrSmb2EncodeWriteRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    LONG64 llDataOffset,
    PRDR_SMB2_FID pFid,
    ULONG ulRemainingBytes,
    ULONG ulFlags,
    PULONG* ppulDataLength
    );

NTSTATUS
RdrSmb2DecodeWriteResponse(
    PSMB_PACKET pPacket,
    PULONG pulDataCount
    );

NTSTATUS
RdrSmb2EncodeSetInfoRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    UCHAR ucInfoType,
    UCHAR ucFileInfoClass,
    ULONG ulAdditionalInfo,
    PRDR_SMB2_FID pFid,
    PULONG* ppulBufferLen
    );

NTSTATUS
RdrSmb2EncodeIoctlRequest(
    PSMB_PACKET pPacket,
    PBYTE* ppCursor,
    PULONG pulRemaining,
    ULONG ulControlCode,
    PRDR_SMB2_FID pFid,
    ULONG ulMaxInputResponse,
    ULONG ulMaxOutputResponse,
    BOOLEAN bIsFsctl,
    PULONG* ppulInputSize
    );

NTSTATUS
RdrSmb2DecodeIoctlResponse(
    PSMB_PACKET pPacket,
    PBYTE* ppOutput,
    PULONG pulOutputSize
    );

#endif /* __RDR_SMB2_H__ */
