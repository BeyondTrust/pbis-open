/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        dssetup.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Directory Services Setup (DsSetup) library
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _DSSETUP_H_
#define _DSSETUP_H_


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


typedef struct _DSR_ROLE_PRIMARY_DOMAIN_INFO
{
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
    GUID   DomainGuid;
} DSR_ROLE_PRIMARY_DOMAIN_INFO_BASIC, *PDSR_ROLE_PRIMARY_DOMAIN_INFO_BASIC;


#define DS_ROLE_NOT_UPGRADING                  (0)
#define DS_ROLE_UPGRADING                      (1)

#define DS_ROLE_PREVIOUS_UNKNOWN               (0)
#define DS_ROLE_PREVIOUS_PRIMARY               (1)
#define DS_ROLE_PREVIOUS_BACKUP                (2)

typedef struct _DSR_ROLE_UPGRADE_STATUS
{
    WORD   swUpgradeStatus;
    DWORD  dwPrevious;
} DSR_ROLE_UPGRADE_STATUS, *PDSR_ROLE_UPGRADE_STATUS;


#define DS_ROLE_OP_IDLE                        (0)
#define DS_ROLE_OP_ACTIVE                      (1)
#define DS_ROLE_NEEDS_REBOOT                   (2)

typedef struct _DSR_ROLE_OP_STATUS
{
    WORD  swStatus;
} DSR_ROLE_OP_STATUS, *PDSR_ROLE_OP_STATUS;


#define DS_ROLE_BASIC_INFORMATION              (1)
#define DS_ROLE_UPGRADE_STATUS                 (2)
#define DS_ROLE_OP_STATUS                      (3)

#ifndef _DCE_IDL_
typedef union _DSR_ROLE_INFO
{
    DSR_ROLE_PRIMARY_DOMAIN_INFO_BASIC  Basic;
    DSR_ROLE_UPGRADE_STATUS             Upgrade;
    DSR_ROLE_OP_STATUS                  OpStatus;
} DSR_ROLE_INFO, *PDSR_ROLE_INFO;
#endif


#ifndef _DCE_IDL_

typedef
void* DSR_BINDING;

typedef
DSR_BINDING *PDSR_BINDING;


WINERROR
DsrInitBindingDefault(
    OUT PDSR_BINDING    phBinding,
    IN  PCWSTR          pwszHostname,
    IN  LW_PIO_CREDS    pCreds
    );


WINERROR
DsrInitBindingFull(
    OUT PDSR_BINDING    phBinding,
    IN  PCWSTR          pszProtSeq,
    IN  PCWSTR          pszHostname,
    IN  PCWSTR          pszEndpoint,
    IN  PCWSTR          pszUuid,
    IN  PCWSTR          pszOptions,
    IN  LW_PIO_CREDS    pCreds
    );


#define DsrInitBindingFromBindingString(binding_ptr, binding_str, creds_ptr) \
    RpcInitBindingFromBindingString((handle_t*)(binding_ptr),                \
                                    (binding_str),                           \
                                    (creds_ptr));


VOID
DsrFreeBinding(
    IN OUT PDSR_BINDING phBinding
    );


WINERROR
DsrRoleGetPrimaryDomainInformation(
    IN  DSR_BINDING   hBinding,
    IN  WORD          swLevel,
    PDSR_ROLE_INFO   *ppInfo
    );

NTSTATUS
DsrInitMemory(
    VOID
    );

NTSTATUS
DsrDestroyMemory(
    VOID
    );

VOID
DsrFreeMemory(
    IN PVOID pPtr
    );

#endif /* _DCE_IDL_ */

#endif /* _DSSETUP_H_ */



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
