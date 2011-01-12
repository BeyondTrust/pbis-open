/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ByteArray.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_ByteArray_H
#define OpenSOAP_IMPL_ByteArray_H

#include <OpenSOAP/ByteArray.h>

#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    struct tagOpenSOAPByteArray {
        OpenSOAPObject super;
        /* */
        size_t  arraySize;
        size_t  allocSize;
        unsigned char *array;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_ByteArray_H */
