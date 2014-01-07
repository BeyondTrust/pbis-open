/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        affinity.h
 *
 * Abstract:
 *
 *        CPU affinity operations
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *          Evgeny Popovich (epopovich@likewise.com)
 *
 */

#ifndef __LWBASE_AFFINITY_H__
#define __LWBASE_AFFINITY_H__

#include <pthread.h>

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

ULONG
LwRtlGetCpuCount(
    VOID
    );

NTSTATUS
LwRtlSetAffinityThreadAttribute(
    pthread_attr_t* pAttr,
    ULONG CpuNumber
    );

NTSTATUS
LwRtlResetAffinityThreadAttribute(
    pthread_attr_t* pAttr
    );

#endif  // __LWBASE_AFFINITY_H__
