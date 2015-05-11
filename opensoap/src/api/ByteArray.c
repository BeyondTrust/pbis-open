/*-----------------------------------------------------------------------------
 * $RCSfile: ByteArray.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: ByteArray.c,v 1.22 2003/11/20 07:03:17 okada Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/ByteArray.h>
#include <string.h>

#include "ByteArray.h"

/*
=begin
= OpenSOAP Byte Array class
=end
 */

static
const size_t
BYTEARRAYINITIALSIZE = 1024;

static
const size_t
BYTEARRAYGRAWSIZE    =  256;

/*
=begin
--- function#OpenSOAPByteArrayFree(obj)
    Byte array memory free.

    :Parameters
      :OpenSOAPObjectPtr [in] ((|obj|))
    :Return Value
      :int
        Return Code Is ErrorCode
    :Note
      This Function Is Static Function
=end
 */
static
int
OpenSOAPByteArrayFree(OpenSOAPObjectPtr obj) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    if (obj) {
        OpenSOAPByteArrayPtr b_ary = (OpenSOAPByteArrayPtr)obj;
        free(b_ary->array);
        ret = OpenSOAPObjectReleaseMembers(obj);
        if (OPENSOAP_SUCCEEDED(ret)) {
            free(b_ary);
        }
    }

    return ret;
}

#define OpenSOAPByteArrayGetReallocSize(sz) \
((sz) > BYTEARRAYINITIALSIZE) ? \
(((sz) - BYTEARRAYINITIALSIZE + BYTEARRAYGRAWSIZE - 1) \
   / BYTEARRAYGRAWSIZE * BYTEARRAYGRAWSIZE \
   + BYTEARRAYINITIALSIZE) : BYTEARRAYINITIALSIZE

/*
=begin
--- function#OpenSOAPByteArrayCreateWithAllocSize(sz, *b_ary)
    Create with allocate size.

    :Parameters
      :[in]  size_t ((|sz|))
        size.
      :[out] OpenSOAPByteArrayPtr * ((|b_ary|))
        byte array pointer.
    :Return Value
      :int
        Error code.
=end
 */
