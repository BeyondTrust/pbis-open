/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __SMB2_NEGOTIATE_REQUEST_HEADER
{
    USHORT  usLength;
    USHORT  usDialectCount;
    USHORT  usSecurityMode;
    USHORT  usPad;
    ULONG   ulCapabilities;
    UCHAR   clientGUID[16];
    ULONG64 ulStartTime;

    // List of dialects follow immediately
} __attribute__((__packed__)) SMB2_NEGOTIATE_REQUEST_HEADER,
                             *PSMB2_NEGOTIATE_REQUEST_HEADER;

typedef struct __SMB2_NEGOTIATE_RESPONSE_HEADER
{
    USHORT  usLength;
    BYTE    ucFlags;
    BYTE    ucPad;
    USHORT  usDialect;
    USHORT  usPad2;
    BYTE    serverGUID[16];
    ULONG   ulCapabilities;
    ULONG   ulMaxTxSize;
    ULONG   ulMaxReadSize;
    ULONG   ulMaxWriteSize;
    ULONG64 ullCurrentTime;
    ULONG64 ullBootTime;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_NEGOTIATE_RESPONSE_HEADER,
                             *PSMB2_NEGOTIATE_RESPONSE_HEADER;

typedef struct __SMB2_SESSION_SETUP_REQUEST_HEADER
{
    USHORT  usLength;
    BYTE    ucVCNumber;
    BYTE    ucSecurityMode;
    ULONG   ulCapabilities;
    ULONG   ulChannel;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;
    ULONG64 ullPrevSessionId;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_SESSION_SETUP_REQUEST_HEADER;

typedef SMB2_SESSION_SETUP_REQUEST_HEADER *PSMB2_SESSION_SETUP_REQUEST_HEADER;

typedef struct __SMB2_SESSION_SETUP_RESPONSE_HEADER
{
    USHORT  usLength;
    USHORT  usSessionFlags;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_SESSION_SETUP_RESPONSE_HEADER,
                             *PSMB2_SESSION_SETUP_RESPONSE_HEADER;

typedef struct __SMB2_LOGOFF_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_LOGOFF_REQUEST_HEADER,
                             *PSMB2_LOGOFF_REQUEST_HEADER;

typedef struct __SMB2_LOGOFF_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_LOGOFF_RESPONSE_HEADER,
                             *PSMB2_LOGOFF_RESPONSE_HEADER;

typedef struct __SMB2_TREE_CONNECT_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usPad;
    USHORT usPathOffset;
    USHORT usPathLength;
} __attribute__((__packed__)) SMB2_TREE_CONNECT_REQUEST_HEADER,
                             *PSMB2_TREE_CONNECT_REQUEST_HEADER;

typedef struct __SMB2_TREE_CONNECT_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usShareType;
    ULONG  ulShareFlags;
    ULONG  ulShareCapabilities;
    ULONG  ulShareAccessMask;
} __attribute__((__packed__)) SMB2_TREE_CONNECT_RESPONSE_HEADER,
                             *PSMB2_TREE_CONNECT_RESPONSE_HEADER;

typedef struct __SMB2_TREE_DISCONNECT_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_TREE_DISCONNECT_REQUEST_HEADER,
                             *PSMB2_TREE_DISCONNECT_REQUEST_HEADER;

typedef struct __SMB2_TREE_DISCONNECT_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_TREE_DISCONNECT_RESPONSE_HEADER,
                             *PSMB2_TREE_DISCONNECT_RESPONSE_HEADER;

typedef struct __SMB2_CREATE_CONTEXT
{
    ULONG  ulNextContextOffset;
    USHORT usNameOffset;
    USHORT usNameLength;
    USHORT usReserved;
    USHORT usDataOffset;
    ULONG  ulDataLength;

    /* ANSI Name */
    /* Optional padding to 8 byte boundary */
    /* Data */

} __attribute__((__packed__)) SMB2_CREATE_CONTEXT,
                             *PSMB2_CREATE_CONTEXT;

typedef struct __NFS_CREATE_CONTEXT
{
    SMB2_CONTEXT_ITEM_TYPE contextItemType;
    PCSTR                  pszName;
    USHORT                 usNameLen;
    ULONG                  ulDataLength;
    PBYTE                  pData;
} NFS_CREATE_CONTEXT, *PNFS_CREATE_CONTEXT;

typedef struct __SMB2_CREATE_REQUEST_HEADER
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
} __attribute__((__packed__)) SMB2_CREATE_REQUEST_HEADER,
                             *PSMB2_CREATE_REQUEST_HEADER;

