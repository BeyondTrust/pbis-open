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