static
int
OpenSOAPByteArrayCreateWithAllocSize(size_t sz,
                                     OpenSOAPByteArrayPtr * /* [out] */ b_ary) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (sz && b_ary) {
        unsigned char *buf = NULL;
        *b_ary = malloc(sizeof(OpenSOAPByteArray));
        if (*b_ary) {
            buf = malloc(sz);
        }
        if (*b_ary && buf) {
            ret = OpenSOAPObjectInitialize((OpenSOAPObjectPtr)*b_ary,
                                           OpenSOAPByteArrayFree);
            if (OPENSOAP_SUCCEEDED(ret)) {
                (*b_ary)->arraySize = 0;
                (*b_ary)->allocSize = sz;
                (*b_ary)->array = buf;
            }
        }
        else {
            ret = OPENSOAP_MEM_BADALLOC;
        }
        if (OPENSOAP_FAILED(ret)) {
            free(buf);
            free(*b_ary);
            *b_ary = NULL;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayCreate(b_ary)
    Create a variable size Byte(unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr * [out] ((|b_ary|))
        ByteArray pointer
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayCreate(OpenSOAPByteArrayPtr * /* [out] */ b_ary) {
    return  OpenSOAPByteArrayCreateWithAllocSize(BYTEARRAYINITIALSIZE,
                                                   b_ary);
}

/*
=begin
--- function#OpenSOAPByteArrayCreateWithData(data, sz, b_ary)
    Create OpenSOAPByteArray with initial data

    :Parameters
      :const unsigned char * [in] ((|data|))
        Initial data. If NULL, allocate ((|sz|)) space, and 0 fill.
      :size_t [in] ((|sz|))
        Initial data size.
      :OpenSOAPByteArrayPtr * [out] ((|b_ary|))
        ByteArray pointer
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayCreateWithData(const unsigned char * /* [in] */ data,
                                size_t /* [in] */ sz,
                                OpenSOAPByteArrayPtr * /* [out] */ b_ary) {
    size_t alloc_sz = OpenSOAPByteArrayGetReallocSize(sz);
    int ret = OpenSOAPByteArrayCreateWithAllocSize(alloc_sz,
                                                   b_ary);

    if (OPENSOAP_SUCCEEDED(ret)) {
        if (sz) {
            if (data) {
                memcpy((*b_ary)->array, data, sz);
            }
            else {
                memset((*b_ary)->array, 0, sz);
            }
        }
        (*b_ary)->arraySize = sz;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayRetain(b_ary)
    retain Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayRetain(OpenSOAPByteArrayPtr /* [in] */ b_ary) {
    return OpenSOAPObjectRetain((OpenSOAPObjectPtr)b_ary);
}

/* release OpenSOAPByteArray Resource */
/*
=begin
--- function#OpenSOAPByteArrayRelease(b_ary)
    release Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayRelease(OpenSOAPByteArrayPtr /* [in] */ b_ary) {
    return OpenSOAPObjectRelease((OpenSOAPObjectPtr)b_ary);
}

/* get size */
/*
=begin
--- function#OpenSOAPByteArrayGetSize(b_ary, sz)
    Get size of variable size Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :size_t * [out] ((|sz|))
        pointer to the valiable storing the size
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayGetSize(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                         size_t * /* [out] */ sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && sz) {
        *sz = b_ary->arraySize;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/* get first element const pointer */
/*
=begin
--- function#OpenSOAPByteArrayBeginConst(b_ary, beg)
    Get const pointer to head of variable Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :const unsigned char ** [out] ((|beg|))
        Pointer to head of ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayBeginConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                            const unsigned char ** /* [out] */beg) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && beg) {
        *beg = b_ary->array;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/* get terminate element const pointer */
/*
=begin
--- function#OpenSOAPByteArrayEndConst(b_ary, ed)
    Get const pointer to tail of variable Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :const unsigned char ** [out] ((|ed|))
        Pointer to tail of ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayEndConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                          const unsigned char ** /* [out] */ed) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && ed) {
        *ed = b_ary->array + b_ary->arraySize;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayGetBeginSizeConst(b_ary, beg, sz)
    Get const pointer to head of variable Byte (unsigned char) Array and size

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :const unsigned char ** [out] ((|beg|))
        Pointer to head of ByteArray
      :size_t * [out] ((|sz|))
        Pointer to the variable storing the size
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayGetBeginSizeConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                                   const unsigned char ** /* [out] */ beg,
                                   size_t * /* [out] */ sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && beg && sz) {
        *beg = b_ary->array;
        *sz  = b_ary->arraySize;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=beginn
--- function#OpenSOAPByteArrayGetBeginEndConst(b_ary, beg, ed)
    Get const pointer to head and tail of variable Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :const unsigned char ** [out] ((|beg|))
        Pointer to head of ByteArray
      :const unsigned char ** [out] ((|beg|))
        Pointer to tail of ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayGetBeginEndConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                                  const unsigned char ** /* [out] */ beg,
                                  const unsigned char ** /* [out] */ ed) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && beg && ed) {
        *beg = b_ary->array;
        *ed  = b_ary->array + b_ary->arraySize;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/* get first element pointer */
/*
=begin
--- function#OpenSOAPByteArrayBegin(b_ary, beg)
    Get pointer to head of variable Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :unsigned char ** [out] ((|sz|))
        Pointer to head of ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayBegin(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                       unsigned char ** /* [out] */beg) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && beg) {
        *beg = b_ary->array;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/* get terminate element const pointer */
/*
=begin
--- function#OpenSOAPByteArrayEnd(b_ary, ed)
    Get pointer to tail of variable Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :unsigned char ** [out] ((|ed|))
        Pointer to tail of ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayEnd(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                     unsigned char ** /* [out] */ed) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && ed) {
        *ed = b_ary->array + b_ary->arraySize;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayGetBeginSize(b_ary, beg, sz)
    Get pointer to head of variable Byte (unsigned char) Array and size

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :unsigned char ** [out] ((|beg|))
        Pointer to head of ByteArray
      :size_t * [out] ((|sz|))
        Pointer to the variable storing the size
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayGetBeginSize(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                              unsigned char ** /* [out] */ beg,
                              size_t * /* [out] */ sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && beg && sz) {
        *beg = b_ary->array;
        *sz  = b_ary->arraySize;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayGetBeginEnd(b_ary, beg, ed)
    Get pointer to head and tail of variable Byte (unsigned char) Array

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :unsigned char ** [out] ((|beg|))
        Pointer to head of ByteArray
      :unsigned char ** [out] ((|ed|))
        Pointer to tail of ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayGetBeginEnd(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                             unsigned char ** /* [out] */ beg,
                             unsigned char ** /* [out] */ ed) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && beg && ed) {
        *beg = b_ary->array;
        *ed  = b_ary->array + b_ary->arraySize;

        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

/* clear OpenSOAPByteArray */
/*
=begin
--- function#OpenSOAPByteArrayClear(b_ary)
    Clear OpenSOAPByteArray

    :Parameters
      :OpenSOAPByteArrayPtr [in, out] ((|b_ary|))
        ByteArray
    :Return Value
      :int
        Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayClear(OpenSOAPByteArrayPtr /* [in, out] */ b_ary) {
    return OpenSOAPByteArraySetData(b_ary, NULL, 0);
}

/* clear OpenSOAPByteArray */
/*
=begin
--- function#OpenSOAPByteArraySetData(b_ary, data, sz)
    Set data of OpenSOAPByteArray

    :Parameters
      :OpenSOAPByteArrayPtr [in, out] ((|b_ary|))
        ByteArray
      :unsigned char * [in] ((|data|))
        data
      :size_t [in] ((|sz|))
        data size.

    :Return Value
      :int
        Return Code Is ErrorCode

=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArraySetData(OpenSOAPByteArrayPtr /* [in, out] */ b_ary,
                         const unsigned char * /* [in] */ data,
                         size_t /* [in] */ sz) {
    int ret = OpenSOAPByteArrayResize(b_ary, sz);

    if (OPENSOAP_SUCCEEDED(ret)) {
        if (data) {
            memcpy(b_ary->array, data, sz);
        }
        else {
            memset(b_ary->array, 0, sz);
        }
    }

    return ret;
}