typedef struct __SMB2_CREATE_RESPONSE_HEADER
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
    SMB2_FID fid;
    ULONG    ulCreateContextOffset;
    ULONG    ulCreateContextLength;
} __attribute__((__packed__)) SMB2_CREATE_RESPONSE_HEADER,
                             *PSMB2_CREATE_RESPONSE_HEADER;

typedef struct __SMB2_MAXIMAL_ACCESS_MASK_CREATE_CONTEXT
{
    ULONG       ulQueryStatus;
    ACCESS_MASK accessMask;
} __attribute__((__packed__))  SMB2_MAXIMAL_ACCESS_MASK_CREATE_CONTEXT,
                             *PSMB2_MAXIMAL_ACCESS_MASK_CREATE_CONTEXT;

typedef struct __SMB2_CLOSE_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    ulReserved;
    SMB2_FID fid;
} __attribute__((__packed__)) SMB2_CLOSE_REQUEST_HEADER,
                             *PSMB2_CLOSE_REQUEST_HEADER;

typedef struct __SMB2_CLOSE_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    ulReserved;
    ULONG64  ullCreationTime;
    ULONG64  ullLastAccessTime;
    ULONG64  ullLastWriteTime;
    ULONG64  ullLastChangeTime;
    ULONG64  ullAllocationSize;
    ULONG64  ullEndOfFile;
    ULONG    ulFileAttributes;
} __attribute__((__packed__)) SMB2_CLOSE_RESPONSE_HEADER,
                             *PSMB2_CLOSE_RESPONSE_HEADER;

typedef struct __SMB2_FLUSH_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    ulReserved;
    SMB2_FID fid;
} __attribute__((__packed__)) SMB2_FLUSH_REQUEST_HEADER,
                             *PSMB2_FLUSH_REQUEST_HEADER;

typedef struct __SMB2_FLUSH_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
} __attribute__((__packed__)) SMB2_FLUSH_RESPONSE_HEADER,
                             *PSMB2_FLUSH_RESPONSE_HEADER;

typedef struct __SMB2_ECHO_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
} __attribute__((__packed__)) SMB2_ECHO_REQUEST_HEADER,
                             *PSMB2_ECHO_REQUEST_HEADER;

typedef struct __SMB2_ECHO_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
} __attribute__((__packed__)) SMB2_ECHO_RESPONSE_HEADER,
                             *PSMB2_ECHO_RESPONSE_HEADER;

typedef struct __SMB2_GET_INFO_REQUEST_HEADER
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
    SMB2_FID fid;

} __attribute__((__packed__)) SMB2_GET_INFO_REQUEST_HEADER,
                             *PSMB2_GET_INFO_REQUEST_HEADER;

typedef struct __SMB2_GET_INFO_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usOutBufferOffset;
    ULONG  ulOutBufferLength;

} __attribute__((__packed__)) SMB2_GET_INFO_RESPONSE_HEADER,
                             *PSMB2_GET_INFO_RESPONSE_HEADER;

typedef struct __SMB2_SET_INFO_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoType;
    UCHAR    ucInfoClass;
    ULONG    ulInputBufferLen;
    USHORT   usInputBufferOffset;
    USHORT   usReserved;
    ULONG    ulAdditionalInfo;
    SMB2_FID fid;

} __attribute__((__packed__)) SMB2_SET_INFO_REQUEST_HEADER,
                             *PSMB2_SET_INFO_REQUEST_HEADER;

typedef struct __SMB2_SET_INFO_RESPONSE_HEADER
{
    USHORT usLength;

} __attribute__((__packed__)) SMB2_SET_INFO_RESPONSE_HEADER,
                             *PSMB2_SET_INFO_RESPONSE_HEADER;

typedef struct __SMB2_WRITE_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usDataOffset;
    ULONG    ulDataLength;
    ULONG64  ullFileOffset;
    SMB2_FID fid;
    ULONG    ulRemaining;
    ULONG    ulChannel;
    USHORT   usWriteChannelInfoOffset;
    USHORT   usWriteChannelInfoLength;
    ULONG    ulFlags;

} __attribute__((__packed__)) SMB2_WRITE_REQUEST_HEADER,
                             *PSMB2_WRITE_REQUEST_HEADER;

typedef struct __SMB2_WRITE_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulBytesWritten;
    ULONG    ulBytesRemaining;
    USHORT   usWriteChannelInfoOffset;
    USHORT   usWriteChannelInfoLength;

} __attribute__((__packed__)) SMB2_WRITE_RESPONSE_HEADER,
                             *PSMB2_WRITE_RESPONSE_HEADER;

