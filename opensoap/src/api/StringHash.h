/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: StringHash.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_StringHash_H
#define OpenSOAP_IMPL_StringHash_H

#include <OpenSOAP/StringHash.h>

#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct tagOpenSOAPStringHashPair OpenSOAPStringHashPair;
    typedef OpenSOAPStringHashPair *OpenSOAPStringHashPairPtr;

    struct tagOpenSOAPStringHashPair {
        OpenSOAPStringPtr       key;
        void *value;
    };

    struct tagOpenSOAPStringHash {
        OpenSOAPObject super;
        size_t  hashSize;
        size_t  hashAllocSize;
        OpenSOAPStringHashPairPtr hash;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_StringHash_H */
