/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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


#ifndef _DSRDEFS_H_
#define _DSRDEFS_H_

#include <lw/types.h>
#include <lwrpc/lsadefs.h>


#define DS_ROLE_STANDALONE_WORKSTATION         (0)
#define DS_ROLE_MEMBER_WORKSTATION             (1)
#define DS_ROLE_STANDALONE_SERVER              (2)
#define DS_ROLE_MEMBER_SERVER                  (3)
#define DS_ROLE_BACKUP_DC                      (4)
#define DS_ROLE_PRIMARY_DC                     (5)

#define DS_ROLE_PRIMARY_DS_RUNNING             (0x00000001)
#define DS_ROLE_PRIMARY_DS_MIXED_MODE          (0x00000002)
#define DS_ROLE_UPGRADE_IN_PROGRESS            (0x00000004)
#define DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT    (0x01000000)


typedef struct ds_role_primary_domain_info {
    DWORD   dwRole;
    DWORD   dwFlags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *pwszDomain;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *pwszDnsDomain;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *pwszForest;
    Guid   DomainGuid;
} DsRolePrimaryDomainInfoBasic, DS_ROLE_PRIMARY_DOMAIN_INFO_BASIC,
  *PDS_ROLE_PRIMARY_DOMAIN_INFO_BASIC;


#define DS_ROLE_NOT_UPGRADING                  (0)
#define DS_ROLE_UPGRADING                      (1)

#define DS_ROLE_PREVIOUS_UNKNOWN               (0)
#define DS_ROLE_PREVIOUS_PRIMARY               (1)
#define DS_ROLE_PREVIOUS_BACKUP                (2)

typedef struct ds_role_upgrade_status {
    WORD   swUpgradeStatus;
    DWORD  dwPrevious;
} DsRoleUpgradeStatus, DS_ROLE_UPGRADE_STATUS, *PDS_ROLE_UPGRADE_STATUS;


#define DS_ROLE_OP_IDLE                        (0)
#define DS_ROLE_OP_ACTIVE                      (1)
#define DS_ROLE_NEEDS_REBOOT                   (2)

typedef struct ds_role_op_status {
    WORD  swStatus;
} DsRoleOpStatus, DS_ROLE_OP_STATUS, *PDS_ROLE_OP_STATUS;


#define DS_ROLE_BASIC_INFORMATION              (1)
#define DS_ROLE_UPGRADE_STATUS                 (2)
#define DS_ROLE_OP_STATUS                      (3)

#ifndef _DCE_IDL_
typedef union ds_role_info {
    DsRolePrimaryDomainInfoBasic  basic;
    DsRoleUpgradeStatus           upgrade;
    DsRoleOpStatus                opstatus;
} DsRoleInfo, DS_ROLE_INFO, *PDS_ROLE_INFO;
#endif


#endif /* _DSRDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
