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
 *     lwzct.c
 *
 * @brief
 *
 *     ZCT (Zero Copy Transfer) API Implementation
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

// From Configure Tests
#undef HAVE_SPLICE
#undef HAVE_SENDFILE
#undef HAVE_SENDFILEV

// From Additional Determinations
#undef HAVE_SENDFILE_HEADER_TRAILER
#undef HAVE_SENDFILE_ANY

#include "config.h"
#include <lwio/lwzct.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <lw/safeint.h>
#include <lw/errno.h>
#include <assert.h>

//
// Notes about syscall interfaces:
//
// readv/writev - can do mem <-> socket/file/pipe
// splice - can do pipe <-> socket/file/pipe
// sendfilev - can do mem/file -> socket/file(/pipe?)
// sendfile (w/ht) - can do header + file + trailer -> socket
// sendfile - can do file -> socket
//

// For iovec support
#include <sys/uio.h>
#ifdef HAVE_SPLICE
// For splice support
#include <fcntl.h>
#endif
// For sendfile support
#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
// For read/write/pread/pwrite support
#include <unistd.h>

// Enable to pull in test definitions to help
// catch compilation issues for splice/sendfilev/etc.
// #include "fake-syscalls.h"

#ifdef HAVE_SENDFILE
#if defined(__FreeBSD__)
#define HAVE_SENDFILE_HEADER_TRAILER 1
#endif
#endif

#define LW_ZCT_ENTRY_CAPACITY_MINIMUM   4
#define LW_ZCT_ENTRY_CAPACITY_INCREMENT 2

typedef ULONG LW_ZCT_CURSOR_TYPE, *PLW_ZCT_CURSOR_TYPE;

#define LW_ZCT_CURSOR_TYPE_IOVEC                      1
#define LW_ZCT_CURSOR_TYPE_SPLICE                     2
#define LW_ZCT_CURSOR_TYPE_SENDFILE                   3

typedef struct _LW_ZCT_CURSOR_IOVEC {
    // Next starting Vector location is modified after
    // each transfer as needed.
    struct iovec* Vector;
    // Pass (Count - Index) into syscall.
    int Count;
    // Points to starting Vector location.  Updated
    // after each transfer as needed.
    int Index;
} LW_ZCT_CURSOR_IOVEC, *PLW_ZCT_CURSOR_IOVEC;

typedef struct _LW_ZCT_CURSOR_SPLICE {
    int FileDescriptor;
    // Length is updated after each transfer as needed.
    size_t Length;
} LW_ZCT_CURSOR_SPLICE, *PLW_ZCT_CURSOR_SPLICE;

//
// The ordering for the have sendfile checks is important here and
// elsewhere in the code:
//
// 1) sendfilev is preferred
// 2) sendfile w/ header/trailer is next
// 3) sendfile w/o header/trailer is last
//
#if defined(HAVE_SENDFILEV)
#define HAVE_SENDFILE_ANY 1
typedef struct _LW_ZCT_CURSOR_SENDFILE {
    // Next starting Vector location is modified after
    // each transfer as needed.
    sendfilevec_t* Vector;
    // Pass (Count - Index) into syscall.
    int Count;
    // Points to starting Vector location.  Updated
    // after each transfer as needed.
    int Index;
} LW_ZCT_CURSOR_SENDFILE, *PLW_ZCT_CURSOR_SENDFILE;
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
#define HAVE_SENDFILE_ANY 1
typedef struct _LW_ZCT_CURSOR_SENDFILE {
    LW_ZCT_CURSOR_IOVEC Header;
    LW_ZCT_CURSOR_IOVEC Trailer;
    int FileDescriptor;
    off_t Offset;
    size_t Length;
} LW_ZCT_CURSOR_SENDFILE, *PLW_ZCT_CURSOR_SENDFILE;
#elif defined(HAVE_SENDFILE)
#define HAVE_SENDFILE_ANY 1
typedef struct _LW_ZCT_CURSOR_SENDFILE {
    int FileDescriptor;
    // Offset and Length are updated after each transfer as needed.
    off_t Offset;
    size_t Length;
} LW_ZCT_CURSOR_SENDFILE, *PLW_ZCT_CURSOR_SENDFILE;
#endif

///
/// ZCT Cursor Entry
///
/// A cursor entry is updated as its data is transferred
/// such that the next transfer can continue from there
/// the previous transfer left off.  Depending on the entry
/// type, pointers, offsets, and sizes are updated.  For
/// entry types that include arrays, the index of where to
/// start in the array is updated.
///
typedef struct _LW_ZCT_CURSOR_ENTRY {
    /// This represent the location(s) in the Entries member
    /// of LW_ZCT_VECTOR that are encompassed by this cursor entry.
    /// This is strictly for debugging.
    struct {
        /// Index into the Entries member of LW_ZCT_VECTOR.
        ULONG Index;
        /// How many entries the current cursor entry contains
        /// starting from Index.  This must be >= 1.
        ULONG Count;
    } DebugExtent;
    /// Current type info.
    LW_ZCT_CURSOR_TYPE Type;
    union {
        LW_ZCT_CURSOR_IOVEC IoVec;
        LW_ZCT_CURSOR_SPLICE Splice;
#ifdef HAVE_SENDFILE_ANY
        LW_ZCT_CURSOR_SENDFILE SendFile;
#endif
    } Data;
} LW_ZCT_CURSOR_ENTRY, *PLW_ZCT_CURSOR_ENTRY;

