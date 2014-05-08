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
 *        lwio/lwiodevctl.h
 *
 * Abstract:
 *
 *        Public Device Control codes and structures
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef __LW_IO_PUBLIC_DEVICECTL_H__
#define __LW_IO_PUBLIC_DEVICECTL_H__

/* Control Codes */

#define IO_DEVICE_TYPE_DISK_FILE_SYSTEM     0x00080000
#define IO_DEVICE_TYPE_NETWORK_FILE_SYSTEM  0x00140000

#define IO_DEVICE_REQ_ACCESS_READ_DATA      0x00010000
#define IO_DEVICE_REQ_ACCESS_WRITE_DATA     0x00020000

#define IO_DEVICE_CUSTOM_CONTROL_CODE       0x00002000

#define IO_DEVICE_FUNC_CODE_LIST_OPEN_FILES 0x00008010
#define IO_DEVICE_FUNC_CODE_STATISTICS      0x00008020

#define IO_DEVICE_FUNC_CODE_GET_DFS         0x00008030
#define IO_DEVICE_FUNC_CODE_SET_DFS         0x00008040

#define IO_DEVICE_TRANSFER_TYPE_BUFFERED    0x00000000
#define IO_DEVICE_TRANSFER_TYPE_IN_DIRECT   0x00000001
#define IO_DEVICE_TRANSFER_TYPE_OUT_DIRECT  0x00000002
#define IO_DEVICE_TRANSFER_TYPE_NEITHER     0x00000003

// TODO: Assign standard codes

#define SRV_DEVCTL_ADD_SHARE         1
#define SRV_DEVCTL_DELETE_SHARE      2
#define SRV_DEVCTL_ENUM_SHARE        3
#define SRV_DEVCTL_SET_SHARE_INFO    4
#define SRV_DEVCTL_GET_SHARE_INFO    5
#define SRV_DEVCTL_ENUM_SESSIONS     6
#define SRV_DEVCTL_DELETE_SESSION    7
#define SRV_DEVCTL_ENUM_FILES        8
#define SRV_DEVCTL_GET_FILE_INFO     9
#define SRV_DEVCTL_CLOSE_FILE       10
#define SRV_DEVCTL_ENUM_CONNECTION  11
#define SRV_DEVCTL_QUERY_TRANSPORT  12 // IN: NULL, OUT: BOOLEAN
#define SRV_DEVCTL_START_TRANSPORT  13 // IN: NULL, OUT: NULL
#define SRV_DEVCTL_STOP_TRANSPORT   14 // IN: BOOLEAN, OUT: NULL
#define SRV_DEVCTL_RELOAD_SHARES    15 // IN: NULL, OUT: NULL

#define RDR_DEVCTL_SET_DOMAIN_HINTS  1
#define RDR_DEVCTL_GET_PHYSICAL_PATH 2

#define IO_DEVICE_CTL_OPEN_FILE_INFO   ( IO_DEVICE_TYPE_DISK_FILE_SYSTEM     | \
                                         IO_DEVICE_REQ_ACCESS_READ_DATA      | \
                                         IO_DEVICE_CUSTOM_CONTROL_CODE       | \
                                         IO_DEVICE_FUNC_CODE_LIST_OPEN_FILES | \
                                         IO_DEVICE_TRANSFER_TYPE_NEITHER       \
                                       )

#define IO_DEVICE_CTL_STATISTICS ( IO_DEVICE_TYPE_NETWORK_FILE_SYSTEM | \
                                         IO_DEVICE_REQ_ACCESS_READ_DATA     | \
                                         IO_DEVICE_REQ_ACCESS_WRITE_DATA    | \
                                         IO_DEVICE_CUSTOM_CONTROL_CODE      | \
                                         IO_DEVICE_FUNC_CODE_STATISTICS     | \
                                         IO_DEVICE_TRANSFER_TYPE_NEITHER      \
                                       )

#define IO_DEVICE_CTL_GET_DFS_REFERRAL ( IO_DEVICE_TYPE_DISK_FILE_SYSTEM     | \
                                         IO_DEVICE_REQ_ACCESS_READ_DATA      | \
                                         IO_DEVICE_CUSTOM_CONTROL_CODE       | \
                                         IO_DEVICE_FUNC_CODE_GET_DFS         | \
                                         IO_DEVICE_TRANSFER_TYPE_NEITHER       \
                                       )

#define IO_DEVICE_CTL_SET_DFS_REFERRAL ( IO_DEVICE_TYPE_DISK_FILE_SYSTEM     | \
                                         IO_DEVICE_REQ_ACCESS_READ_DATA      | \
                                         IO_DEVICE_CUSTOM_CONTROL_CODE       | \
                                         IO_DEVICE_FUNC_CODE_SET_DFS         | \
                                         IO_DEVICE_TRANSFER_TYPE_NEITHER       \
                                       )


/* Device IoControl structures */

typedef struct _IO_OPEN_FILE_INFO_0
{
    ULONG NextEntryOffset;
    ULONG OpenHandleCount;
    ULONG FileNameLength;
    PWSTR pwszFileName[1];

} IO_OPEN_FILE_INFO_0, *PIO_OPEN_FILE_INFO_0;

typedef struct _IO_OPEN_FILE_INFO_100
{
    ULONG NextEntryOffset;
    ULONG OpenHandleCount;
    BOOLEAN bDeleteOnClose;
    ULONG FileNameLength;
    PWSTR pwszFileName[1];

} IO_OPEN_FILE_INFO_100, *PIO_OPEN_FILE_INFO_100;

typedef struct _IO_OPEN_FILE_INFO_INPUT_BUFFER
{
    DWORD Level;
    
} IO_OPEN_FILE_INFO_INPUT_BUFFER, *PIO_OPEN_FILE_INFO_INPUT_BUFFER;

typedef struct _IO_STATISTICS_INFO_0
{
    LONG64 llNumConnections;
    LONG64 llMaxNumConnections;

    LONG64 llNumSessions;
    LONG64 llMaxNumSessions;

    LONG64 llNumTreeConnects;
    LONG64 llMaxNumTreeConnects;

    LONG64 llNumOpenFiles;
    LONG64 llMaxNumOpenFiles;

} IO_STATISTICS_INFO_0, *PIO_STATISTICS_INFO_0;

typedef ULONG IO_STATISTICS_ACTION_TYPE;

#define IO_STATISTICS_ACTION_TYPE_GET   0x00000000
#define IO_STATISTICS_ACTION_TYPE_RESET 0x00000001

typedef struct _IO_STATISTICS_INFO_INPUT_BUFFER
{
    ULONG ulAction;
    ULONG ulInfoLevel;

} IO_STATISTICS_INFO_INPUT_BUFFER, *PIO_STATISTICS_INFO_INPUT_BUFFER;

#endif   /* __LW_IO_PUBLIC_DEVICECTL_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
