/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: StringImpl.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_String_H
#define OpenSOAP_IMPL_String_H

#include <OpenSOAP/String.h>

#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    struct tagOpenSOAPString {
        OpenSOAPObject  super;
        size_t  length;
        wchar_t *stringEntity;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_String_H */
