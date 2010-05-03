/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lwzct.h
 *
 * @brief
 *
 *     ZCT (Zero Copy Transfer) API
 *
 * @defails
 *
 *     The ZCT (zero copy transfer) API supports an abstraction for
 *     performing I/O to/from file system drivers (FSDs) such that the
 *     FSD can pass internal buffers and/or file descriptors to the caller
 *     for reading/writing directly into the file system buffers/file
 *     descriptors.
 *
 * @author Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LW_ZCT_H__
#define __LW_ZCT_H__

#include <lw/types.h>
#include <lw/attrs.h>

///
/// ZCT Vector
///
/// The ZCT vector is an opaque type representing ZCT "buffers" and
/// tracking the location of the I/O within the vector.
///
typedef struct _LW_ZCT_VECTOR LW_ZCT_VECTOR, *PLW_ZCT_VECTOR;

///
/// Type of ZCT I/O.
///

typedef UCHAR LW_ZCT_IO_TYPE, *PLW_ZCT_IO_TYPE;

#define LW_ZCT_IO_TYPE_READ_SOCKET    1
#define LW_ZCT_IO_TYPE_WRITE_SOCKET   2

///
/// Type of ZCT buffers/entries
///
/// A ZCT vector can contain "buffers" of several types.
/// A single ZCT entry contains just one "buffer" type:
///
/// - memory (for use with readv/writev)
/// - file descriptor for a file (for use with sendfile)
/// - file descriptor for a pipe (for use with splice)
///

typedef UCHAR LW_ZCT_ENTRY_TYPE, *PLW_ZCT_ENTRY_TYPE;

#define LW_ZCT_ENTRY_TYPE_MEMORY    1
#define LW_ZCT_ENTRY_TYPE_FD_FILE   2
#define LW_ZCT_ENTRY_TYPE_FD_PIPE   3

//
// Mask of allowed ZCT buffer/entry types
//

typedef UCHAR LW_ZCT_ENTRY_MASK, *PLW_ZCT_ENTRY_MASK;

#define _LW_ZCT_ENTRY_MASK_FROM_TYPE(Type)  (1 << ((Type) - 1))

#define LW_ZCT_ENTRY_MASK_MEMORY    _LW_ZCT_ENTRY_MASK_FROM_TYPE(LW_ZCT_ENTRY_TYPE_MEMORY)
#define LW_ZCT_ENTRY_MASK_FD_FILE   _LW_ZCT_ENTRY_MASK_FROM_TYPE(LW_ZCT_ENTRY_TYPE_FD_FILE)
#define LW_ZCT_ENTRY_MASK_FD_PIPE   _LW_ZCT_ENTRY_MASK_FROM_TYPE(LW_ZCT_ENTRY_TYPE_FD_PIPE)

///
/// A ZCT buffer/entry
///
/// A ZCT buffer/entry represents a memory buffer or file/pipe descriptor.
///
typedef struct _LW_ZCT_ENTRY {
    /// Type of ZCT entry
    LW_ZCT_ENTRY_TYPE Type;
    /// Length of data represented by entry in bytes (e.g., size of the
    /// memory buffer or how much to read/write from/to the file
    /// descriptor).
    ULONG Length;
    union {
        /// LW_ZCT_ENTRY_TYPE_MEMORY
        struct {
            PVOID Buffer;
        } Memory;
        /// LW_ZCT_ENTRY_TYPE_FD_FILE
        struct {
            int Fd;
            LONG64 Offset;
        } FdFile;
        /// LW_ZCT_ENTRY_TYPE_FD_PIPE
        struct {
            int Fd;
        } FdPipe;
    } Data;
} LW_ZCT_ENTRY, *PLW_ZCT_ENTRY;

///
/// Create a ZCT vector.
///
/// @param[out] ppZct - Returns created ZCT vector.
/// @param[in] IoType - Type of I/O for transfer.
///
/// @retval STATUS_SUCCESS on success
/// @retval !NT_SUCCESS on failure
///
NTSTATUS
LwZctCreate(
    OUT PLW_ZCT_VECTOR* ppZct,
    IN LW_ZCT_IO_TYPE IoType
    );

///
/// Destroy a ZCT vector.
///
/// Free all resources used for tracking the ZCT buffers and file
/// descriptors.
///
/// @paaram[in,out] ppZct ZCT to destroy.  Set to NULL on output.
///
VOID
LwZctDestroy(
    IN OUT PLW_ZCT_VECTOR* ppZct
    );

