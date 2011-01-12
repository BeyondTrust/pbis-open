/*-----------------------------------------------------------------------------
 * $RCSfile: Object.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Object.c,v 1.8 2003/11/20 07:03:18 okada Exp $";
#endif  /* _DEBUG */

#include "Object.h"

#include <OpenSOAP/ErrorCode.h>

#ifdef NONGCC_WIN32
#include <windows.h>
#endif /* NONGCC_WIN32 */

/*
=begin
= OpenSOAPObject
=end
 */

/*
=begin
== OpenSOAPObjectRefCounter
=end
 */
static
int
OpenSOAPObjectRefCounterInitialize(OpenSOAPObjectRefCounterPtr ref_counter) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ref_counter) {
#ifdef HAVE_PTHREAD
        pthread_mutex_init(&ref_counter->counterLock, NULL);
#endif /* HAVE_PTHREAD */
        *(long *)ref_counter = 1;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

static
int
OpenSOAPObjectRefCounterUltimate(OpenSOAPObjectRefCounterPtr ref_counter) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ref_counter) {
#ifdef HAVE_PTHREAD
        pthread_mutex_destroy(&ref_counter->counterLock);
#endif
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}


static
long
OpenSOAPObjectRefCounterIncrement(OpenSOAPObjectRefCounterPtr ref_counter) {
    long ret = 1;

    if (ref_counter) {
#ifdef HAVE_PTHREAD
        pthread_mutex_lock(&ref_counter->counterLock);
        ret = ++(ref_counter->counter);
        pthread_mutex_unlock(&ref_counter->counterLock);
#else /* HAVE_PTHREAD */
#if defined(NONGCC_WIN32)        
        ret = InterlockedIncrement(ref_counter);
#else /* NONGCC_WIN32 */        
        ret = ++(*ref_counter);
#endif        
#endif /* HAVE_PTHREAD */
    }

    return ret;
}

static
long
OpenSOAPObjectRefCounterDecrement(OpenSOAPObjectRefCounterPtr ref_counter) {
    long ret = -1;

    if (ref_counter) {
#ifdef HAVE_PTHREAD
        pthread_mutex_lock(&ref_counter->counterLock);
        ret = --(ref_counter->counter);
        pthread_mutex_unlock(&ref_counter->counterLock);
#else /* HAVE_PTHREAD */
#if defined(NONGCC_WIN32)        
        ret = InterlockedDecrement(ref_counter);
#else /* NONGCC_WIN32 */        
        ret = --(*ref_counter);
#endif        
#endif /* HAVE_PTHREAD */
    }

    return ret;
}

static
int
OpenSOAPObjectRefCounterGetCounter(OpenSOAPObjectRefCounterPtr ref_counter,
                                   long * /* [out] */ ret_p) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ref_counter && ret_p) {
#ifdef HAVE_PTHREAD
        pthread_mutex_lock(&ref_counter->counterLock);
        *ret_p = ref_counter->counter;
        pthread_mutex_unlock(&ref_counter->counterLock);
#else /* HAVE_PTHREAD */
        *ret_p = *ref_counter;
#endif /* HAVE_PTHREAD */
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
== OpenSOAPObject
=end
 */
/*
=begin
--- function#OpenSOAPObjectFreeDefault(obj)
    Default OpenSOAPObject free function.

    :Parameters
      :OpenSOAPObjectPtr [in, out] ((|obj|))
        OpenSOAPObject Pointer.

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPObjectFreeDefault(OpenSOAPObjectPtr obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (obj) {
        ret = OpenSOAPObjectReleaseMembers(obj);
        if (OPENSOAP_SUCCEEDED(ret)) {
/*            free(obj); */
            ret = OPENSOAP_NO_ERROR;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPObjectInitialize(obj, free_func)
    Initialize OpenSOAPObject

    :Parameters
      :OpenSOAPObjectPtr [in, out] ((|obj|))
        OpenSOAPObject Pointer.
      :OpenSOAPObjectFreeFunc [in] ((|free_func|))
        OpenSOAPObject free function's pointer.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPObjectInitialize(OpenSOAPObjectPtr /* [in, out] */ obj,
                         OpenSOAPObjectFreeFunc /* [in] */ free_func) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        obj->freeFunction = free_func;
        ret = OpenSOAPObjectRefCounterInitialize(&obj->refCounter);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPObjectReleaseMembers(obj)
    Release OpenSOAPObject's members.

    :Parameters
      :OpenSOAPObjectPtr [in, out] ((|obj|))
        OpenSOAPObject Pointer.

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPObjectReleaseMembers(OpenSOAPObjectPtr /* [in, out] */ obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        ret = OpenSOAPObjectRefCounterUltimate(&obj->refCounter);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPObjectRelease(obj)
    Release OpenSOAPObject.

    :Parameters
      :OpenSOAPObjectPtr [in, out] ((|obj|))
        OpenSOAPObject Pointer.

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPObjectRelease(OpenSOAPObjectPtr /* [in, out] */ obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        if (!OpenSOAPObjectRefCounterDecrement(&obj->refCounter)) {
            OpenSOAPObjectFreeFunc free_func
                = obj->freeFunction;
            if (!free_func) {
                free_func = OpenSOAPObjectFreeDefault;
            }
            ret = (free_func)(obj);
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPObjectRetain(obj)
    Retain OpenSOAPObject.

    :Parameters
      :OpenSOAPObjectPtr [in, out] ((|obj|))
        OpenSOAPObject Pointer.

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPObjectRetain(OpenSOAPObjectPtr /* [in, out] */ obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        OpenSOAPObjectRefCounterIncrement(&obj->refCounter);
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPObjectInitialize(node)
    Initialize Node

    :Parameters
      :OpenSOAPObjectPtr [in, out] ((|node|))
        XML Node Pointer.

    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPObjectGetRefCounter(OpenSOAPObjectPtr /* [in] */ obj,
                            long * /* [out] */ ref_counter) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        ret = OpenSOAPObjectRefCounterGetCounter(&obj->refCounter,
                                                 ref_counter);
    }

    return ret;
}
