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
