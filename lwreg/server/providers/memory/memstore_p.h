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
 *        memstore_p.h
 *
 * Abstract:
 *        Registry memory provider backend private structures
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */
#ifndef _MEMSTORE_P_H_
#define _MEMSTORE_P_H_

#define REGMEM_TYPE_ROOT 1
#define REGMEM_TYPE_HIVE 2
#define REGMEM_TYPE_KEY 3
#define REGMEM_TYPE_VALUE 4

#define REGMEM_MAX_SUBNODES 512

typedef struct _REGMEM_VALUE
{
    PWSTR Name;
    DWORD Type;
    PVOID Data;
    DWORD DataLen;
    LWREG_VALUE_ATTRIBUTES Attributes;
} REGMEM_VALUE, *PREGMEM_VALUE;


typedef struct _REGMEM_NODE_SD
{
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;
    ULONG SecurityDescriptorLen;
    BOOLEAN SecurityDescriptorAllocated;
} REGMEM_NODE_SD, *PREGMEM_NODE_SD;


typedef struct _REGMEM_NODE
{
    PWSTR Name;
    DWORD NodeType;

    /*
     * Needed to limit maximum number of subkeys to 255
     */
    DWORD SubNodeDepth;

    /*
     * Back reference to parent node. Needed for some operations (delete)
     */
    struct _REGMEM_NODE *ParentNode;
    
    PREGMEM_NODE_SD pNodeSd;

    struct _REGMEM_NODE **SubNodes;
    DWORD NodesLen;

    PREGMEM_VALUE *Values;
    DWORD ValuesLen;
} REGMEM_NODE, *PREGMEM_NODE;

#endif
