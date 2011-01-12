/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: CStdio.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OpenSOAP_IMPL_CStdio_H
#define OpenSOAP_IMPL_CStdio_H

#include <OpenSOAP/CStdio.h>
#include "Stream.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
    struct tagOpenSOAPCStdio {
        OpenSOAPStream  super;
        FILE    *inputStream;
        FILE    *outputStream;
    };
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_CStdio_H */