typedef struct __SMB2_READ_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulDataLength;
    ULONG64  ullFileOffset;
    SMB2_FID fid;
    ULONG    ulMinimumCount;
    ULONG    ulChannel;
    USHORT   usReadChannelInfoOffset;
    USHORT   usReadChannelInfoLength;

} __attribute__((__packed__)) SMB2_READ_REQUEST_HEADER,
                             *PSMB2_READ_REQUEST_HEADER;

typedef struct __SMB2_READ_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usDataOffset;
    ULONG  ulDataLength;
    ULONG  ulRemaining;
    ULONG  ulReserved;

} __attribute__((__packed__)) SMB2_READ_RESPONSE_HEADER,
                             *PSMB2_READ_RESPONSE_HEADER;

typedef struct __SMB2_IOCTL_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulFunctionCode;
    SMB2_FID fid;
    ULONG    ulInOffset;
    ULONG    ulInLength;
    ULONG    ulMaxInLength;
    ULONG    ulOutOffset;
    ULONG    ulOutLength;
    ULONG    ulMaxOutLength;
    ULONG    ulFlags;
    ULONG    ulReserved;

} __attribute__((__packed__)) SMB2_IOCTL_REQUEST_HEADER,
                             *PSMB2_IOCTL_REQUEST_HEADER;

typedef struct __SMB2_IOCTL_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulFunctionCode;
    SMB2_FID fid;
    ULONG    ulInOffset;
    ULONG    ulInLength;
    ULONG    ulOutOffset;
    ULONG    ulOutLength;
    ULONG    ulFlags;
    ULONG    ulReserved;

} __attribute__((__packed__)) SMB2_IOCTL_RESPONSE_HEADER,
                             *PSMB2_IOCTL_RESPONSE_HEADER;

typedef struct __SMB2_LOCK
{
    LONG64 ullFileOffset;
    LONG64 ullByteRange;
    ULONG  ulFlags;
    ULONG  ulReserved;
} __attribute__((__packed__)) SMB2_LOCK, *PSMB2_LOCK;

typedef struct __SMB2_LOCK_REQUEST_HEADER
{
    USHORT    usLength;
    USHORT    usLockCount;
    ULONG     ulLockSequence;
    SMB2_FID  fid;
    SMB2_LOCK locks[1];
} __attribute__((__packed__)) SMB2_LOCK_REQUEST_HEADER,
                             *PSMB2_LOCK_REQUEST_HEADER;

typedef struct __SMB2_LOCK_RESPONSE_HEADER
{
    USHORT    usLength;
    USHORT    usReserved;
} __attribute__((__packed__)) SMB2_LOCK_RESPONSE_HEADER,
                             *PSMB2_LOCK_RESPONSE_HEADER;;

typedef struct __SMB2_FIND_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoClass;
    UCHAR    ucSearchFlags;
    ULONG    ulFileIndex;
    SMB2_FID fid;
    USHORT   usFilenameOffset;
    USHORT   usFilenameLength;
    ULONG    ulOutBufferLength;

    /* File name/Search pattern follows */

} __attribute__((__packed__)) SMB2_FIND_REQUEST_HEADER,
                             *PSMB2_FIND_REQUEST_HEADER;

typedef struct __SMB2_FIND_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usOutBufferOffset;
    ULONG    ulOutBufferLength;

    /* File name/Search results follow */

} __attribute__((__packed__)) SMB2_FIND_RESPONSE_HEADER,
                             *PSMB2_FIND_RESPONSE_HEADER;

typedef struct __SMB2_NOTIFY_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usOutBufferOffset;
    ULONG    ulOutBufferLength;

    /* Notify results follow */

} __attribute__((__packed__)) SMB2_NOTIFY_RESPONSE_HEADER,
                             *PSMB2_NOTIFY_RESPONSE_HEADER;

typedef struct __SMB2_OPLOCK_BREAK_HEADER
{
    USHORT   usLength;
    UCHAR    ucOplockLevel;
    UCHAR    ucReserved;
    ULONG    ulReserved;
    SMB2_FID fid;
} __attribute__((__packed__)) SMB2_OPLOCK_BREAK_HEADER,
                             *PSMB2_OPLOCK_BREAK_HEADER;

typedef struct __SMB2_ERROR_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usReserved;
    ULONG  ulByteCount;
    // Error message follows
} __attribute__((__packed__)) SMB2_ERROR_RESPONSE_HEADER,
                             *PSMB2_ERROR_RESPONSE_HEADER;