///
/// Append entries to a ZCT vector.
///
/// @param[in] pZct - ZCT vector to modify.
///
/// @param[in] Entries - Array of entries to add.
///
/// @param[in] Count - Count of entries to add.
///
/// @retval STATUS_SUCCESS on success
/// @retval !NT_SUCCESS on failure
///
NTSTATUS
LwZctAppend(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count
    );

///
/// Prepend entries to a ZCT vector.
///
/// @param[in] pZct - ZCT vector to modify.
///
/// @param[in] Entries - Array of entries to add.
///
/// @param[in] Count - Count of entries to add.
///
/// @retval STATUS_SUCCESS on success
/// @retval !NT_SUCCESS on failure
///
NTSTATUS
LwZctPrepend(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count
    );

///
/// Get the total length represented by the ZCT vector.
///
/// @param[in] pZct - ZCT vector to query.
///
/// @return Length in bytes
///
ULONG
LwZctGetLength(
    IN PLW_ZCT_VECTOR pZct
    );

///
/// Get remaing transfer for the ZCT vector.
///
/// @param[in] pZct - ZCT vector to query.
///
/// @return bytes remaining
///
ULONG
LwZctGetRemaining(
    IN PLW_ZCT_VECTOR pZct
    );

///
/// Get the mask of ZCT buffer types supported in a ZCT vector.
///
/// @param[in] pZct - ZCT vector to query.
///
/// @return Appropriate #LW_ZCT_ENTRY_MASK 
///
LW_ZCT_ENTRY_MASK
LwZctGetSupportedMask(
    IN PLW_ZCT_VECTOR pZct
    );

///
/// Get supported ZCT buffer types based on I/O type.
///
/// Get supported ZCT buffer types based on I/O type.  If an invalid
/// I/O type is specified, a zero mask would be returned.
///
/// @param[in] IoType - Type of I/O
///
/// @return mask of supported ZCT buffer types on the system

LW_ZCT_ENTRY_MASK
LwZctGetSystemSupportedMask(
    IN LW_ZCT_IO_TYPE IoType
    );

///
/// Prepare ZCT vector for I/O.
///
/// After calling this, the ZCT vector can no longer be extended.
///
/// @param[in, out] pZct - ZCT vector to prepare for I/O.
///
NTSTATUS
LwZctPrepareIo(
    IN OUT PLW_ZCT_VECTOR pZct
    );

#if 0
NTSTATUS
LwZctPerform{Socket}Io(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN int {Socket}Fd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );
#endif

///
/// Read from socket into ZCT vector.
///
/// The ZCT vector must have been prepared with LwZctPrepareIo().
///
/// @param[in out] pZct - ZCT vector into which to read.
/// @param[in] pSocketFd - Socket from which to read.
/// @param[out] BytesTrasnferred - returns bytes read.
/// @param[out] BytesRemaining - returns bytes remaining to read.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_MORE_PROCESSING_REQUIRED
/// @retval !NT_SUCCESS
///
NTSTATUS
LwZctReadSocketIo(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );

///
/// Write into socket from ZCT vector.
///
/// The ZCT vector must have been prepared with LwZctPrepareIo().
///
/// @param[in out] pZct - ZCT vector from which to write.
/// @param[in] pSocketFd - Socket info which to write.
/// @param[out] BytesTrasnferred - returns bytes written.
/// @param[out] BytesRemaining - returns bytes remaining to write.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_MORE_PROCESSING_REQUIRED
/// @retval !NT_SUCCESS
///
NTSTATUS
LwZctWriteSocketIo(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );

///
/// Read from memory buffer into ZCT vector.
///
/// The ZCT vector must have been prepared with LwZctPrepareIo().
///
/// @param[in out] pZct - ZCT vector into which to read.
/// @param[in] pBuffer - Memory buffer from which to read.
/// @param[in] Length - Number of bytes to read.
/// @param[out] BytesTrasnferred - returns bytes read.
/// @param[out] BytesRemaining - returns bytes remaining to read.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_MORE_PROCESSING_REQUIRED
/// @retval !NT_SUCCESS
///
NTSTATUS
LwZctReadBufferIo(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PVOID pBuffer,
    IN ULONG Length,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );

#endif /* __LW_ZCT_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
