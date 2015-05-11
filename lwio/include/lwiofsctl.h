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
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#ifndef __LW_IO_FSCTL_H__
#define __LW_IO_FSCTL_H__

#include <lwio/lwio.h>
#include <lwio/lwiofsctl.h>

#define IO_FSCTL_SMB_GET_SESSION_KEY        0x01
#define IO_NPFS_FSCTL_CONNECT_NAMED_PIPE    0x02
#define IO_FSCTL_SMB_GET_PEER_ACCESS_TOKEN  0x03
#define IO_FSCTL_SMB_GET_PEER_ADDRESS       0x04

#define IO_FSCTL_PIPE_WAIT                  0x00110018
#define IO_FSCTL_PIPE_TRANSCEIVE            0x0011C017
#define IO_FSCTL_GET_DFS_REFERRALS          0x00060194



#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
