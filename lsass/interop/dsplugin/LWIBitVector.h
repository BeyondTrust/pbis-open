/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    
 * All rights reserved.
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