typedef struct _SMB2_FILE_ID_BOTH_DIR_INFO_HEADER
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    UCHAR           ucShortNameLength;
    UCHAR           ucReserved1;
    WCHAR           wszShortName[12];
    USHORT          usReserved2;
    LONG64          llFileId;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_ID_BOTH_DIR_INFO_HEADER,
                             *PSMB2_FILE_ID_BOTH_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_ID_FULL_DIR_INFO_HEADER
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    ULONG           ulReserved;
    LONG64          llFileId;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_ID_FULL_DIR_INFO_HEADER,
                             *PSMB2_FILE_ID_FULL_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_BOTH_DIR_INFO_HEADER
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    USHORT          usShortNameLength;
    WCHAR           wszShortName[12];
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_BOTH_DIR_INFO_HEADER,
                             *PSMB2_FILE_BOTH_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_FULL_DIR_INFO_HEADER
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_FULL_DIR_INFO_HEADER,
                             *PSMB2_FILE_FULL_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_DIRECTORY_INFO_HEADER
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_DIRECTORY_INFO_HEADER,
                             *PSMB2_FILE_DIRECTORY_INFO_HEADER;

typedef struct _SMB2_FILE_NAMES_INFO_HEADER
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    ULONG           ulFileNameLength;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_NAMES_INFO_HEADER,
                             *PSMB2_FILE_NAMES_INFO_HEADER;

typedef struct _SMB2_FILE_ALL_INFORMATION_HEADER
{
    LONG64          llCreationTime;       /* FileBasicInformation     */
    LONG64          llLastAccessTime;
    LONG64          llLastWriteTime;
    LONG64          llChangeTime;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulReserved;
    LONG64          ullAllocationSize;    /* FileStandardInformation  */
    LONG64          ullEndOfFile;
    ULONG           ulNumberOfLinks;
    UCHAR           ucDeletePending;
    UCHAR           ucIsDirectory;
    USHORT          usReserved;
    ULONG64         ullIndexNumber;       /* FileInternalInformation  */
    ULONG           ulEaSize;             /* FileEAInformation        */
    ULONG           ulAccessMask;         /* FileAccessInformation    */
    ULONG64         ullCurrentByteOffset; /* FilePositionInformation  */
    ULONG           ulMode;               /* FileModeInformation      */
    ULONG           ulAlignment;          /* FileAlignmentInformation */
    ULONG           ulFilenameLength;     /* FileNameInformation      */
    // WCHAR           wszFilename[1];

} __attribute__((__packed__)) SMB2_FILE_ALL_INFORMATION_HEADER,
                             *PSMB2_FILE_ALL_INFORMATION_HEADER;

typedef struct _SMB2_FILE_RENAME_INFORMATION
{
    UCHAR     ucReplaceIfExists;
    UCHAR     ucReserved[7];
    ULONG64   ullRootDir;
    ULONG     ulFileNameLength;
    WCHAR     wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_RENAME_INFO_HEADER,
                             *PSMB2_FILE_RENAME_INFO_HEADER;

typedef struct _SMB2_FILE_STREAM_INFORMATION_HEADER
{
    ULONG   ulNextEntryOffset;
    ULONG   ulStreamNameLength;
    ULONG64 ullStreamSize;
    LONG64  llStreamAllocationSize;
} __attribute__((__packed__)) SMB2_FILE_STREAM_INFORMATION_HEADER,
                             *PSMB2_FILE_STREAM_INFORMATION_HEADER;

typedef struct _SMB2_FILE_FULL_EA_INFORMATION_HEADER
{
    ULONG  ulNextEntryOffset;
    UCHAR  ucFlags;
    UCHAR  ucEaNameLength;
    USHORT usEaValueLength;
    // CHAR   szEaName[1];
    // PBYTE  pEaValue;
} __attribute__((__packed__)) SMB2_FILE_FULL_EA_INFORMATION_HEADER,
                             *PSMB2_FILE_FULL_EA_INFORMATION_HEADER;

typedef struct _SMB2_FILE_COMPRESSION_INFORMATION_HEADER
{
    LONG64 llCompressedFileSize;
    USHORT usCompressionFormat;
    UCHAR  ucCompressionUnitShift;
    UCHAR  ucChunkShift;
    UCHAR  ucClusterShift;
    UCHAR  ucReserved[3];
} __attribute__((__packed__)) SMB2_FILE_COMPRESSION_INFORMATION_HEADER,
                             *PSMB2_FILE_COMPRESSION_INFORMATION_HEADER;

typedef struct _SMB2_FILE_NAME_INFORMATION
{
    ULONG     ulFileNameLength;
    WCHAR     FileName[];
} __attribute__((__packed__)) SMB2_FILE_NAME_INFORMATION,
                             *PSMB2_FILE_NAME_INFORMATION;

typedef struct _SMB2_FILE_NETWORK_OPEN_INFORMATION {
    LONG64          llCreationTime;
    LONG64          llLastAccessTime;
    LONG64          llLastWriteTime;
    LONG64          llChangeTime;
    ULONG64         ullAllocationSize;
    ULONG64         ullEndOfFile;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulPad;
} __attribute__((__packed__)) SMB2_FILE_NETWORK_OPEN_INFORMATION,
                            *PSMB2_FILE_NETWORK_OPEN_INFORMATION;

typedef struct
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    ulOutputBufferLength;
    SMB2_FID fid;
    ULONG    ulCompletionFilter;
    ULONG    ulReserved;
} __attribute__((__packed__)) SMB2_NOTIFY_CHANGE_HEADER,
                             *PSMB2_NOTIFY_CHANGE_HEADER;

