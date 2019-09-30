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
 *        memstore_p.h
 *
 * Abstract:
 *        Registry memory provider backend private structures
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */
#ifndef _MEMSTORE_P_H_
#define _MEMSTORE_P_H_

#define MEMREG_TYPE_ROOT 1
#define MEMREG_TYPE_HIVE 2
#define MEMREG_TYPE_KEY 3
#define MEMREG_TYPE_VALUE 4

#define MEMREG_MAX_SUBNODES 512
#define MEMREG_MAX_SUBNODE_STACK (MEMREG_MAX_SUBNODES * 16)
#define MEMREG_MAX_VALUENAME_LEN 255

typedef struct _MEMREG_VALUE
{
    PWSTR Name;
    DWORD Type;
    PVOID Data;
    DWORD DataLen;
    LWREG_VALUE_ATTRIBUTES Attributes;
} MEMREG_VALUE, *PMEMREG_VALUE;


typedef struct _MEMREG_NODE_SD
{
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;
    ULONG SecurityDescriptorLen;
    BOOLEAN SecurityDescriptorAllocated;
} MEMREG_NODE_SD, *PMEMREG_NODE_SD;


typedef struct _MEMREG_NODE
{
    PWSTR Name;
    DWORD NodeType;

    /*
     * Needed to limit maximum number of subkeys
     */
    DWORD SubNodeDepth;

    /*
     * Key reference count. This is the number of open connections
     * to the current node (Name), not any SubNodes referenced in this node.
     */
    DWORD NodeRefCount;

    /*
     * Back reference to parent node. Needed for some operations (delete)
     */
    struct _MEMREG_NODE *ParentNode;
    
    PMEMREG_NODE_SD pNodeSd;

    struct _MEMREG_NODE **SubNodes;
    DWORD NodesLen;

    PMEMREG_VALUE *Values;
    DWORD ValuesLen;
} MEMREG_NODE;

#endif
