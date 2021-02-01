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
 * Module Name:
 *
 *        atomic.h
 *
 * Abstract:
 *
 *        Atomic operations
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWBASE_ATOMIC_H__
#define __LWBASE_ATOMIC_H__

#include <lw/types.h>
#include <lw/attrs.h>

LW_BEGIN_EXTERN_C

LW_LONG
LwInterlockedCompareExchange(
    LW_IN LW_OUT LW_LONG volatile *plDestination,
    LW_IN LW_LONG lNewValue,
    LW_IN LW_LONG lCompareValue
    );

LW_PVOID
LwInterlockedCompareExchangePointer(
    LW_IN LW_OUT LW_PVOID volatile *ppDestination,
    LW_IN LW_PVOID pNewPointer,
    LW_IN LW_PVOID pComparePointer
    );

LW_LONG
LwInterlockedRead(
    LW_IN LW_LONG volatile *plSource
    );

LW_LONG
LwInterlockedIncrement(
    LW_IN LW_OUT LW_LONG volatile* plDestination
    );

LW_LONG
LwInterlockedDecrement(
    LW_IN LW_OUT LW_LONG volatile* plDestination
    );

#ifndef LW_STRICT_NAMESPACE

#define InterlockedCompareExchange(Desination, NewValue, CompareValue) \
    LwInterlockedCompareExchange(Desination, NewValue, CompareValue)
#define InterlockedCompareExchangePointer(Destination, Exchange, Comparand) \
    LwInterlockedCompareExchangePointer(Destination, Exchange, Comparand)
#define InterlockedRead(Source)             LwInterlockedRead(Source)
#define InterlockedIncrement(Destination)   LwInterlockedIncrement(Destination)
#define InterlockedDecrement(Destination)   LwInterlockedDecrement(Destination)

#endif /* LW_STRICT_NAMESPACE */

LW_END_EXTERN_C

#endif