typedef enum
{
    NFS_TREE_CONNECT_STAGE_SMB_V2_INITIAL = 0,
    NFS_TREE_CONNECT_STAGE_SMB_V2_CREATE_TREE_ROOT_HANDLE,
    NFS_TREE_CONNECT_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_TREE_CONNECT_STAGE_SMB_V2_DONE
} NFS_TREE_CONNECT_STAGE_SMB_V2;

typedef struct _NFS_TREE_CONNECT_STATE_SMB_V2
{
    LONG                          refCount;

    pthread_mutex_t               mutex;
    pthread_mutex_t*              pMutex;

    NFS_TREE_CONNECT_STAGE_SMB_V2 stage;

    IO_STATUS_BLOCK               ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK        acb;
    PIO_ASYNC_CONTROL_BLOCK       pAcb;

    PSMB2_TREE_CONNECT_REQUEST_HEADER pRequestHeader; // Do not free

    PWSTR                         pwszPath;

    PVOID                         pSecurityDescriptor;
    PVOID                         pSecurityQOS;
    PIO_ECP_LIST                  pEcpList;

    PLWIO_NFS_SESSION_2           pSession;
    PLWIO_NFS_TREE_2              pTree;

    IO_FILE_NAME                  fileName;

    BOOLEAN                       bRemoveTreeFromSession;

} NFS_TREE_CONNECT_STATE_SMB_V2, *PNFS_TREE_CONNECT_STATE_SMB_V2;

typedef VOID (*PFN_NFS_MESSAGE_STATE_RELEASE_SMB_V2)(HANDLE hState);

typedef struct _NFS_OPLOCK_INFO
{
    UCHAR oplockRequest;
    UCHAR oplockLevel;
} NFS_OPLOCK_INFO, *PNFS_OPLOCK_INFO;

typedef struct _NFS_OPLOCK_STATE_SMB_V2
{
    LONG                    refCount;

    pthread_mutex_t         mutex;
    pthread_mutex_t*        pMutex;

    IO_STATUS_BLOCK         ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    PLWIO_NFS_CONNECTION    pConnection;

    ULONG64                 ullUid;
    ULONG64                 ulTid;

    SMB2_FID                fid;
    BOOLEAN                 bBreakRequestSent;

    PNFS_TIMER_REQUEST      pTimerRequest;

    IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER   oplockBuffer_in;
    IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER  oplockBuffer_out;
    IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER oplockBuffer_ack;

} NFS_OPLOCK_STATE_SMB_V2, *PNFS_OPLOCK_STATE_SMB_V2;

typedef enum
{
    NFS_CREATE_STAGE_SMB_V2_INITIAL = 0,
    NFS_CREATE_STAGE_SMB_V2_CREATE_FILE_COMPLETED,
    NFS_CREATE_STAGE_SMB_V2_ATTEMPT_QUERY_INFO,
    NFS_CREATE_STAGE_SMB_V2_QUERY_CREATE_CONTEXTS,
    NFS_CREATE_STAGE_SMB_V2_QUERY_INFO_COMPLETED,
    NFS_CREATE_STAGE_SMB_V2_REQUEST_OPLOCK,
    NFS_CREATE_STAGE_SMB_V2_DONE
} NFS_CREATE_STAGE_SMB_V2;

