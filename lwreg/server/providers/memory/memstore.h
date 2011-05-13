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
    OUT PMEMREG_NODE * phDb
    );


NTSTATUS
MemRegStoreClose(
    IN PMEMREG_NODE hDb
    );


NTSTATUS
MemRegStoreAddNode(
    IN PMEMREG_NODE hDb,
    PCWSTR Name,
    DWORD NodeType,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG SecurityDescriptorLen,
    OUT PMEMREG_NODE * phNode,
    OUT OPTIONAL PMEMREG_NODE * pRetNewNode
    );


NTSTATUS
MemRegStoreFindNode(
    IN PMEMREG_NODE hDb,
    IN PCWSTR Name,
    OUT PMEMREG_NODE * phNode
    );


NTSTATUS
MemRegStoreFindNodeSubkey(
    IN PMEMREG_NODE hDb,
    IN PCWSTR pwszSubKeyPath,
    OUT PMEMREG_NODE *pphNode
    );


NTSTATUS
MemRegStoreAddNodeValue(
    PMEMREG_NODE hDb,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );


NTSTATUS
MemRegStoreDeleteNodeValue(
    IN PMEMREG_NODE hDb,
    IN PCWSTR Name
    );


NTSTATUS
MemRegStoreFindNodeValue(
    IN PMEMREG_NODE hDb,
    IN PCWSTR Name,
    OUT PMEMREG_VALUE *pphValue
    );


NTSTATUS
MemRegStoreAddNodeAttribute(
    PMEMREG_VALUE hValue,
    IN PLWREG_VALUE_ATTRIBUTES pAttributes
    );


NTSTATUS
MemRegStoreGetNodeValueAttributes(
    PMEMREG_VALUE hValue,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes
    );


NTSTATUS
MemRegStoreChangeNodeValue(
    IN PMEMREG_VALUE pNodeValue,
    IN const BYTE *pData,
    DWORD cbData
    );


NTSTATUS
MemRegStoreDeleteNode(
    IN PMEMREG_NODE hDb
    );


NTSTATUS
MemRegStoreCreateNodeSdFromSddl(
    IN PSTR SecurityDescriptor,
    IN ULONG SecurityDescriptorLen,
    PMEMREG_NODE_SD *ppRetNodeSd
    );


NTSTATUS
MemRegStoreCreateSecurityDescriptor(
    PMEMREG_NODE_SD pNodeSd,
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG SecurityDescriptorLen,
    PMEMREG_NODE_SD *ppUpdatedNodeSd
    );
