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
#ifndef OpenSOAP_ByteArray_H
#define OpenSOAP_ByteArray_H

#include <stdlib.h>
#include <sys/types.h>

#include <OpenSOAP/Defines.h>
#include <OpenSOAP/ErrorCode.h>

/**
 * @file OpenSOAP/ByteArray.h
 * @brief OpenSOAP API ByteArray Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPByteArray OpenSOAPByteArray
     * @brief OpenSOAPByteArray Structure Type Definition
     */
    typedef struct tagOpenSOAPByteArray OpenSOAPByteArray;

    /**
     * @typedef OpenSOAPByteArray    *OpenSOAPByteArrayPtr
     * @brief OpenSOAPByteArray Pointer Type Definition
     */
    typedef OpenSOAPByteArray    *OpenSOAPByteArrayPtr;

    /**
      * @fn int OpenSOAPByteArrayCreate(OpenSOAPByteArrayPtr * b_ary)
      * @brief Create a variable size Byte(unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [out] ((|b_ary|)) ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayCreate(OpenSOAPByteArrayPtr * /* [out] */ b_ary);

    /**
      * @fn int OpenSOAPByteArrayCreateWithData(const unsigned char * data, size_t sz, OpenSOAPByteArrayPtr * b_ary)
      * @brief Create OpenSOAPByteArray with initial data
      * @param
      *    data const unsigned char * [in] ((|data|)) Initial data. If NULL, allocate ((|sz|)) space, and 0 fill.
      * @param
      *    sz size_t [in] ((|sz|)) Initial data size.
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [out] ((|b_ary|)) ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayCreateWithData(const unsigned char * /* [in] */ data,
                                    size_t /* [in] */ sz,
                                    OpenSOAPByteArrayPtr * /* [out] */ b_ary);

    /**
     * @def OpenSOAPByteArrayCreateWithSize(sz, b_ary) OpenSOAPByteArrayCreateWithData(0, (sz), (b_ary))
     * @brief OpenSOAPByteArrayCreateWithSize() Function Definition
     */
    #define OpenSOAPByteArrayCreateWithSize(sz, b_ary) OpenSOAPByteArrayCreateWithData(0, (sz), (b_ary))

    /**
      * @fn int OpenSOAPByteArrayAppendRef(OpenSOAPByteArrayPtr b_ary)
      * @brief Use OpenSOAPByteArray Resource declare
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayAppendRef(OpenSOAPByteArrayPtr /* [in] */ b_ary);

    /**
      * @fn int OpenSOAPByteArrayRelease(OpenSOAPByteArrayPtr b_ary)
      * @brief Release OpenSOAPByteArray Resource
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayRelease(OpenSOAPByteArrayPtr /* [in] */ b_ary);

    /**
      * @fn int OpenSOAPByteArrayGetSize(OpenSOAPByteArrayPtr b_ary, size_t * sz)
      * @brief Get size of variable size Byte (unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    sz size_t * [out] ((|sz|)) size
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayGetSize(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                             size_t * /* [out] */ sz);

    /**
      * @fn int OpenSOAPByteArrayBeginConst(OpenSOAPByteArrayPtr b_ary, const unsigned char ** beg)
      * @brief Get const pointer to head of variable Byte (unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    beg const unsigned char ** [out] ((|beg|)) Pointer to head of ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayBeginConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                                const unsigned char ** /* [out] */beg);
    
    /**
      * @fn int OpenSOAPByteArrayEndConst(OpenSOAPByteArrayPtr b_ary, const unsigned char ** ed)
      * @brief Get const pointer to tail of variable Byte (unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    ed const unsigned char ** [out] ((|ed|)) Pointer to tail of ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayEndConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                              const unsigned char ** /* [out] */ed);
    
    /**
      * @fn int OpenSOAPByteArrayGetBeginSizeConst(OpenSOAPByteArrayPtr b_ary, const unsigned char ** beg, size_t * sz)
      * @brief Get const pointer to head of variable Byte (unsigned char) Array and size
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    beg const unsigned char ** [out] ((|beg|)) Pointer to head of ByteArray
      * @param
      *    sz size_t * [out] ((|sz|)) size
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayGetBeginSizeConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                                       const unsigned char ** /* [out] */ beg,
                                       size_t * /* [out] */ sz);

    /**
      * @fn int OpenSOAPByteArrayGetBeginEndConst(OpenSOAPByteArrayPtr b_ary, const unsigned char ** beg, const unsigned char ** ed)
      * @brief Get const pointer to head and tail of variable Byte (unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    beg const unsigned char ** [out] ((|beg|)) Pointer to head of ByteArray
      * @param
      *    ed const unsigned char ** [out] ((|ed|)) Pointer to tail of ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayGetBeginEndConst(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                                      const unsigned char ** /* [out] */ beg,
                                      const unsigned char ** /* [out] */ ed);
    
    /**
      * @fn int OpenSOAPByteArrayBegin(OpenSOAPByteArrayPtr b_ary, unsigned char ** beg)
      * @brief Get pointer to head of variable Byte (unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    beg unsigned char ** [out] ((|beg|)) Pointer to head of ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayBegin(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                           unsigned char ** /* [out] */beg);
    
    /**
      * @fn int OpenSOAPByteArrayEnd(OpenSOAPByteArrayPtr b_ary, unsigned char ** ed)
      * @brief Get pointer to tail of variable Byte (unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    ed unsigned char ** [out] ((|ed|)) Pointer to tail of ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayEnd(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                         unsigned char ** /* [out] */ed);

    /**
      * @fn int OpenSOAPByteArrayGetBeginSize(OpenSOAPByteArrayPtr b_ary, unsigned char ** beg, size_t * sz)
      * @brief Get pointer to head of variable Byte (unsigned char) Array and size
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    beg unsigned char ** [out] ((|beg|)) Pointer to head of ByteArray
      * @param
      *    sz size_t * [out] ((|sz|)) size
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayGetBeginSize(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                                  unsigned char ** /* [out] */ beg,
                                  size_t * /* [out] */ sz);

    /**
      * @fn int OpenSOAPByteArrayGetBeginEnd(OpenSOAPByteArrayPtr b_ary, unsigned char ** beg, unsigned char ** ed)
      * @brief Get pointer to head and tail of variable Byte (unsigned char) Array
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    beg unsigned char ** [out] ((|beg|)) Pointer to head of ByteArray
      * @param
      *    ed unsigned char ** [out] ((|ed|)) Pointer to tail of ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayGetBeginEnd(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                                 unsigned char ** /* [out] */ beg,
                                 unsigned char ** /* [out] */ ed);
    
    /**
      * @fn int OpenSOAPByteArrayClear(OpenSOAPByteArrayPtr b_ary)
      * @brief Clear OpenSOAPByteArray
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayClear(OpenSOAPByteArrayPtr /* [in] */ b_ary);

    /**
      * @fn int OpenSOAPByteArraySetData(OpenSOAPByteArrayPtr b_ary, const unsigned char * data, size_t sz)
      * @brief Set data of OpenSOAPByteArray
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    data unsigned char * [in] ((|data|)) data 
      * @param
      *    sz size_t [in] ((|sz|)) data size
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArraySetData(OpenSOAPByteArrayPtr /* [in, out] */ b_ary,
                             const unsigned char * /* [in] */ data,
                             size_t /* [in] */ sz);
    
    /**
      * @fn int OpenSOAPByteArrayAppend(OpenSOAPByteArrayPtr b_ary, const unsigned char *dat, size_t dat_len)
      * @brief Append Byte data to OpenSOAPByteArray
      * @param
      *    b_ary OpenSOAPByteArrayPtr * [in] ((|b_ary|)) ByteArray
      * @param
      *    dat const unsigned char * [in] ((|dat|)) Pointer to head of Append data. If NULL, 0 fill.
      * @param
      *    dat_len size_t [in] dat_sz Append data size
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayAppend(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                            const unsigned char *dat,
                            size_t       dat_len);

    /**
      * @fn int OpenSOAPByteArrayResize(OpenSOAPByteArrayPtr b_ary, size_t new_size)
      * @brief Resize OpenSOAPByteArray
      * @param
      *    b_ary OpenSOAPByteArrayPtr [in, out] ((|b_ary|)) ByteArray
      * @param
      *    new_size size_t [in] ((|new_size|)) New size. If the new size is greater than the current size, 0 fill the extra area.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayResize(OpenSOAPByteArrayPtr /* [in, out] */ b_ary,
                            size_t       /* [in] */ new_size);

    /**
      * @fn int OpenSOAPByteArrayDuplicate(OpenSOAPByteArrayPtr b_ary, OpenSOAPByteArrayPtr * dup_b_ary)
      * @brief Duplicate OpenSOAPByteArray
      * @param
      *    b_ary OpenSOAPByteArrayPtr [in] ((|b_ary|)) ByteArray
      * @param
      *    dup_b_ary OpenSOAPByteArrayPtr * [out] ((|new_size|)) Duplicate ByteArray
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPByteArrayDuplicate(OpenSOAPByteArrayPtr /* [in] */ b_ary,
                               OpenSOAPByteArrayPtr * /* [out] */ dup_b_ary);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_ByteArray_H */
