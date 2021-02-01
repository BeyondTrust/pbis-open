/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
