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

/* Oplock FsIoControl codes - possibly should be moved
   to a public header file at some point */

#define IO_FSCTL_OPLOCK_REQUEST             0x00000100
#define IO_FSCTL_OPLOCK_BREAK_ACK           0x00000101
#define IO_FSCTL_SET_SPARSE                 0x000900c4

/* Shadow copy FsIoControl codes. */
#define IO_FSCTL_NETWORK_FILESYSTEM	        0x00140000
#define IO_FSCTL_ACCESS_READ                0x00004000
#define IO_FSCTL_ENUMERATE_SNAPSHOTS        (IO_FSCTL_NETWORK_FILESYSTEM | IO_FSCTL_ACCESS_READ | 0x64)

/* Oplock Request Input Buffer */

#define IO_OPLOCK_REQUEST_OPLOCK_BATCH      0x01
#define IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1    0x02
#define IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2    0x03

typedef struct _IO_FSCTL_REQUEST_OPLOCK_INPUT_BUFFER
{
    ULONG OplockRequestType;

} IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER, 
    *PIO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER;

/* Oplock Request Output Buffer */

#define IO_OPLOCK_NOT_BROKEN                 0x00000000
#define IO_OPLOCK_BROKEN_TO_NONE             0x00000001
#define IO_OPLOCK_BROKEN_TO_LEVEL_2          0x00000002

typedef struct _IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER
{
    ULONG OplockBreakResult;

} IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER, 
    *PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER;


/* Oplock Break Acknowledge Input Buffer */

#define IO_OPLOCK_BREAK_ACKNOWLEDGE         0x01
#define IO_OPLOCK_BREAK_ACK_NO_LEVEL_2      0x02
#define IO_OPLOCK_BREAK_CLOSE_PENDING       0x03

typedef struct _IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER
{
    ULONG Response;

} IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER, 
    *PIO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER;

/* Oplock Break Acknowledge Output Buffer */
/* Accepts the same BreakResults as the OplockRequestOutputBuffer */

typedef IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER 
            IO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER, 
            *PIO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER;


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
