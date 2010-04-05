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
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Defines
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#define LWIO_DEFAULT_TIMEOUT_MSECS_SMB_V2 (30 * 1000)

#define COM2_NEGOTIATE_DESC       "SMB2_NEGOTIATE"
#define COM2_SESSION_SETUP_DESC   "SMB2_SESSION_SETUP"
#define COM2_LOGOFF_DESC          "SMB2_LOGOFF"
#define COM2_TREE_CONNECT_DESC    "SMB2_TREE_CONNECT"
#define COM2_TREE_DISCONNECT_DESC "SMB2_TREE_DISCONNECT"
#define COM2_CREATE_DESC          "SMB2_CREATE"
#define COM2_CLOSE_DESC           "SMB2_CLOSE"
#define COM2_FLUSH_DESC           "SMB2_FLUSH"
#define COM2_READ_DESC            "SMB2_READ"
#define COM2_WRITE_DESC           "SMB2_WRITE"
#define COM2_LOCK_DESC            "SMB2_LOCK"
#define COM2_IOCTL_DESC           "SMB2_IOCTL"
#define COM2_CANCEL_DESC          "SMB2_CANCEL"
#define COM2_ECHO_DESC            "SMB2_ECHO"
#define COM2_FIND_DESC            "SMB2_FIND"
#define COM2_NOTIFY_DESC          "SMB2_NOTIFY"
#define COM2_GETINFO_DESC         "SMB2_GETINFO"
#define COM2_SETINFO_DESC         "SMB2_SETINFO"
#define COM2_BREAK_DESC           "SMB2_BREAK"

#define SMB2_OPLOCK_LEVEL_BATCH 0x09
#define SMB2_OPLOCK_LEVEL_I     0x08
#define SMB2_OPLOCK_LEVEL_II    0x01
#define SMB2_OPLOCK_LEVEL_NONE  0x00

typedef USHORT LW_SMB2_OPLOCK_ACTION;

#define LW_SMB2_OPLOCK_ACTION_SEND_BREAK  0x0001
#define LW_SMB2_OPLOCK_ACTION_PROCESS_ACK 0x0002

#endif /* __DEFS_H__ */
