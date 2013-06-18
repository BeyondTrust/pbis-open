/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Stream.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OpenSOAP_IMPL_Stream_H
#define OpenSOAP_IMPL_Stream_H

#include <OpenSOAP/Stream.h>

#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
    typedef
    int
    (*OpenSOAPStreamReadFunc)(OpenSOAPStreamPtr /* [in] */ s,
                              unsigned char * /* [out] */ buf,
                              size_t * /* [in, out] */ buf_sz);
    
    typedef
    int
    (*OpenSOAPStreamWriteFunc)(OpenSOAPStreamPtr /* [in] */ s,
                               const unsigned char * /* [in] */ buf,
                               size_t * /* [in, out] */ buf_sz);
    
    struct tagOpenSOAPStream {
        OpenSOAPObject  super;
        OpenSOAPStreamReadFunc  readFunc;
        OpenSOAPStreamWriteFunc writeFunc;
    };

    int
    OPENSOAP_API
    OpenSOAPStreamInitialize(OpenSOAPStreamPtr /* [in] */ s,
                             OpenSOAPObjectFreeFunc /* [in] */ free_func,
                             OpenSOAPStreamReadFunc  /* [in] */ read_func,
                             OpenSOAPStreamWriteFunc /* [in] */ write_func);
    
    int
    OPENSOAP_API
    OpenSOAPStreamReleaseMembers(OpenSOAPStreamPtr /* [in] */ s);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Stream_H */
