/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwiofsctl.h
 *
 * Abstract:
 *
 *        Common FSCTL constants
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef __LW_IO_PUBLIC_FSCTL_H__
#define __LW_IO_PUBLIC_FSCTL_H__

//
// Type: FILE_DEVICE_FILE_SYSTEM
//

#define IO_FSCTL_SET_COMPRESSION                \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,           \
             16,                                \
             METHOD_BUFFERED,                   \
             FILE_READ_ACCESS|FILE_WRITE_ACCESS)

#define IO_FSCTL_GET_REPARSE_POINT              \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,           \
             42,                                \
             METHOD_BUFFERED,                   \
             FILE_ANY_ACCESS)

#define IO_FSCTL_CREATE_OR_GET_OBJECT_ID            \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,               \
             48,                                    \
             METHOD_BUFFERED,                       \
             FILE_ANY_ACCESS)

#define IO_FSCTL_SET_SPARSE              \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,    \
             49,                         \
             METHOD_BUFFERED,            \
             FILE_ANY_ACCESS)

#define IO_FSCTL_OPLOCK_REQUEST            \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,      \
             CUSTOM_CTL_FUNCTION(1),       \
             METHOD_NEITHER,               \
             FILE_ANY_ACCESS)

#define IO_FSCTL_OPLOCK_BREAK_ACK            \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,        \
             CUSTOM_CTL_FUNCTION(2),         \
             METHOD_NEITHER,                 \
             FILE_ANY_ACCESS)

#define IO_FSCTL_OPLOCK_BREAK_NOTIFY         \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,        \
             CUSTOM_CTL_FUNCTION(3),         \
             METHOD_NEITHER,                 \
             FILE_ANY_ACCESS)

//
// Type: FILE_DEVICE_NETWORK_FILE_SYSTEM
//

#define IO_FSCTL_ENUMERATE_SNAPSHOTS             \
    CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM,    \
             25,                                 \
             METHOD_BUFFERED,                    \
             FILE_READ_ACCESS)

typedef ULONG IO_LEASE_STATE, *PIO_LEASE_STATE; // Combination of R|W|H
//
// Lease state flags
//
#define IO_LEASE_STATE_READ               0x00000001
#define IO_LEASE_STATE_WRITE              0x00000002
#define IO_LEASE_STATE_HANDLE             0x00000004

#define IO_LEASE_STATE_NONE               0x00000000

typedef ULONG IO_OPLOCK_TYPE;
//
// Oplock IoFsControl Structures
//
#define IO_OPLOCK_REQUEST_OPLOCK_BATCH      0x01
#define IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1    0x02
#define IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2    0x03
#define IO_OPLOCK_REQUEST_LEASE             0x04


typedef struct _IO_FSCTL_REQUEST_OPLOCK_INPUT_BUFFER
{
    IO_OPLOCK_TYPE OplockRequestType;
    IO_LEASE_STATE RequestLeaseState;

} IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER, 
    *PIO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER;

// Oplock Request Output Buffer

typedef ULONG IO_OPLOCK_REQUEST_OUTPUT_TYPE;

#define IO_OPLOCK_NOT_BROKEN                 0x00000000
#define IO_OPLOCK_BROKEN_TO_NONE             0x00000001
#define IO_OPLOCK_BROKEN_TO_LEVEL_2          0x00000002
#define IO_OPLOCK_BROKEN_LEASE               0x00000003

typedef struct _IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER
{
    IO_OPLOCK_REQUEST_OUTPUT_TYPE OplockBreakResult;
    IO_LEASE_STATE CurrentLeaseState;
    IO_LEASE_STATE NewLeaseState;

} IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER, 
    *PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER;


typedef ULONG IO_OPLOCK_ACK_RESPONSE_TYPE;

// Oplock Break Acknowledge Input Buffer

#define IO_OPLOCK_BREAK_ACKNOWLEDGE         0x01
#define IO_OPLOCK_BREAK_ACK_NO_LEVEL_2      0x02
#define IO_OPLOCK_BREAK_CLOSE_PENDING       0x03
#define IO_OPLOCK_BREAK_ACK_LEASE           0x04

typedef struct _IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER
{
    IO_OPLOCK_ACK_RESPONSE_TYPE Response;
    IO_LEASE_STATE AckLeaseState;

} IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER, 
    *PIO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER;

// Oplock Break Acknowledge Output Buffer
// Accepts the same BreakResults as the OplockRequestOutputBuffer

