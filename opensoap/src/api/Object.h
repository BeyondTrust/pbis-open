/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Object.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_Object_H
#define OpenSOAP_IMPL_Object_H

#include <OpenSOAP/Defines.h>
#include <OpenSOAP/ErrorCode.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# if defined(WIN32)
#  include "win32config.h"
# endif
#endif /* HAVE_CONFIG_H */

#if defined(HAVE_PTHREAD_H) && defined(HAVE_LIBPTHREAD)
# include <pthread.h>
# define HAVE_PTHREAD
#endif /* HAVE_PTHREAD_H && HAVE_LIBPTHREAD */

#if defined(WIN32) && !defined(__GNUC__)
#define NONGCC_WIN32 1
#endif /* NONGCC Win32 Platform */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    typedef struct tagOpenSOAPObject OpenSOAPObject;
    typedef OpenSOAPObject *OpenSOAPObjectPtr;

    typedef
#ifdef HAVE_PTHREAD
    struct {
        long counter;
        pthread_mutex_t counterLock;
    }        
#else /* HAVE_PTHREAD */
    long
#endif /* !HAVE_PTHREAD */        
    OpenSOAPObjectRefCounter;
    typedef OpenSOAPObjectRefCounter *OpenSOAPObjectRefCounterPtr;
    
    typedef int (*OpenSOAPObjectFreeFunc)(OpenSOAPObjectPtr obj);
    
    struct tagOpenSOAPObject {
        OpenSOAPObjectRefCounter        refCounter;
        OpenSOAPObjectFreeFunc          freeFunction;
    };

    int
    OPENSOAP_API
    OpenSOAPObjectInitialize(OpenSOAPObjectPtr /* [in, out] */ obj,
                             OpenSOAPObjectFreeFunc /* [in] */ free_func);

    int
    OPENSOAP_API
    OpenSOAPObjectReleaseMembers(OpenSOAPObjectPtr /* [in, out] */ obj);

    int
    OPENSOAP_API
    OpenSOAPObjectRelease(OpenSOAPObjectPtr /* [in, out] */ obj);

    int
    OPENSOAP_API
    OpenSOAPObjectRetain(OpenSOAPObjectPtr /* [in, out] */ obj);

    int
    OPENSOAP_API
    OpenSOAPObjectGetRefCounter(OpenSOAPObjectPtr /* [in] */ obj,
                                long * /* [out] */ ref_counter);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Object_H */