typedef struct _NFS_CREATE_STATE_SMB_V2
{
    LONG                         refCount;

    pthread_mutex_t              mutex;
    pthread_mutex_t*             pMutex;

    NFS_CREATE_STAGE_SMB_V2      stage;

    PSMB2_CREATE_REQUEST_HEADER  pRequestHeader; // Do not free

    PWSTR                        pwszFilename;

    IO_STATUS_BLOCK              ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK       acb;
    PIO_ASYNC_CONTROL_BLOCK      pAcb;

    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor; // Do not free
    PVOID                         pSecurityQOS;
    PIO_FILE_NAME                 pFilename;
    PIO_ECP_LIST                  pEcpList;
    IO_FILE_HANDLE                hFile;

    PNFS_CREATE_CONTEXT          pCreateContexts;
    ULONG                        iContext;
    ULONG                        ulNumContexts;

    PNFS_CREATE_CONTEXT          pExtAContext;

    FILE_BASIC_INFORMATION       fileBasicInfo;
    PFILE_BASIC_INFORMATION      pFileBasicInfo;

    FILE_STANDARD_INFORMATION    fileStdInfo;
    PFILE_STANDARD_INFORMATION   pFileStdInfo;

    FILE_PIPE_INFORMATION        filePipeInfo;
    PFILE_PIPE_INFORMATION       pFilePipeInfo;

    FILE_PIPE_LOCAL_INFORMATION  filePipeLocalInfo;
    PFILE_PIPE_LOCAL_INFORMATION pFilePipeLocalInfo;

    ACCESS_MASK                  ulMaximalAccessMask;

    FILE_CREATE_RESULT           ulCreateAction;

    UCHAR                        ucOplockLevel;

    PLWIO_NFS_TREE_2             pTree;
    PLWIO_NFS_FILE_2             pFile;
    BOOLEAN                      bRemoveFileFromTree;

    ULONG64                      ullAsyncId;

} NFS_CREATE_STATE_SMB_V2, *PNFS_CREATE_STATE_SMB_V2;

typedef enum
{
    NFS_FLUSH_STAGE_SMB_V2_INITIAL = 0,
    NFS_FLUSH_STAGE_SMB_V2_FLUSH_COMPLETED,
    NFS_FLUSH_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_FLUSH_STAGE_SMB_V2_DONE
} NFS_FLUSH_STAGE_SMB_V2;

typedef struct _NFS_FLUSH_STATE_SMB_V2
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    NFS_FLUSH_STAGE_SMB_V2    stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PSMB2_FID                 pFid; // Do not free

    PLWIO_NFS_FILE_2          pFile;

} NFS_FLUSH_STATE_SMB_V2, *PNFS_FLUSH_STATE_SMB_V2;

typedef enum
{
    NFS_READ_STAGE_SMB_V2_INITIAL = 0,
    NFS_READ_STAGE_SMB_V2_ATTEMPT_READ,
    NFS_READ_STAGE_SMB_V2_ATTEMPT_READ_COMPLETED,
    NFS_READ_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_READ_STAGE_SMB_V2_DONE
} NFS_READ_STAGE_SMB_V2;

typedef struct _NFS_READ_STATE_SMB_V2
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    NFS_READ_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PLWIO_NFS_FILE_2          pFile;

    LONG64                    llByteOffset;
    ULONG                     ulBytesRead;
    ULONG                     ulRemaining;

    PSMB2_READ_REQUEST_HEADER pRequestHeader; // Do not free

    PBYTE                     pData;
    ULONG                     ulKey;

} NFS_READ_STATE_SMB_V2, *PNFS_READ_STATE_SMB_V2;

typedef enum
{
    NFS_WRITE_STAGE_SMB_V2_INITIAL = 0,
    NFS_WRITE_STAGE_SMB_V2_ATTEMPT_WRITE,
    NFS_WRITE_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_WRITE_STAGE_SMB_V2_DONE
} NFS_WRITE_STAGE_SMB_V2;

typedef struct _NFS_WRITE_STATE_SMB_V2
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    NFS_WRITE_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PLWIO_NFS_FILE_2           pFile;

    PSMB2_WRITE_REQUEST_HEADER pRequestHeader; // Do not free
    PBYTE                      pData;          // Do not free

    ULONG                      ulBytesWritten;
    LONG64                     llDataOffset;
    ULONG                      ulKey;

} NFS_WRITE_STATE_SMB_V2, *PNFS_WRITE_STATE_SMB_V2;

typedef enum
{
    NFS_GET_INFO_STAGE_SMB_V2_INITIAL = 0,
    NFS_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO,
    NFS_GET_INFO_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_GET_INFO_STAGE_SMB_V2_DONE
} NFS_GET_INFO_STAGE_SMB_V2;