typedef IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER 
            IO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER, 
            *PIO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER;



//
// Set Compression
// Only returns NTSTATUS (no output buffer)
//

#define IO_COMPRESSION_FORMAT_NONE         1
#define IO_COMPRESSION_FORMAT_DEFAULT      2
#define IO_COMPRESSION_FORMAT_LZNT1        3

typedef struct _IO_FSCTL_SET_COMPRESSION_INPUT_BUFFER
{
    SHORT CompressionFormat;

} IO_FSCTL_SET_COMPRESSION_INPUT_BUFFER, 
    *PIO_FSCTL_SET_COMPRESSION_INPUT_BUFFER;



//
// Set Sparse File
// Only returns NTSTATUS (no output buffer)
//

typedef struct _IO_FSCTL_SET_SPARSE_INPUT_BUFFER
{
    BOOLEAN SetSparse;

} IO_FSCTL_SET_SPARSE_INPUT_BUFFER, 
    *PIO_FSCTL_SET_SPARSE_INPUT_BUFFER;


//
// Get Reparse Point
// No input buffer
//

typedef struct _IO_REPARSE_DATA_BUFFER
{
    LONG ReparseTag;
    SHORT ReparseDataLength;
    SHORT Sbz1;
    BYTE  DataBuffer[1];

} IO_REPARSE_DATA_BUFFER, *PIO_REPARSE_DATA_BUFFER;

typedef struct _IO_REPARSE_GUID_DATA_BUFFER
{
    LONG ReparseTag;
    SHORT ReparseDataLength;
    SHORT Sbz1;
    LW_GUID ReparseGuid;
    BYTE  DataBuffer[1];

} IO_REPARSE_GUID_DATA_BUFFER, *PIO_REPARSE_GUID_DATA_BUFFER;

    
typedef struct _IO_FSCTL_GET_REPARSE_POINT_OUTPUT_BUFFER
{
    union
    {
        IO_REPARSE_DATA_BUFFER ReparseDataBuffer;
        IO_REPARSE_GUID_DATA_BUFFER ReparseGuidDataBuffer;

    } ReparseData;
    
} IO_FSCTL_GET_REPARSE_POINT_OUTPUT_BUFFER,
    *PIO_FSCTL_GET_REPARSE_POINT_OUTPUT_BUFFER;
    

//
// Create Or Get Object ID
// No Input buffer
//

typedef struct _IO_FILE_OBJECT_ID_BUFFER_TYPE_1
{
    LW_GUID ObjectId;
    LW_GUID BirthVolumeId;
    LW_GUID BirthObjectId;
    LW_GUID DomainId;

} IO_FILE_OBJECT_ID_BUFFER_TYPE_1, *PIO_FILE_OBJECT_ID_BUFFER_TYPE_1;


#define FILE_OBJECT_EXTENDED_INFO_LENGTH    48

typedef struct _IO_FILE_OBJECT_ID_BUFFER_TYPE_2
{
    LW_GUID ObjectId;
    BYTE ObjectExtendedInfo[FILE_OBJECT_EXTENDED_INFO_LENGTH];

} IO_FILE_OBJECT_ID_BUFFER_TYPE_2, *PIO_FILE_OBJECT_ID_BUFFER_TYPE_2;

    
typedef struct _IO_FSCTL_CREATE_OR_GET_OBJECT_ID_OUTPUT_BUFFER
{
    union
    {
        IO_FILE_OBJECT_ID_BUFFER_TYPE_1 ObjectIdBufferType1;
        IO_FILE_OBJECT_ID_BUFFER_TYPE_2 ObjectIdBufferType2;

    } ObjectIdBuffer;
    
} IO_FSCTL_CREATE_OR_GET_OBJECT_ID_OUTPUT_BUFFER,
    *PIO_FSCTL_CREATE_OR_GET_OBJECT_ID_OUTPUT_BUFFER;
    

//
// Shadow Copy (a.k.a. Snapshot)
// No input buffer
//

typedef struct _IO_FSCTL_ENUMERATE_SNAPSHOTS_OUTPUT_BUFFER
{
    ULONG ulNumberOfSnapshots;
    ULONG ulNumberOfSnapshotsReturned;
    ULONG ulSnapshotArraySize;
    WCHAR pwszSnapshotArray[1];
} IO_FSCTL_ENUMERATE_SNAPSHOTS_OUTPUT_BUFFER,
    *PIO_FSCTL_ENUMERATE_SNAPSHOTS_OUTPUT_BUFFER;

#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
