/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 * Abstract: Lsa interface (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _RPC_LSA_H_
#define _RPC_LSA_H_

#include <lwrpc/lsabinding.h>
#include <lwrpc/lsadefs.h>


NTSTATUS
LsaOpenPolicy2(
    IN  handle_t       hBinding,
    IN  PCWSTR         pwszSysname,
    IN  PVOID          attrib,
    IN  UINT32         AccessMask,
    OUT POLICY_HANDLE *phPolicy
    );


NTSTATUS
LsaClose(
    IN  handle_t hBinding,
    IN  void*    hObject
    );


NTSTATUS
LsaQueryInfoPolicy(
    IN  handle_t               hBinding,
    IN  POLICY_HANDLE          hPolicy,
    IN  UINT16                 Level,
    OUT LsaPolicyInformation **ppInfo
    );


NTSTATUS
LsaQueryInfoPolicy2(
    IN  handle_t               hBinding,
    IN  POLICY_HANDLE          hPolicy,
    IN  UINT16                 Level,
    OUT LsaPolicyInformation **ppInfo
    );


NTSTATUS
LsaLookupNames(
    IN  handle_t        hBinding,
    IN  POLICY_HANDLE   hPolicy,
    IN  UINT32          NumNames,
    IN  PWSTR          *ppwszNames,
    OUT RefDomainList **ppDomList,
    OUT TranslatedSid **ppSids,
    IN  UINT32          Level,
    IN OUT UINT32      *Count
    );


NTSTATUS
LsaLookupNames2(
    IN  handle_t         hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  UINT32           NumNames,
    IN  PWSTR           *ppNames,
    OUT RefDomainList  **ppDomList,
    OUT TranslatedSid2 **ppSids,
    IN  UINT16           Level,
    IN OUT UINT32       *Count
    );


NTSTATUS
LsaLookupNames3(
    IN  handle_t         hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  UINT32           NumNames,
    IN  PWSTR           *ppNames,
    OUT RefDomainList  **ppDomList,
    OUT TranslatedSid3 **ppSids,
    IN  UINT16           Level,
    IN OUT UINT32       *Count
    );


NTSTATUS
LsaLookupSids(
    IN  handle_t         hBinding,
    IN  POLICY_HANDLE    hPolicy,
    IN  SidArray        *pSids,
    OUT RefDomainList  **ppRefDomList,
    OUT TranslatedName **ppTransNames,
    IN  UINT16           Level,
    IN OUT UINT32       *Count
    );


NTSTATUS
LsaRpcInitMemory(
    VOID
    );


NTSTATUS
LsaRpcDestroyMemory(
    VOID
    );


VOID
LsaRpcFreeMemory(
    IN PVOID pPtr
    );


#endif /* _RPC_LSA_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
