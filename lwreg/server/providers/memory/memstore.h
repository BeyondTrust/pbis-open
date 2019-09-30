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