/* Append Byte data to OpenSOAPByteArray */
/*
=begin
--- function#OpenSOAPByteArrayAppend(b_ary, dat, dat_len)
    Append Byte data to OpenSOAPByteArray

    :Parameters
      :OpenSOAPByteArrayPtr [in, out] ((|b_ary|))
        ByteArray
      :const unsigned char * [in] ((|dat|))
        Pointer to head of Append data. If NULL, 0 fill.
      :size_t [in] dat_sz
        Append data size

    :Return Value
      :int
        Return Code Is ErrorCode

=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayAppend(OpenSOAPByteArrayPtr /* [in, out] */ b_ary,
                        const unsigned char * /* [in] */ dat,
                        size_t       /* [in] */ dat_sz) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && dat_sz) {
        size_t new_size = b_ary->arraySize + dat_sz;
        size_t realloc_size = b_ary->allocSize;
        unsigned char *new_array = b_ary->array;
        if (new_size > realloc_size) {
            realloc_size = OpenSOAPByteArrayGetReallocSize(new_size);
            new_array = realloc(new_array, realloc_size);
        }
        if (new_array) {
            if (dat) {
                memcpy(new_array + b_ary->arraySize, dat, dat_sz);
            }
            else {
                memset(new_array + b_ary->arraySize, 0, dat_sz);
            }
            b_ary->arraySize = new_size;
            b_ary->allocSize = realloc_size;
            b_ary->array     = new_array;

            ret = OPENSOAP_NO_ERROR;
        }
        else {
            ret = OPENSOAP_MEM_BADALLOC;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayResize(b_ary, new_size)
    Resize OpenSOAPByteArray

    :Parameters
      :OpenSOAPByteArrayPtr [in, out] ((|b_ary|))
        ByteArray
      :size_t [in] ((|new_size|))
        New size. If the new size is greater than the current size,
        0 fill the extra area.

    :Return Value
      :int
        Return Code Is ErrorCode

=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayResize(OpenSOAPByteArrayPtr /* [in, out] */ b_ary,
                        size_t       /* [in] */ new_size) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary) {
        ret = OPENSOAP_NO_ERROR;
        if (new_size) {
            size_t old_size = b_ary->arraySize;
            if (old_size >= new_size) {
                b_ary->arraySize = new_size;
            }
            else {
                size_t realloc_size = b_ary->allocSize;
                unsigned char *new_array = b_ary->array;
                if (new_size > realloc_size) {
                    realloc_size = OpenSOAPByteArrayGetReallocSize(new_size);
                    new_array = realloc(new_array, realloc_size);
                }
                if (new_array) {
                    memset(new_array + old_size,
                           0, new_size - old_size);
                    b_ary->arraySize = new_size;
                    b_ary->allocSize = realloc_size;
                    b_ary->array     = new_array;
                }
                else {
                    ret = OPENSOAP_MEM_BADALLOC;
                }
            }
        }
        else {
            b_ary->arraySize = 0;
        }
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayDuplicate(b_ary, dup_b_ary)
    Duplicate OpenSOAPByteArray

    :Parameters
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        ByteArray
      :OpenSOAPByteArrayPtr * [out] ((|new_size|))
        Duplicate ByteArray

    :Return Value
      :int
        Return Code Is ErrorCode

=end
 */
extern
int
OPENSOAP_API
OpenSOAPByteArrayDuplicate(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                           OpenSOAPByteArrayPtr * /* [out] */ dup_b_ary) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (b_ary && dup_b_ary && (*dup_b_ary != b_ary)) {
        if (*dup_b_ary) {
            ret = OpenSOAPByteArraySetData(*dup_b_ary,
                                           b_ary->array,
                                           b_ary->arraySize);
        }
        else {
            ret = OpenSOAPByteArrayCreateWithData(b_ary->array,
                                                  b_ary->arraySize,
                                                  dup_b_ary);
        }
    }

    return ret;
}
