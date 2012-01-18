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
 *        swab.h
 *
 * Abstract:
 *
 *        Safe Integer Arithmetic
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LWBASE_SAFEINT_H__
#define __LWBASE_SAFEINT_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

LW_BEGIN_EXTERN_C

#ifdef WIN32
#define inline __inline
#endif

inline
static
LW_NTSTATUS
LwRtlSafeMultiplyULONG(
    LW_OUT LW_PULONG Result,
    LW_IN LW_ULONG OperandA,
    LW_IN LW_ULONG OperandB
    )
{
    LW_NTSTATUS status = LW_STATUS_SUCCESS;
    LW_ULONG64 result = ((LW_ULONG64) OperandA) * ((LW_ULONG64) OperandB);
    if (result > ((LW_ULONG)-1))
    {
        status = LW_STATUS_INTEGER_OVERFLOW;
        *Result = ((LW_ULONG)-1);
    }
    else
    {
        status = LW_STATUS_SUCCESS;
    }   *Result = (LW_ULONG) result;
    return status;
}

inline
static
LW_NTSTATUS
LwRtlSafeAddULONG(
    LW_OUT LW_PULONG Result,
    LW_IN LW_ULONG OperandA,
    LW_IN LW_ULONG OperandB
    )
{
    LW_NTSTATUS status = LW_STATUS_SUCCESS;
    LW_ULONG result = OperandA + OperandB;
    if (result < OperandA)
    {
        status = LW_STATUS_INTEGER_OVERFLOW;
        *Result = ((LW_ULONG)-1);
    }
    else
    {
        status = LW_STATUS_SUCCESS;
    }   *Result = result;
    return status;
}

inline
static
LW_NTSTATUS
LwRtlSafeAddUSHORT(
    LW_OUT LW_PUSHORT Result,
    LW_IN LW_USHORT OperandA,
    LW_IN LW_USHORT OperandB
    )
{
    LW_NTSTATUS status = LW_STATUS_SUCCESS;
    LW_USHORT result = OperandA + OperandB;
    if (result < OperandA)
    {
        status = LW_STATUS_INTEGER_OVERFLOW;
        *Result = ((LW_USHORT)-1);
    }
    else
    {
        status = LW_STATUS_SUCCESS;
    }   *Result = result;
    return status;
}

#ifndef LW_STRICT_NAMESPACE
#define RtlSafeMultiplyULONG(Result, OperandA, OperandB)    LwRtlSafeMultiplyULONG(Result, OperandA, OperandB)
#define RtlSafeAddULONG(Result, OperandA, OperandB)         LwRtlSafeAddULONG(Result, OperandA, OperandB)
#define RtlSafeAddUSHORT(Result, OperandA, OperandB)        LwRtlSafeAddUSHORT(Result, OperandA, OperandB)
#endif /* LW_STRICT_NAMESPACE */

LW_END_EXTERN_C

#endif /* __LWBASE_SAFEINT_H__ */
