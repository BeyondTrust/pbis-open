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
 *        memstore.h
 *
 * Abstract:
 *        Registry memory provider data storage backend function prototypes
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */
#include "includes.h"
#include "memstore_p.h"

NTSTATUS
MemRegStoreOpen(
    OUT PMEM_REG_STORE_HANDLE phDb
    );


NTSTATUS
MemRegStoreClose(
    IN MEM_REG_STORE_HANDLE hDb
    );


NTSTATUS
MemRegStoreAddNode(
    IN MEM_REG_STORE_HANDLE hDb,
    PCWSTR Name,
    DWORD NodeType,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG SecurityDescriptorLen,
    OUT PMEM_REG_STORE_HANDLE phNode,
    OUT OPTIONAL PMEM_REG_STORE_HANDLE pRetNewNode
    );


NTSTATUS
MemRegStoreFindNode(
    IN MEM_REG_STORE_HANDLE hDb,
    IN PCWSTR Name,
    OUT PMEM_REG_STORE_HANDLE phNode
    );


NTSTATUS
MemRegStoreFindNodeSubkey(
    IN MEM_REG_STORE_HANDLE hDb,
    IN PCWSTR pwszSubKeyPath,
    OUT PMEM_REG_STORE_HANDLE phNode
    );


NTSTATUS
MemRegStoreAddNodeValue(
    MEM_REG_STORE_HANDLE hDb,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );


NTSTATUS
MemRegStoreDeleteNodeValue(
    IN MEM_REG_STORE_HANDLE hDb,
    IN PCWSTR Name
    );


NTSTATUS
MemRegStoreFindNodeValue(
    IN MEM_REG_STORE_HANDLE hDb,
    IN PCWSTR Name,
    OUT PREGMEM_VALUE *phValue
    );


NTSTATUS
MemRegStoreAddNodeAttribute(
    PREGMEM_VALUE hValue,
    IN PLWREG_VALUE_ATTRIBUTES pAttributes
    );


NTSTATUS
MemRegStoreGetNodeValueAttributes(
    PREGMEM_VALUE hValue,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );


NTSTATUS
MemRegStoreChangeNodeValue(
    IN PREGMEM_VALUE pNodeValue,
    IN const BYTE *pData,
    DWORD cbData
    );


NTSTATUS
MemRegStoreDeleteNode(
    IN MEM_REG_STORE_HANDLE hDb
    );


