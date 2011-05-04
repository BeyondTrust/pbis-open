/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        io-types-posix.h
 *
 * Abstract:
 *
 *        IO Manager POSIX Types
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

// LW-specific info classes

#ifndef __IO_TYPES_POSIX_H__
#define __IO_TYPES_POSIX_H__

#include <lwio/io-types.h>


#define _LW_FILE_INFORMATION_CLASS_BASE   0x80000000

#define _MAKE_LW_FILE_INFORMATION_CLASS(Code) \
    (_LW_FILE_INFORMATION_CLASS_BASE | (Code))

#define LwFilePosixInformation            _MAKE_LW_FILE_INFORMATION_CLASS(1)
#define LwFilePosixDirectoryInformation   _MAKE_LW_FILE_INFORMATION_CLASS(2)
#define LwFileMaximumInformation          _MAKE_LW_FILE_INFORMATION_CLASS(3)


// QUERY: LwFilePosixInformation
typedef ULONG LW_UNIX_MODE, *PLW_UNIX_MODE;

typedef struct _LW_FILE_POSIX_INFORMATION {
    ULONG64 UnixAccessTime;         // Seconds since 1970
    ULONG64 UnixModificationTime;   // Seconds since 1970
    ULONG64 UnixChangeTime;         // Seconds since 1970
    FILE_ATTRIBUTES FileAttributes;
    ULONG64 EndOfFile;
    ULONG64 AllocationSize;
    ULONG UnixNumberOfLinks;
    LW_UNIX_MODE UnixMode;
    ULONG Uid;
    ULONG Gid;
    ULONG64 VolumeId;
    ULONG64 InodeNumber;
    ULONG64 GenerationNumber;
} LW_FILE_POSIX_INFORMATION, *PLW_FILE_POSIX_INFORMATION;

// DIR: LwFilePosixDirectoryInformation
typedef struct _LW_FILE_POSIX_DIR_INFORMATION {
    ULONG NextEntryOffset;
    LW_FILE_POSIX_INFORMATION PosixInformation;
    ULONG FileNameLength;
    CHAR FileName[1];
} LW_FILE_POSIX_DIR_INFORMATION, *PLW_FILE_POSIX_DIR_INFORMATION;

// SET: LwFilePosixInformation
typedef ULONG LW_FILE_POSIX_SETINFO_BITMASK, *PLW_FILE_POSIX_SETINFO_BITMASK;

#define LW_FILE_POSIX_SETINFO_MODE  0x01
#define LW_FILE_POSIX_SETINFO_UID   0x02
#define LW_FILE_POSIX_SETINFO_GID   0x04
#define LW_FILE_POSIX_SETINFO_SIZE  0x08
#define LW_FILE_POSIX_SETINFO_ATIME 0x10
#define LW_FILE_POSIX_SETINFO_MTIME 0x20

typedef struct _LW_FILE_POSIX_SET_INFORMATION {
   LW_FILE_POSIX_SETINFO_BITMASK Bitmask;
   LW_UNIX_MODE UnixMode;
   ULONG Uid;
   ULONG Gid;
   ULONG64 EndOfFile;
   ULONG64 UnixAccessTime;         // Seconds since 1970
   ULONG64 UnixModificationTime;   // Seconds since 1970
} LW_FILE_POSIX_SET_INFORMATION, *PLW_FILE_POSIX_SET_INFORMATION;


#endif  // __IO_TYPES_POSIX_H__
