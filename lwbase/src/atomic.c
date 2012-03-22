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
 *        atomic.c
 *
 * Abstract:
 *
 *        Atomic operations
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

#if defined(LWBASE_ATOMIC_X86)

LONG
LwInterlockedCompareExchange(
    IN OUT LONG volatile *plDestination,
    IN LONG lNewValue,
    IN LONG lCompareValue
    )
{
    LONG lOldValue;

    /*
     * cmpxchg compares the contents of the destination
     * operand with the accumulator (eax in this case).
     * If they are equal, it sets the destination operand
     * to the source operand.  Otherwise, it sets the accumulator
     * to the destination operand.  The lock prefix ensures
     * this is done atomically.
     */
    __asm__ __volatile__ (
        "lock; cmpxchgl %2, %0"
        : "=m" (*plDestination), "=a" (lOldValue)
        : "r" (lNewValue), "1" (lCompareValue), "m" (*plDestination));

    return lOldValue;
}

PVOID
LwInterlockedCompareExchangePointer(
    IN OUT PVOID volatile *ppDestination,
    IN PVOID pNewPointer,
    IN PVOID pComparePointer
    )
{
    PVOID pOldPointer;

    /* Identical to the above, just the C types differ */
    __asm__ __volatile__ (
        "lock; cmpxchgl %2, %0"
        : "=m" (*ppDestination), "=a" (pOldPointer)
        : "r" (pNewPointer), "1" (pComparePointer), "m" (*ppDestination));

    return pOldPointer;
}

LONG
LwInterlockedRead(
    IN LONG volatile *plSource
    )
{
    /*
     * All 32-bit reads on x86 are atomic
     */
    return *plSource;
}

LONG
LwInterlockedIncrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lValue = 1;

    /* 
     * xadd will give us the old value of
     * the destination operand, so we need to
     * add 1 to yield the value after incrementation
     */
    __asm__ __volatile__ (
        "lock; xaddl %0, %1"
        : "=r" (lValue), "=m" (*plDestination)
        : "0" (lValue), "m" (*plDestination));

    return lValue + 1;
}

LONG
LwInterlockedDecrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lValue = -1;

    /* 
     * Same as increment but with -1 as the source operand
     */
    __asm__ __volatile__ (
        "lock; xaddl %0, %1"
        : "=r" (lValue), "=m" (*plDestination)
        : "0" (lValue), "m" (*plDestination));

    return lValue - 1;
}

#elif defined(LWBASE_ATOMIC_X86_64)

/*
 * All operations on LONG on x64_64 are identical to their
 * x86 counterparts
 */

LONG
LwInterlockedCompareExchange(
    IN OUT LONG volatile *plDestination,
    IN LONG lNewValue,
    IN LONG lCompareValue
    )
{
    LONG lOldValue;

    __asm__ __volatile__ (
        "lock; cmpxchgl %2, %0"
        : "=m" (*plDestination), "=a" (lOldValue)
        : "r" (lNewValue), "1" (lCompareValue), "m" (*plDestination));

    return lOldValue;
}

PVOID
LwInterlockedCompareExchangePointer(
    IN OUT PVOID volatile *ppDestination,
    IN PVOID pNewPointer,
    IN PVOID pComparePointer
    )
{
    PVOID pOldPointer;

    /* Identical to above except 64-bit */
    __asm__ __volatile__ (
        "lock; cmpxchgq %2, %0"
        : "=m" (*ppDestination), "=a" (pOldPointer)
        : "r" (pNewPointer), "1" (pComparePointer), "m" (*ppDestination));

    return pOldPointer;
}

LONG
LwInterlockedRead(
    IN LONG volatile *plSource
    )
{
    return *plSource;
}

LONG
LwInterlockedIncrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lValue = 1;

    __asm__ __volatile__ (
        "lock; xaddl %0, %1"
        : "=r" (lValue), "=m" (*plDestination)
        : "0" (lValue), "m" (*plDestination));

    return lValue + 1;
}

LONG
LwInterlockedDecrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lValue = -1;

    __asm__ __volatile__ (
        "lock; xaddl %0, %1"
        : "=r" (lValue), "=m" (*plDestination)
        : "0" (lValue), "m" (*plDestination));

    return lValue - 1;
}

#elif defined(LWBASE_ATOMIC_LOCK)

static pthread_mutex_t gAtomicMutex = PTHREAD_MUTEX_INITIALIZER;

LONG
LwInterlockedCompareExchange(
    IN OUT LONG volatile *plDestination,
    IN LONG lNewValue,
    IN LONG lCompareValue
    )
{
    LONG lOldValue;

    pthread_mutex_lock(&gAtomicMutex);

    lOldValue = *plDestination;

    if (lOldValue == lCompareValue)
    {
        *plDestination = lNewValue;
    }

    pthread_mutex_unlock(&gAtomicMutex);

    return lOldValue;
}

PVOID
LwInterlockedCompareExchangePointer(
    IN OUT PVOID volatile *ppDestination,
    IN PVOID pNewPointer,
    IN PVOID pComparePointer
    )
{
    PVOID pOldPointer;

    pthread_mutex_lock(&gAtomicMutex);

    pOldPointer = *ppDestination;

    if (pOldPointer == pComparePointer)
    {
        *ppDestination = pNewPointer;
    }

    pthread_mutex_unlock(&gAtomicMutex);

    return pOldPointer;
}

LONG
LwInterlockedRead(
    IN LONG volatile *plSource
    )
{
    LONG lValue;

    pthread_mutex_lock(&gAtomicMutex);

    lValue = *plSource;

    pthread_mutex_unlock(&gAtomicMutex);

    return lValue;
}

LONG
LwInterlockedIncrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lNewValue;

    pthread_mutex_lock(&gAtomicMutex);

    lNewValue = *plDestination += 1;

    pthread_mutex_unlock(&gAtomicMutex);

    return lNewValue;
}

LONG
LwInterlockedDecrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lNewValue;

    pthread_mutex_lock(&gAtomicMutex);

    lNewValue = *plDestination -= 1;

    pthread_mutex_unlock(&gAtomicMutex);

    return lNewValue;
}

#else
#error No atomic operations variant selected
#endif
