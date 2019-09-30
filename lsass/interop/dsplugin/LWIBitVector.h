/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef __LWIBITVECTOR_H__
#define __LWIBITVECTOR_H__

#include "LWIPlugIn.h"

#define _LWI_BITVECTOR_VALID_POS(bitvector, pos) \
    (((pos) >= 0 && ((uint32_t)(pos)) < (bitvector)->nBits))

#define _LWI_BITVECTOR_WORD(bitvector, pos) \
    ((bitvector)->data[((uint32_t)(pos))/8])

#define _LWI_BITVECTOR_WORD_BIT(bitvector, pos) \
    (1 << (((uint32_t)(pos))%8))

#define LWI_BITVECTOR_SET(bitvector, pos) \
    do { \
        if (_LWI_BITVECTOR_VALID_POS(bitvector, pos)) { \
            _LWI_BITVECTOR_WORD(bitvector, pos) |= _LWI_BITVECTOR_WORD_BIT(bitvector, pos); \
        } \
    } while (0)

#define _LWI_BITVECTOR_UNSET(bitvector, pos)             \
    do { \
        if (_LWI_BITVECTOR_VALID_POS(bitvector, pos)) { \
            _LWI_BITVECTOR_WORD(bitvector, pos) &= ~(_LWI_BITVECTOR_WORD_BIT(bitvector, pos)); \
        } \
    } while (0)

#define _LWI_BITVECTOR_FLIP(bitvector, pos)              \
    do { \
        if (_LWI_BITVECTOR_VALID_POS(bitvector, pos)) { \
            _LWI_BITVECTOR_WORD(bitvector, pos) ^= _LWI_BITVECTOR_WORD_BIT(bitvector, pos); \
        } \
    } while (0)

#define LWI_BITVECTOR_ISSET(bitvector, pos)                             \
    ( _LWI_BITVECTOR_VALID_POS(bitvector, pos) ? ( (_LWI_BITVECTOR_WORD(bitvector, pos) &  _LWI_BITVECTOR_WORD_BIT(bitvector, pos)) != 0 ) : 0 )

#define LWI_BITVECTOR_SET_ALL(bitvector)                        \
    do {                                                           \
        int i = 0;                                              \
        for (i = 0; i < (((bitvector)->nBits-1)/8)+1; i++)        \
            (bitvector)->data[i] |= ~0;                           \
    } while (0)

#define LWI_BITVECTOR_RESET(bitvector)                          \
    do {                                                           \
        int i = 0;                                              \
        for (i = 0; i < (((bitvector)->nBits-1)/8)+1; i++)        \
            (bitvector)->data[i] &= 0;                            \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

long
LWIMakeBitVector(
    int nBits,
    PLWIBITVECTOR* ppBitVector
    );

void
LWIFreeBitVector(
    PLWIBITVECTOR pBitVector
    );

#ifdef __cplusplus
}
#endif

#endif /* __LWIBITVECTOR_H__ */