///
/// ZCT Cursor
///
/// The ZCT cursor is allocated when the I/O is prepared
/// via IoZctPreprareIo().  It contains all the OS-level
/// structures to perform the entire I/O.
///
/// The allocated size is sufficient to contain all of
/// the data needed by the cursor, including vectors and
/// such.  This makes the allocation more efficient.
///
typedef struct _LW_ZCT_CURSOR {
    /// Total cursor allocation size.  This must be large enough to
    /// encompass all of the cursor entries.
    ULONG Size;
    /// Offset of struct iovec area
    ULONG IoVecOffset;
    /// Offset of free struct iovec area
    ULONG FreeIoVecOffset;
#ifdef HAVE_SENDFILEV
    /// Offset of sendfilevec_t area
    ULONG SendFileVecOffset;
    /// Offset of free sendfilevec_t area
    ULONG FreeSendFileVecOffset;
#endif
    /// Count of entries.
    ULONG Count;
    /// Index into cursor entries.  Whenever an entry is
    /// finished, the index is incremented.
    ULONG Index;
    /// Cursor entries.
    LW_ZCT_CURSOR_ENTRY Entry[1];
} LW_ZCT_CURSOR, *PLW_ZCT_CURSOR;

struct _LW_ZCT_VECTOR {
    LW_ZCT_IO_TYPE IoType;
    LW_ZCT_ENTRY_MASK Mask;
    PLW_ZCT_ENTRY Entries;
    ULONG Count;
    ULONG Capacity;
    /// Total size of transfer.
    ULONG Length;
    /// Track how much has been transferred so far
    /// so that it can be returned at the end of
    /// the I/O.
    ULONG BytesTransferred;
    /// Do not allow I/O to continue after a failure.
    NTSTATUS Status;
    /// When the cursor is allocated, the ZCT can
    /// no longer have entries added.
    PLW_ZCT_CURSOR Cursor;
};

typedef enum _ZCT_ENDPOINT_TYPE {
    ZCT_ENDPOINT_TYPE_UNINITIALZIED,
    ZCT_ENDPOINT_TYPE_SOCKET,
    ZCT_ENDPOINT_TYPE_BUFFER
} ZCT_ENDPOINT_TYPE, *PZCT_ENDPOINT_TYPE;

typedef struct _ZCT_ENDPOINT {
    ZCT_ENDPOINT_TYPE Type;
    union {
        int Socket;
        struct {
            PVOID pBuffer;
            ULONG Length;
        };
    };
} ZCT_ENDPOINT, *PZCT_ENDPOINT;

#define ZctEndpointSocketInit(pEndpoint, _FileDescriptor) \
    do { \
        (pEndpoint)->Type = ZCT_ENDPOINT_TYPE_SOCKET; \
        (pEndpoint)->Socket = (_FileDescriptor); \
    } while (0)

#define ZctEndpointBufferInit(pEndpoint, _pBuffer, _Length) \
    do { \
        (pEndpoint)->Type = ZCT_ENDPOINT_TYPE_BUFFER; \
        (pEndpoint)->pBuffer = (_pBuffer); \
        (pEndpoint)->Length = (_Length); \
    } while (0)

static
NTSTATUS
LwpZctReadWrite(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PZCT_ENDPOINT pEndpoint,
    IN BOOLEAN IsWrite,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );

static
NTSTATUS
LwpZctCursorEntryReadWriteSocket(
    IN int SocketFd,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_ENTRY pEntry,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDoneEntry
    );

static
NTSTATUS
LwpZctIoVecReadWrite(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_IOVEC Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );

#if defined(HAVE_SPLICE)
static
NTSTATUS
LwpZctSplice(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_SPLICE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );
#endif

#if defined(HAVE_SENDFILE_ANY)
static
NTSTATUS
LwpZctSendFile(
    IN int FileDescriptor,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );
#endif

static
NTSTATUS
LwpZctCursorEntryReadWriteBuffer(
    IN OUT PVOID pBuffer,
    IN ULONG Length,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_ENTRY pEntry,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDoneEntry
    );

static
NTSTATUS
LwpZctIoVecReadWriteBuffer(
    IN OUT PVOID pBuffer,
    IN ULONG Length,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_IOVEC Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );

#if defined(HAVE_SPLICE)
static
NTSTATUS
LwpZctSpliceBuffer(
    IN OUT PVOID pBuffer,
    IN ULONG Length,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_SPLICE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );
#endif

#if defined(HAVE_SENDFILE_ANY)
static
NTSTATUS
LwpZctSendFileBuffer(
    OUT PVOID pBuffer,
    IN ULONG Length,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );
#endif

NTSTATUS
LwZctCreate(
    OUT PLW_ZCT_VECTOR* ppZct,
    IN LW_ZCT_IO_TYPE IoType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    PLW_ZCT_VECTOR pZct = NULL;

    switch (IoType)
    {
    case LW_ZCT_IO_TYPE_READ_SOCKET:
    case LW_ZCT_IO_TYPE_WRITE_SOCKET:
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }
    
    status = RTL_ALLOCATE(&pZct, LW_ZCT_VECTOR, sizeof(*pZct));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pZct->Capacity = LW_ZCT_ENTRY_CAPACITY_MINIMUM;

    status = RTL_ALLOCATE(&pZct->Entries, LW_ZCT_ENTRY, sizeof(*pZct->Entries) * pZct->Capacity);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pZct->IoType = IoType;
    pZct->Mask = LwZctGetSystemSupportedMask(IoType);

cleanup:
    if (status)
    {
        LwZctDestroy(&pZct);
    }

    *ppZct = pZct;

    return status;
}

VOID
LwZctDestroy(
    IN OUT PLW_ZCT_VECTOR* ppZct
    )
{
    PLW_ZCT_VECTOR pZct = *ppZct;

    if (pZct)
    {
        RTL_FREE(&pZct->Cursor);
        RTL_FREE(&pZct->Entries);
        RtlMemoryFree(pZct);
        *ppZct = NULL;
    }
}