typedef struct _NFS_GET_INFO_STATE_SMB_V2
{
    LONG                          refCount;

    pthread_mutex_t               mutex;
    pthread_mutex_t*              pMutex;

    NFS_GET_INFO_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK               ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK        acb;
    PIO_ASYNC_CONTROL_BLOCK       pAcb;

    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader; // Do not free

    PLWIO_NFS_FILE_2              pFile;

    PBYTE                         pData2;
    ULONG                         ulDataLength;
    ULONG                         ulActualDataLength;

} NFS_GET_INFO_STATE_SMB_V2, *PNFS_GET_INFO_STATE_SMB_V2;

typedef enum
{
    NFS_SET_INFO_STAGE_SMB_V2_INITIAL = 0,
    NFS_SET_INFO_STAGE_SMB_V2_ATTEMPT_IO,
    NFS_SET_INFO_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_SET_INFO_STAGE_SMB_V2_DONE
} NFS_SET_INFO_STAGE_SMB_V2;

typedef struct _NFS_SET_INFO_STATE_SMB_V2
{
    LONG                          refCount;

    pthread_mutex_t               mutex;
    pthread_mutex_t*              pMutex;

    NFS_SET_INFO_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK               ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK        acb;
    PIO_ASYNC_CONTROL_BLOCK       pAcb;
    PIO_ECP_LIST                  pEcpList;

    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader; // Do not free
    PBYTE                         pData;          // Do not free

    PBYTE                         pSecurityDescriptor;
    PBYTE                         pSecurityQOS;
    IO_FILE_HANDLE                hDir;
    IO_FILE_NAME                  dirPath;

    PBYTE                         pData2;
    ULONG                         ulData2Length;

    PLWIO_NFS_FILE_2              pFile;
    PLWIO_NFS_FILE_2              pRootDir;

} NFS_SET_INFO_STATE_SMB_V2, *PNFS_SET_INFO_STATE_SMB_V2;

typedef enum
{
    NFS_IOCTL_STAGE_SMB_V2_INITIAL = 0,
    NFS_IOCTL_STAGE_SMB_V2_ATTEMPT_IO,
    NFS_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_IOCTL_STAGE_SMB_V2_DONE
} NFS_IOCTL_STAGE_SMB_V2;

typedef struct _NFS_IOCTL_STATE_SMB_V2
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    NFS_IOCTL_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader; // Do not free
    PBYTE                      pData;          // Do not free

    PLWIO_NFS_CONNECTION       pConnection;
    PLWIO_NFS_FILE_2           pFile;

    PBYTE                      pResponseBuffer;
    size_t                     sResponseBufferLen;
    ULONG                      ulResponseBufferLen;

} NFS_IOCTL_STATE_SMB_V2, *PNFS_IOCTL_STATE_SMB_V2;

typedef struct _NFS_NOTIFY_STATE_SMB_V2
{
    LONG                    refCount;

    pthread_mutex_t         mutex;
    pthread_mutex_t*        pMutex;

    ULONG64                 ullAsyncId;

    IO_STATUS_BLOCK         ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    ULONG                   ulCompletionFilter;
    BOOLEAN                 bWatchTree;

    PLWIO_NFS_CONNECTION    pConnection;

    USHORT                  usEpoch;
    ULONG64                 ullSessionId;
    ULONG                   ulTid;
    ULONG                   ulPid;
    ULONG64                 ullCommandSequence;

    PLWIO_NFS_FILE_2        pFile;

    PBYTE                   pBuffer;
    ULONG                   ulBufferLength;
    ULONG                   ulBytesUsed;

    ULONG                   ulMaxBufferSize;

} NFS_NOTIFY_STATE_SMB_V2, *PNFS_NOTIFY_STATE_SMB_V2;

typedef enum
{
    NFS_NOTIFY_STAGE_SMB_V2_INITIAL = 0,
    NFS_NOTIFY_STAGE_SMB_V2_ATTEMPT_IO,
    NFS_NOTIFY_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_NOTIFY_STAGE_SMB_V2_DONE
} NFS_NOTIFY_STAGE_SMB_V2;

typedef struct _NFS_NOTIFY_REQUEST_STATE_SMB_V2
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    NFS_NOTIFY_STAGE_SMB_V2    stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB2_NOTIFY_CHANGE_HEADER pRequestHeader; // Do not free

    PLWIO_NFS_FILE_2           pFile;

    ULONG64                    ullAsyncId;

    PBYTE                      pResponseBuffer;
    size_t                     sResponseBufferLen;
    ULONG                      ulResponseBufferLen;

} NFS_NOTIFY_REQUEST_STATE_SMB_V2, *PNFS_NOTIFY_REQUEST_STATE_SMB_V2;


