/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2010
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lmserver.h
 *
 * Abstract:
 *
 *        Likewise Network Management API
 *
 *        LanMan API (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LM_SERVER_H_
#define _LM_SERVER_H_

#include <lw/security-types.h>

#define SV_TYPE_WORKSTATION         (0x00000001)
#define SV_TYPE_SERVER              (0x00000002)
#define SV_TYPE_SQLSERVER           (0x00000004)
#define SV_TYPE_DOMAIN_CTRL         (0x00000008)
#define SV_TYPE_DOMAIN_BAKCTRL      (0x00000010)
#define SV_TYPE_TIME_SOURCE         (0x00000020)
#define SV_TYPE_AFP                 (0x00000040)
#define SV_TYPE_NOVELL              (0x00000080)
#define SV_TYPE_DOMAIN_MEMBER       (0x00000100)
#define SV_TYPE_PRINTQ_SERVER       (0x00000200)
#define SV_TYPE_DIALIN_SERVER       (0x00000400)
#define SV_TYPE_XENIX_SERVER        (0x00000800)
#define SV_TYPE_NT                  (0x00001000)
#define SV_TYPE_WFW                 (0x00002000)
#define SV_TYPE_SERVER_MFPN         (0x00004000)
#define SV_TYPE_SERVER_NT           (0x00008000)
#define SV_TYPE_POTENTIAL_BROWSER   (0x00010000)
#define SV_TYPE_BACKUP_BROWSER      (0x00020000)
#define SV_TYPE_MASTER_BROWSER      (0x00040000)
#define SV_TYPE_DOMAIN_MASTER       (0x00080000)
#define SV_TYPE_SERVER_OSF          (0x00100000)
#define SV_TYPE_SERVER_VMS          (0x00200000)
#define SV_TYPE_WIN95_PLUS          (0x00400000)
#define SV_TYPE_DFS_SERVER          (0x00800000)
#define SV_TYPE_ALTERNATE_XPORT     (0x20000000)
#define SV_TYPE_LOCAL_LIST_ONLY     (0x40000000)
#define SV_TYPE_DOMAIN_ENUM         (0x80000000)


NET_API_STATUS
NetServerEnum(
    PCWSTR   pwszHostname,
    DWORD    dwLevel,
    PVOID   *ppBuffer,
    DWORD    dwPrefMaxLen,
    PDWORD   pdwNumEntries,
    PDWORD   pdwTotalNumEntries,
    DWORD    dwServerType,
    PCWSTR   pwszDomain,
    PDWORD   pdwResume
    );


#endif /* _LM_SERVER_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