NTSTATUS
LwpZctCheckEntry(
    IN LW_ZCT_ENTRY_MASK Mask,
    IN PLW_ZCT_ENTRY Entry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;

    if (Entry->Length < 1)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (!IsSetFlag(Mask, _LW_ZCT_ENTRY_MASK_FROM_TYPE(Entry->Type)))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    switch (Entry->Type)
    {
        case LW_ZCT_ENTRY_TYPE_MEMORY:
            if (!Entry->Data.Memory.Buffer)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            if ((PVOID)LwRtlOffsetToPointer(Entry->Data.Memory.Buffer, Entry->Length) < Entry->Data.Memory.Buffer)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            break;

        case LW_ZCT_ENTRY_TYPE_FD_FILE:
            if (Entry->Data.FdFile.Fd < 0)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            break;

        case LW_ZCT_ENTRY_TYPE_FD_PIPE:
            if (Entry->Data.FdPipe.Fd < 0)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    return status;
}

static
NTSTATUS
LwpZctAdd(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN BOOLEAN bAddToFront,
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    ULONG i = 0;
    ULONG newCount = 0;
    PLW_ZCT_ENTRY pTarget = NULL;
    ULONG newLength = pZct->Length;

    if (pZct->Cursor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    for (i = 0; i < Count; i++)
    {
        status = LwpZctCheckEntry(pZct->Mask, &Entries[i]);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        status = LwRtlSafeAddULONG(&newLength, newLength, Entries[i].Length);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    newCount = pZct->Count + Count;
    if (pZct->Capacity < newCount)
    {
        PLW_ZCT_ENTRY pEntries = NULL;
        ULONG newCapacity = newCount + LW_ZCT_ENTRY_CAPACITY_INCREMENT;

        status = RTL_ALLOCATE(&pEntries, LW_ZCT_ENTRY, sizeof(pEntries[0]) * newCapacity);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        pTarget = pEntries;
        if (bAddToFront)
        {
            pTarget += Count;
        }

        LwRtlCopyMemory(pTarget, pZct->Entries, pZct->Count * sizeof(pZct->Entries[0]));

        RTL_FREE(&pZct->Entries);
        pZct->Entries = pEntries;
        pEntries = NULL;
        pZct->Capacity = newCapacity;
    }
    else if (bAddToFront)
    {
        LwRtlMoveMemory(pZct->Entries + Count, pZct->Entries, pZct->Count * sizeof(pZct->Entries[0]));
    }

    pTarget = pZct->Entries;
    if (!bAddToFront)
    {
        pTarget += pZct->Count;
    }
    LwRtlCopyMemory(pTarget, Entries, sizeof(Entries[0]) * Count);
    pZct->Count = newCount;
    pZct->Length = newLength;

cleanup:
    return status;
}

NTSTATUS
LwZctAppend(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    return LwpZctAdd(pZct, FALSE, Entries, Count);
}

NTSTATUS
LwZctPrepend(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    return LwpZctAdd(pZct, TRUE, Entries, Count);
}

ULONG
LwZctGetLength(
    IN PLW_ZCT_VECTOR pZct
    )
{
    return pZct->Length;
}

ULONG
LwZctGetRemaining(
    IN PLW_ZCT_VECTOR pZct
    )
{
    assert(pZct->BytesTransferred <= pZct->Length);
    return pZct->Length - pZct->BytesTransferred;
}

LW_ZCT_ENTRY_MASK
LwZctGetSupportedMask(
    IN PLW_ZCT_VECTOR pZct
    )
{
    return pZct->Mask;
}

LW_ZCT_ENTRY_MASK
LwZctGetSystemSupportedMask(
    IN LW_ZCT_IO_TYPE IoType
    )
{
    LW_ZCT_ENTRY_MASK mask = 0;

    switch (IoType)
    {
        case LW_ZCT_IO_TYPE_READ_SOCKET:
        case LW_ZCT_IO_TYPE_WRITE_SOCKET:
            SetFlag(mask, LW_ZCT_ENTRY_MASK_MEMORY);
#if defined(HAVE_SPLICE)
            SetFlag(mask, LW_ZCT_ENTRY_MASK_FD_PIPE);
#endif
#if defined(HAVE_SENDFILE_ANY)
            if (LW_ZCT_IO_TYPE_WRITE_SOCKET == IoType)
            {
                SetFlag(mask, LW_ZCT_ENTRY_MASK_FD_FILE);
            }
#endif
            break;
    }

    return mask;
}

static
NTSTATUS
LwpZctPrepareForSocketIo(
    IN OUT PLW_ZCT_VECTOR pZct
    );

NTSTATUS
LwZctPrepareIo(
    IN OUT PLW_ZCT_VECTOR pZct
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;

    if (pZct->Count < 1)
    {
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

    switch (pZct->IoType)
    {
    case LW_ZCT_IO_TYPE_READ_SOCKET:
    case LW_ZCT_IO_TYPE_WRITE_SOCKET:
        status = LwpZctPrepareForSocketIo(pZct);
        GOTO_CLEANUP_EE(EE);
        break;
    default:
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    return status;
}

static
ULONG
LwpZctCountRun(
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count,
    IN PLW_ZCT_ENTRY_TYPE AllowedEntryTypes,
    IN ULONG AllowedCount,
    IN OPTIONAL PLW_ZCT_ENTRY_TYPE RequiredType
    )
{
    ULONG count = 0;
    ULONG i = 0;
    BOOLEAN foundRequired = FALSE;

    for (i = 0; i < Count; i++)
    {
        PLW_ZCT_ENTRY pEntry = &Entries[i];
        ULONG allowedIndex = 0;
        BOOLEAN foundAllowed = FALSE;

        if (RequiredType &&
            !foundRequired &&
            ((*RequiredType) == pEntry->Type))
        {
            foundRequired = TRUE;
        }
        
        for (allowedIndex = 0; allowedIndex < AllowedCount; allowedIndex++)
        {
            if (AllowedEntryTypes[allowedIndex] == pEntry->Type)
            {
                foundAllowed = TRUE;
                break;
            }
        }

        if (!foundAllowed)
        {
            break;
        }

        count++;
    }

    if (RequiredType && !foundRequired)
    {
        count = 0;
    }

    return count;
}

static
ULONG
LwpZctCountRunMemory(
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    LW_ZCT_ENTRY_TYPE memoryAllowed[] = { LW_ZCT_ENTRY_TYPE_MEMORY };

    return LwpZctCountRun(
                Entries,
                Count,
                memoryAllowed,
                LW_ARRAY_SIZE(memoryAllowed),
                NULL);
}

#if defined(HAVE_SENDFILEV)
static
ULONG
LwpZctCountRunSendFileV(
    IN PLW_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    LW_ZCT_ENTRY_TYPE memoryFileAllowed[] = { LW_ZCT_ENTRY_TYPE_MEMORY, LW_ZCT_ENTRY_TYPE_FD_FILE };
    LW_ZCT_ENTRY_TYPE fileRequired[] = { LW_ZCT_ENTRY_TYPE_FD_FILE };

    return LwpZctCountRun(
                Entries,
                Count,
                memoryFileAllowed,
                LW_ARRAY_SIZE(memoryFileAllowed),
                fileRequired);
}
#endif

static
LW_ZCT_CURSOR_TYPE
LwpZctCountRangeForSocketIo(
    IN PLW_ZCT_VECTOR pZct,
    IN ULONG StartIndex,
    OUT PULONG Count
    )
{
    LW_ZCT_CURSOR_TYPE cursorType = 0;
    int EE ATTRIBUTE_UNUSED = 0;
    ULONG count = 0;
    PLW_ZCT_ENTRY pEntry = &pZct->Entries[StartIndex];

    if (StartIndex >= pZct->Count)
    {
        assert(FALSE);
        count = 0;
        cursorType = 0;
        GOTO_CLEANUP_EE(EE);
    }

#if defined(HAVE_SENDFILEV)
    count = LwpZctCountRunSendFileV(pEntry, pZct->Count - StartIndex);
    if (count > 0)
    {
        cursorType = LW_ZCT_CURSOR_TYPE_SENDFILE;
        GOTO_CLEANUP_EE(EE);
    }
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
    count = LwpZctCountRunMemory(pEntry, pZct->Count - StartIndex);
    if (((StartIndex + count) < pZct->Count) &&
        (LW_ZCT_ENTRY_TYPE_FD_FILE == pEntry[count].Type))
    {
        ULONG headerCount = count;

        count = LwpZctCountRunMemory(
                        &pEntry[count],
                        pZct->Count - (StartIndex + count));
        count = headerCount + 1 + count;
        cursorType = LW_ZCT_CURSOR_TYPE_SENDFILE;
        GOTO_CLEANUP_EE(EE);
    }
#elif defined(HAVE_SENDFILE)
    if (LW_ZCT_ENTRY_TYPE_FD_FILE == pEntry->Type)
    {
        count = 1;
        cursorType = LW_ZCT_CURSOR_TYPE_SENDFILE;
        GOTO_CLEANUP_EE(EE);
    }
#endif
    count = LwpZctCountRunMemory(pEntry, pZct->Count - StartIndex);
    if (count > 0)
    {
        cursorType = LW_ZCT_CURSOR_TYPE_IOVEC;
        GOTO_CLEANUP_EE(EE);
    }

    if (LW_ZCT_ENTRY_TYPE_FD_PIPE == pEntry->Type)
    {
        count = 1;
        cursorType = LW_ZCT_CURSOR_TYPE_SPLICE;
        GOTO_CLEANUP_EE(EE);
    }

    // Should never get here.
    assert(FALSE);

cleanup:
    *Count = count;

    return cursorType;
}

static
NTSTATUS
LwpZctCursorAllocateForSocketIo(
    IN PLW_ZCT_VECTOR pZct,
    OUT PLW_ZCT_CURSOR* ppCursor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    ULONG i = 0;
    ULONG cursorEntryCount = 0;
    ULONG cursorEntrySize = 0;
    ULONG ioVecCount = 0;
    ULONG ioVecSize = 0;
#if defined(HAVE_SENDFILEV)
    ULONG sendFileVecCount = 0;
    ULONG sendFileVecSize = 0;
#endif
    ULONG baseSize = LW_FIELD_OFFSET(LW_ZCT_CURSOR, Entry);
    ULONG size = 0;
    PLW_ZCT_CURSOR pCursor = NULL;

    while (i < pZct->Count)
    {
        LW_ZCT_CURSOR_TYPE cursorType = 0;
        ULONG count = 0;

        cursorType = LwpZctCountRangeForSocketIo(
                            pZct,
                            i,
                            &count);
        switch (cursorType)
        {
            case LW_ZCT_CURSOR_TYPE_IOVEC:
                cursorEntryCount++;
                assert(count > 0);
                ioVecCount += count;
                break;
            case LW_ZCT_CURSOR_TYPE_SPLICE:
                cursorEntryCount++;
                assert(1 == count);
                break;
#if defined(HAVE_SENDFILEV)
            case LW_ZCT_CURSOR_TYPE_SENDFILE:
                cursorEntryCount++;
                assert(count > 0);
                sendFileVecCount += count;
                break;
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
            case LW_ZCT_CURSOR_TYPE_SENDFILE:
                cursorEntryCount++;
                assert(count > 0);
                ioVecCount += count - 1;
                break;
#elif defined(HAVE_SENDFILE)
            case LW_ZCT_CURSOR_TYPE_SENDFILE:
                cursorEntryCount++;
                assert(1 == count);
                break;
#endif
            default:
                assert(FALSE);
                status = STATUS_INTERNAL_ERROR;
                GOTO_CLEANUP_EE(EE);
                break;
        }

        i += count;
    }

    size += baseSize;

    cursorEntrySize = cursorEntryCount * sizeof(LW_ZCT_CURSOR_ENTRY);
    size += cursorEntrySize;

    ioVecSize = ioVecCount * sizeof(struct iovec);
    size += ioVecSize;

#if defined(HAVE_SENDFILEV)
    sendFileVecSize = sendFileVecCount * sizeof(sendfilevec_t);
    size += sendFileVecSize;
#endif

    status = RTL_ALLOCATE(&pCursor, LW_ZCT_CURSOR, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCursor->Size = size;
    pCursor->IoVecOffset = baseSize + cursorEntrySize;
    pCursor->FreeIoVecOffset = pCursor->IoVecOffset;
#if defined(HAVE_SENDFILEV)
    pCursor->SendFileVecOffset = pCursor->IoVecOffset + ioVecSize;
    pCursor->FreeSendFileVecOffset = pCursor->SendFileVecOffset;
#endif
    pCursor->Count = cursorEntryCount;

cleanup:
    if (status)
    {
        RTL_FREE(&pCursor);
    }

    *ppCursor = pCursor;

    return status;
}

static
struct iovec*
LwpZctCursorAllocateIoVec(
    IN OUT PLW_ZCT_CURSOR pCursor,
    IN ULONG Count
    )
{
    struct iovec* pointer = (struct iovec*) LwRtlOffsetToPointer(pCursor, pCursor->FreeIoVecOffset);
    pCursor->FreeIoVecOffset += sizeof(struct iovec) * Count;
#if defined(HAVE_SENDFILEV)
    assert(pCursor->FreeIoVecOffset <= pCursor->SendFileVecOffset);
#else
    assert(pCursor->FreeIoVecOffset <= pCursor->Size);
#endif
    return pointer;
}

#ifdef HAVE_SENDFILEV
static
sendfilevec_t*
LwpZctCursorAllocateSendFileVec(
    IN OUT PLW_ZCT_CURSOR pCursor,
    IN ULONG Count
    )
{
    sendfilevec_t* pointer = (sendfilevec_t*) LwRtlOffsetToPointer(pCursor, pCursor->FreeSendFileVecOffset);
    pCursor->FreeSendFileVecOffset += sizeof(sendfilevec_t) * Count;
    assert(pCursor->FreeSendFileVecOffset <= pCursor->Size);
    return pointer;
}
#endif

static
VOID
LwpZctCursorInitiazeIoVecCursorEntry(
    IN OUT PLW_ZCT_CURSOR pCursor,
    OUT PLW_ZCT_CURSOR_IOVEC pIoVecCursor,
    IN PLW_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    ULONG i = 0;

    assert(Count > 0);

    pIoVecCursor->Vector = LwpZctCursorAllocateIoVec(pCursor, Count);
    pIoVecCursor->Count = Count;

    for (i = 0; i < Count; i++)
    {
        struct iovec* pVector = &pIoVecCursor->Vector[i];
        PLW_ZCT_ENTRY pEntry = &pEntries[i];

        assert(LW_ZCT_ENTRY_TYPE_MEMORY == pEntry->Type);

        pVector->iov_base = pEntry->Data.Memory.Buffer;
        pVector->iov_len = pEntry->Length;
    }
}

static
VOID
LwpZctCursorInitiazeSpliceCursorEntry(
    IN OUT PLW_ZCT_CURSOR pCursor,
    OUT PLW_ZCT_CURSOR_SPLICE pSpliceCursor,
    IN PLW_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    PLW_ZCT_ENTRY pEntry = &pEntries[0];

    assert(1 == Count);
    assert(LW_ZCT_ENTRY_TYPE_FD_PIPE == pEntry->Type);

    pSpliceCursor->FileDescriptor = pEntry->Data.FdPipe.Fd;
    pSpliceCursor->Length = pEntry->Length;
}

#if defined(HAVE_SENDFILEV)
static
VOID
LwpZctCursorInitiazeSendFileCursorEntry(
    IN OUT PLW_ZCT_CURSOR pCursor,
    OUT PLW_ZCT_CURSOR_SENDFILE pSendFileCursor,
    IN PLW_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    ULONG i = 0;

    assert(Count > 0);

    pSendFileCursor->Vector = LwpZctCursorAllocateSendFileVec(pCursor, Count);
    pSendFileCursor->Count = Count;

    for (i = 0; i < Count; i++)
    {
        sendfilevec_t* pVector = &pSendFileCursor->Vector[i];
        PLW_ZCT_ENTRY pEntry = &pEntries[i];

        pVector->sfv_len = pEntry->Length;
        pVector->sfv_flag = 0;

        switch (pEntry->Type)
        {
            case LW_ZCT_ENTRY_TYPE_FD_FILE:
                pVector->sfv_fd = pEntry->Data.FdFile.Fd;
                pVector->sfv_off = pEntry->Data.FdFile.Offset;
                break;
            case LW_ZCT_ENTRY_TYPE_MEMORY:
                pVector->sfv_fd = SFV_FD_SELF;
                pVector->sfv_off = (off_t) pEntry->Data.Memory.Buffer;
                break;
            default:
                assert(FALSE);
                break;
        }
    }
}
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
static
VOID
LwpZctCursorInitiazeSendFileCursorEntry(
    IN OUT PLW_ZCT_CURSOR pCursor,
    OUT PLW_ZCT_CURSOR_SENDFILE pSendFileCursor,
    IN PLW_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    PLW_ZCT_ENTRY pEntry = &pEntries[0];
    ULONG headerCount = 0;
    ULONG trailerCount = 0;

    assert(Count > 0);

    headerCount = LwpZctCountRunMemory(pEntry, Count);
    if (headerCount > 0)
    {
        LwpZctCursorInitiazeIoVecCursorEntry(
                pCursor,
                &pSendFileCursor->Header,
                pEntry,
                headerCount);
    }

    pEntry += headerCount;

    assert(LW_ZCT_ENTRY_TYPE_FD_FILE == pEntry->Type);

    pSendFileCursor->FileDescriptor = pEntry->Data.FdFile.Fd;
    pSendFileCursor->Offset = pEntry->Data.FdFile.Offset;
    pSendFileCursor->Length = pEntry->Length;

    pEntry += 1;

    trailerCount = Count - headerCount - 1;
    assert(trailerCount < Count);
    if (trailerCount > 0)
    {
        LwpZctCursorInitiazeIoVecCursorEntry(
                pCursor,
                &pSendFileCursor->Trailer,
                pEntry,
                trailerCount);
    }
}
#elif defined(HAVE_SENDFILE)
static
VOID
LwpZctCursorInitiazeSendFileCursorEntry(
    IN OUT PLW_ZCT_CURSOR pCursor,
    OUT PLW_ZCT_CURSOR_SENDFILE pSendFileCursor,
    IN PLW_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    PLW_ZCT_ENTRY pEntry = &pEntries[0];

    assert(1 == Count);

    pSendFileCursor->FileDescriptor = pEntry->Data.FdFile.Fd;
    pSendFileCursor->Offset = pEntry->Data.FdFile.Offset;
    pSendFileCursor->Length = pEntry->Length;
}
#endif

static
NTSTATUS
LwpZctCursorInitializeForSocketIo(
    IN PLW_ZCT_VECTOR pZct,
    IN OUT PLW_ZCT_CURSOR pCursor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    ULONG i = 0;
    ULONG cursorIndex = 0;

    while (i < pZct->Count)
    {
        PLW_ZCT_CURSOR_ENTRY pCursorEntry = &pCursor->Entry[cursorIndex];
        LW_ZCT_CURSOR_TYPE cursorType = 0;
        ULONG count = 0;

        assert(cursorIndex < pCursor->Count);

        cursorType = LwpZctCountRangeForSocketIo(
                            pZct,
                            i,
                            &count);

        pCursorEntry->Type = cursorType;
        pCursorEntry->DebugExtent.Index = i;
        pCursorEntry->DebugExtent.Count = count;

        switch (cursorType)
        {
            case LW_ZCT_CURSOR_TYPE_IOVEC:
                LwpZctCursorInitiazeIoVecCursorEntry(
                        pCursor,
                        &pCursorEntry->Data.IoVec,
                        &pZct->Entries[i],
                        count);
                break;

            case LW_ZCT_CURSOR_TYPE_SPLICE:
                LwpZctCursorInitiazeSpliceCursorEntry(
                        pCursor,
                        &pCursorEntry->Data.Splice,
                        &pZct->Entries[i],
                        count);
                break;
#if defined(HAVE_SENDFILE_ANY)
            case LW_ZCT_CURSOR_TYPE_SENDFILE:
                LwpZctCursorInitiazeSendFileCursorEntry(
                        pCursor,
                        &pCursorEntry->Data.SendFile,
                        &pZct->Entries[i],
                        count);
                break;
#endif
            default:
                assert(FALSE);
                status = STATUS_INTERNAL_ERROR;
                GOTO_CLEANUP_EE(EE);
                break;
        }

        i += count;
        cursorIndex++;
    }

cleanup:
    return status;
}

static
NTSTATUS
LwpZctPrepareForSocketIo(
    IN OUT PLW_ZCT_VECTOR pZct
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    PLW_ZCT_CURSOR pCursor = NULL;

    status = LwpZctCursorAllocateForSocketIo(pZct, &pCursor);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwpZctCursorInitializeForSocketIo(
                    pZct,
                    pCursor);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pZct->Cursor = pCursor;
    pCursor = NULL;

cleanup:
    RTL_FREE(&pCursor);

    return status;
}

NTSTATUS
LwZctReadSocketIo(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    )
{
    ZCT_ENDPOINT endpoint = { 0 };
    ZctEndpointSocketInit(&endpoint, SocketFd);
    return LwpZctReadWrite(
                pZct,
                &endpoint,
                FALSE,
                BytesTransferred,
                BytesRemaining);
}

NTSTATUS
LwZctWriteSocketIo(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    )
{
    ZCT_ENDPOINT endpoint = { 0 };
    ZctEndpointSocketInit(&endpoint, SocketFd);
    return LwpZctReadWrite(
                pZct,
                &endpoint,
                TRUE,
                BytesTransferred,
                BytesRemaining);
}

NTSTATUS
LwZctReadBufferIo(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PVOID pBuffer,
    IN ULONG Length,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    )
{
    ZCT_ENDPOINT endpoint = { 0 };
    ZctEndpointBufferInit(&endpoint, pBuffer, Length);
    return LwpZctReadWrite(
                pZct,
                &endpoint,
                FALSE,
                BytesTransferred,
                BytesRemaining);
}

static
NTSTATUS
LwpZctReadWrite(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PZCT_ENDPOINT pEndpoint,
    IN BOOLEAN IsWrite,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    ULONG totalBytesTransferred = 0;
    ULONG bytesRemaining = 0;
    LW_ZCT_IO_TYPE ioType = IsWrite ? LW_ZCT_IO_TYPE_WRITE_SOCKET : LW_ZCT_IO_TYPE_READ_SOCKET;

    if (!pZct->Cursor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (pZct->IoType != ioType)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (pZct->Status)
    {
        status = pZct->Status;
        GOTO_CLEANUP_EE(EE);
    }

    switch (pEndpoint->Type)
    {
    case ZCT_ENDPOINT_TYPE_SOCKET:
        if (pEndpoint->Socket < 0)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_EE(EE);
        }
        break;
    case ZCT_ENDPOINT_TYPE_BUFFER:
        if (pEndpoint->Length && !pEndpoint->pBuffer)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_EE(EE);
        }
        if (!pEndpoint->Length && !pEndpoint->pBuffer)
        {
            status = STATUS_SUCCESS;
            GOTO_CLEANUP_EE(EE);
        }
        break;
    default:
        assert(FALSE);
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP_EE(EE);
    }

    while (pZct->Cursor->Index < pZct->Cursor->Count)
    {
        ULONG bytesTransferred = 0;
        BOOLEAN isDoneEntry = FALSE;
        PLW_ZCT_CURSOR_ENTRY pEntry = &pZct->Cursor->Entry[pZct->Cursor->Index];

        switch (pEndpoint->Type)
        {
        case ZCT_ENDPOINT_TYPE_SOCKET:
            status = LwpZctCursorEntryReadWriteSocket(
                            pEndpoint->Socket,
                            IsWrite,
                            pEntry,
                            &bytesTransferred,
                            &isDoneEntry);
            break;
        case ZCT_ENDPOINT_TYPE_BUFFER:
            status = LwpZctCursorEntryReadWriteBuffer(
                            pEndpoint->pBuffer,
                            pEndpoint->Length,
                            IsWrite,
                            pEntry,
                            &bytesTransferred,
                            &isDoneEntry);
            assert(bytesTransferred <= pEndpoint->Length);
            pEndpoint->Length -= bytesTransferred;
            break;
        default:
            assert(FALSE);
            status = STATUS_ASSERTION_FAILURE;
            GOTO_CLEANUP_EE(EE);
        }
        // Handle blocking where we already got some data
        if ((STATUS_MORE_PROCESSING_REQUIRED == status) &&
            (totalBytesTransferred > 0))
        {
            status = STATUS_SUCCESS;
            break;
        }
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        totalBytesTransferred += bytesTransferred;
        if (isDoneEntry)
        {
            pZct->Cursor->Index++;
        }
        if ((ZCT_ENDPOINT_TYPE_BUFFER == pEndpoint->Type) &&
            !pEndpoint->Length)
        {
            break;
        }
    }

cleanup:
    if (status)
    {
        if (STATUS_MORE_PROCESSING_REQUIRED != status)
        {
            // Subsequent I/O should fail
            pZct->Status = status;
        }
        totalBytesTransferred = 0;
    }

    pZct->BytesTransferred += totalBytesTransferred;
    bytesRemaining = pZct->Length - pZct->BytesTransferred;

    if (BytesTransferred)
    {
        *BytesTransferred = totalBytesTransferred;
    }

    if (BytesRemaining)
    {
        *BytesRemaining = bytesRemaining;
    }

    return status;
}

static
NTSTATUS
LwpZctCursorEntryReadWriteSocket(
    IN int SocketFd,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_ENTRY pEntry,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDoneEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDoneEntry = FALSE;

    switch (pEntry->Type)
    {
    case LW_ZCT_CURSOR_TYPE_IOVEC:
    {
        status = LwpZctIoVecReadWrite(
                        SocketFd,
                        IsWrite,
                        &pEntry->Data.IoVec,
                        &bytesTransferred,
                        &isDoneEntry);
        break;
    }
#ifdef HAVE_SPLICE
    case LW_ZCT_CURSOR_TYPE_SPLICE:
        status = LwpZctSplice(
                        SocketFd,
                        IsWrite,
                        &pEntry->Data.Splice,
                        &bytesTransferred,
                        &isDoneEntry);
        break;
#endif
#ifdef HAVE_SENDFILE_ANY
    case LW_ZCT_CURSOR_TYPE_SENDFILE:
        assert(IsWrite);
        if (!IsWrite)
        {
            status = STATUS_INTERNAL_ERROR;
        }
        else
        {
            status = LwpZctSendFile(
                        SocketFd,
                        &pEntry->Data.SendFile,
                        &bytesTransferred,
                        &isDoneEntry);
        }
        break;
#endif
    default:
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDoneEntry = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDoneEntry = isDoneEntry;

    return status;
}

NTSTATUS
LwpZctIoVecReadWrite(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_IOVEC Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    struct iovec* vector = &Cursor->Vector[Cursor->Index];
    int count = Cursor->Count - Cursor->Index;
    ssize_t result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;
    int i = 0;

    if (IsWrite)
    {
        result = writev(FileDescriptor, vector, count);
    }
    else
    {
        result = readv(FileDescriptor, vector, count);
    }
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

    assert(result <= LW_MAXULONG);

    bytesTransferred = (ULONG) result;

    for (i = 0; i < count; i++)
    {
        if (result >= vector[i].iov_len)
        {
            // Note: Do not need to zero since we are moving on.
            // vector[i].iov_len = 0;
            result -= vector[i].iov_len;
            Cursor->Index++;

            if (0 == result)
            {
                break;
            }
        }
        else
        {
            vector[i].iov_base = LwRtlOffsetToPointer(vector[i].iov_base, result);
            vector[i].iov_len -= result;
            result -= result;
            break;
        }
        assert(result > 0);
    }

    assert(Cursor->Index <= Cursor->Count);

    if (Cursor->Index == Cursor->Count)
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}

#if defined(HAVE_SPLICE)
static
NTSTATUS
LwpZctSplice(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_SPLICE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    long result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;

    if (IsWrite)
    {
        result = splice(Cursor->FileDescriptor,
                        NULL,
                        FileDescriptor,
                        NULL,
                        Cursor->Length,
                        SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
    }
    else
    {
        result = splice(FileDescriptor,
                        NULL,
                        Cursor->FileDescriptor,
                        NULL,
                        Cursor->Length,
                        SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
    }
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }
    if (0 == result)
    {
        // TODO: Need to investigate semantics of this case.
    }

    assert(result <= Cursor->Length);

    bytesTransferred = (ULONG) result;

    if (bytesTransferred < Cursor->Length)
    {
        Cursor->Length -= bytesTransferred;
    }
    else
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}
#endif

#if defined(HAVE_SENDFILEV)
static
NTSTATUS
LwpZctSendFile(
    IN int FileDescriptor,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
#elif defined (HAVE_SENDFILE_HEADER_TRAILER)
static
NTSTATUS
LwpZctSendFile(
    IN int FileDescriptor,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
#elif defined (HAVE_SENDFILE)
static
NTSTATUS
LwpZctSendFile(
    IN int FileDescriptor,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ssize_t result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;

    result = sendfile(
                    FileDescriptor,
                    Cursor->FileDescriptor,
                    &Cursor->Offset,
                    Cursor->Length);
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

    assert(result <= Cursor->Length);

    bytesTransferred = (ULONG) result;

    if (bytesTransferred < Cursor->Length)
    {
        Cursor->Offset += bytesTransferred;
        Cursor->Length -= bytesTransferred;
    }
    else
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}
#endif

static
NTSTATUS
LwpZctCursorEntryReadWriteBuffer(
    IN OUT PVOID pBuffer,
    IN ULONG Length,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_ENTRY pEntry,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDoneEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDoneEntry = FALSE;

    switch (pEntry->Type)
    {
    case LW_ZCT_CURSOR_TYPE_IOVEC:
    {
        status = LwpZctIoVecReadWriteBuffer(
                        pBuffer,
                        Length,
                        IsWrite,
                        &pEntry->Data.IoVec,
                        &bytesTransferred,
                        &isDoneEntry);
        break;
    }
#ifdef HAVE_SPLICE
    case LW_ZCT_CURSOR_TYPE_SPLICE:
        status = LwpZctSpliceBuffer(
                        pBuffer,
                        Length,
                        IsWrite,
                        &pEntry->Data.Splice,
                        &bytesTransferred,
                        &isDoneEntry);
        break;
#endif
#ifdef HAVE_SENDFILE_ANY
    case LW_ZCT_CURSOR_TYPE_SENDFILE:
        assert(IsWrite);
        if (!IsWrite)
        {
            status = STATUS_INTERNAL_ERROR;
        }
        else
        {
            status = LwpZctSendFileBuffer(
                        pBuffer,
                        Length,
                        &pEntry->Data.SendFile,
                        &bytesTransferred,
                        &isDoneEntry);
        }
        break;
#endif
    default:
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDoneEntry = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDoneEntry = isDoneEntry;

    return status;
}

static
NTSTATUS
LwpZctIoVecReadWriteBuffer(
    IN OUT PVOID pBuffer,
    IN ULONG Length,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_IOVEC Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct iovec* vector = &Cursor->Vector[Cursor->Index];
    int count = Cursor->Count - Cursor->Index;
    ULONG remaining = Length;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;
    int i = 0;

    for (i = 0; i < count; i++)
    {
        if (remaining >= vector[i].iov_len)
        {
            if (IsWrite)
            {
                RtlCopyMemory(pBuffer, vector[i].iov_base, vector[i].iov_len);
            }
            else
            {
                RtlCopyMemory(vector[i].iov_base, pBuffer, vector[i].iov_len);
            }
            // Note: Do not need to zero since we are moving on.
            // vector[i].iov_len = 0;
            remaining -= vector[i].iov_len;
            Cursor->Index++;

            if (0 == remaining)
            {
                break;
            }
        }
        else
        {
            if (IsWrite)
            {
                RtlCopyMemory(pBuffer, vector[i].iov_base, remaining);
            }
            else
            {
                RtlCopyMemory(vector[i].iov_base, pBuffer, remaining);
            }
            vector[i].iov_base = LwRtlOffsetToPointer(vector[i].iov_base, remaining);
            vector[i].iov_len -= remaining;
            remaining -= remaining;
            break;
        }
        assert(remaining > 0);
    }

    bytesTransferred = Length - remaining;

    assert(Cursor->Index <= Cursor->Count);

    if (Cursor->Index == Cursor->Count)
    {
        isDone = TRUE;
    }

// cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}

#if defined(HAVE_SPLICE)
static
NTSTATUS
LwpZctSpliceBuffer(
    IN OUT PVOID pBuffer,
    IN ULONG Length,
    IN BOOLEAN IsWrite,
    IN OUT PLW_ZCT_CURSOR_SPLICE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    long result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;

    if (IsWrite)
    {
        result = read(Cursor->FileDescriptor,
                      pBuffer,
                      Length);
    }
    else
    {
        result = write(Cursor->FileDescriptor,
                       pBuffer,
                       Length);
    }
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }
    if (0 == result)
    {
        // TODO: Need to investigate semantics of this case.
    }

    assert(result <= Cursor->Length);

    bytesTransferred = (ULONG) result;

    if (bytesTransferred < Cursor->Length)
    {
        Cursor->Length -= bytesTransferred;
    }
    else
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}
#endif

#if defined(HAVE_SENDFILEV)
static
NTSTATUS
LwpZctSendFileBuffer(
    OUT PVOID pBuffer,
    IN ULONG Length,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
#elif defined (HAVE_SENDFILE_HEADER_TRAILER)
static
NTSTATUS
LwpZctSendFileBuffer(
    OUT PVOID pBuffer,
    IN ULONG Length,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
#elif defined (HAVE_SENDFILE)
static
NTSTATUS
LwpZctSendFileBuffer(
    OUT PVOID pBuffer,
    IN ULONG Length,
    IN OUT PLW_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ssize_t result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;

    result = pread(Cursor->FileDescriptor,
                   pBuffer,
                   Length,
                   Cursor->Offset);
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

    assert(result <= Cursor->Length);

    bytesTransferred = (ULONG) result;

    if (bytesTransferred < Cursor->Length)
    {
        Cursor->Offset += bytesTransferred;
        Cursor->Length -= bytesTransferred;
    }
    else
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}
#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
