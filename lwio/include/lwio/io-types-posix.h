/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