typedef enum
{
    NFS_LOCK_STAGE_SMB_V2_INITIAL = 0,
    NFS_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK,
    NFS_LOCK_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_LOCK_STAGE_SMB_V2_DONE
} NFS_LOCK_STAGE_SMB_V2;

typedef struct _NFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    NFS_LOCK_STAGE_SMB_V2     stage;

    ULONG64                   ullAsyncId;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PSMB2_LOCK_REQUEST_HEADER pRequestHeader; // Do not free

    ULONG                     ulTid;
    PLWIO_NFS_FILE_2          pFile;

    PSMB2_LOCK*               ppUnlockArray;
    ULONG                     ulNumUnlocks;
    ULONG                     iUnlock;
    BOOLEAN                   bUnlockPending;

    PSMB2_LOCK*               ppLockArray;
    ULONG                     ulNumLocks;
    ULONG                     iLock;
    BOOLEAN                   bLockPending;

    BOOLEAN                   bFailImmediately;

} NFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2, *PNFS_ASYNC_LOCK_REQUEST_STATE_SMB_V2;

typedef struct _NFS_LOCK_REQUEST_STATE_SMB_V2
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    NFS_LOCK_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PSMB2_LOCK_REQUEST_HEADER pRequestHeader; // Do not free

    ULONG                     ulTid;

    PLWIO_NFS_FILE_2          pFile;

    PSMB2_LOCK*               ppUnlockArray;
    ULONG                     ulNumUnlocks;
    ULONG                     iUnlock;
    BOOLEAN                   bUnlockPending;

    PSMB2_LOCK*               ppLockArray;
    ULONG                     ulNumLocks;
    ULONG                     iLock;
    BOOLEAN                   bLockPending;

    BOOLEAN                   bFailImmediately;

    ULONG64                   ullAsyncId;

} NFS_LOCK_REQUEST_STATE_SMB_V2, *PNFS_LOCK_REQUEST_STATE_SMB_V2;

typedef enum
{
    NFS_CLOSE_STAGE_SMB_V2_INITIAL = 0,
    NFS_CLOSE_STAGE_SMB_V2_ATTEMPT_IO,
    NFS_CLOSE_STAGE_SMB_V2_BUILD_RESPONSE,
    NFS_CLOSE_STAGE_SMB_V2_DONE
} NFS_CLOSE_STAGE_SMB_V2;

typedef struct _NFS_CLOSE_STATE_SMB_V2
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    NFS_CLOSE_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB2_CLOSE_REQUEST_HEADER pRequestHeader; // Do not free

    FILE_BASIC_INFORMATION     fileBasicInfo;
    PFILE_BASIC_INFORMATION    pFileBasicInfo;

    FILE_STANDARD_INFORMATION  fileStdInfo;
    PFILE_STANDARD_INFORMATION pFileStdInfo;

    PLWIO_NFS_TREE_2           pTree;
    PLWIO_NFS_FILE_2           pFile;

} NFS_CLOSE_STATE_SMB_V2, *PNFS_CLOSE_STATE_SMB_V2;

typedef struct __NFS_MESSAGE_SMB_V2
{
    PBYTE        pBuffer;
    PSMB2_HEADER pHeader;
    ULONG        ulHeaderSize;
    ULONG        ulMessageSize;

    ULONG        ulBytesAvailable;

} NFS_MESSAGE_SMB_V2, *PNFS_MESSAGE_SMB_V2;

typedef struct _NFS_EXEC_CONTEXT_SMB_V2
{
    PNFS_MESSAGE_SMB_V2                  pRequests;
    ULONG                                ulNumRequests;
    ULONG                                iMsg;

    PLWIO_NFS_SESSION_2                  pSession;
    PLWIO_NFS_TREE_2                     pTree;
    PLWIO_NFS_FILE_2                     pFile;

    LONG                                 llNumSuccessfulCreates;
    NTSTATUS                             lastCloseStatus;

    HANDLE                               hState;
    PFN_NFS_MESSAGE_STATE_RELEASE_SMB_V2 pfnStateRelease;

    ULONG                                ulNumResponses;
    PNFS_MESSAGE_SMB_V2                  pResponses;

    PBYTE                                pErrorMessage;
    ULONG                                ulErrorMessageLength;

} NFS_EXEC_CONTEXT_SMB_V2;

typedef struct _NFS_RUNTIME_GLOBALS_SMB_V2
{
    pthread_mutex_t      mutex;

    PSMB_PROD_CONS_QUEUE pWorkQueue;

    ULONG                ulOplockTimeout;

} NFS_RUNTIME_GLOBALS_SMB_V2, *PNFS_RUNTIME_GLOBALS_SMB_V2;

#endif /* __STRUCTS_H__ */
