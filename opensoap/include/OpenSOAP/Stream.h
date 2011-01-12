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
#ifndef OpenSOAP_Stream_H
#define OpenSOAP_Stream_H

#include <OpenSOAP/ByteArray.h>

/**
 * @file OpenSOAP/Stream.h
 * @brief OpenSOAP API Stream Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPStream OpenSOAPStream
     * @brief OPenSOAPStream Structure Type Definition
     */
    typedef struct tagOpenSOAPStream OpenSOAPStream;

    /**
     * @typedef OpenSOAPStream    *OpenSOAPStreamPtr
     * @brief OpenSOAPStream Pointer Type Definition
     */
    typedef OpenSOAPStream    *OpenSOAPStreamPtr;
    
    /**
      * @fn int OpenSOAPStreamRelease(OpenSOAPStreamPtr s)
      * @brief Release OpenSOAP Stream
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamRelease(OpenSOAPStreamPtr /* [in, out] */ s);
    
    /**
      * @fn int OpenSOAPStreamRead(OpenSOAPStreamPtr s, unsigned char * buf, size_t * buf_sz)
      * @brief Read From OpenSOAP Stream
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @param
      *    buf unsigned char * [out] ((|buf|)) Read Buffer
      * @param
      *    buf_sz size_t * [in, out] ((|buf_sz|)) Read Buffer size. After calling this function, contains actual size read.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamRead(OpenSOAPStreamPtr /* [in] */ s,
                       unsigned char * /* [out] */ buf,
                       size_t * /* [in, out] */ buf_sz);
    

    /**
      * @fn int OpenSOAPStreamWrite(OpenSOAPStreamPtr s, const unsigned char * buf, size_t * buf_sz)
      * @brief Write To OpenSOAP Stream
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @param
      *    buf const unsigned char * [in] ((|buf|)) Write Buffer
      * @param
      *    buf_sz size_t * [in, out] ((|buf_sz|)) Write Buffer size. After calling this function, contains actual size written.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamWrite(OpenSOAPStreamPtr /* [in] */ s,
                        const unsigned char * /* [in] */ buf,
                        size_t * /* [in, out] */ buf_sz);
    

    /**
      * @fn int OpenSOAPStreamReadToDelm(OpenSOAPStreamPtr s, const unsigned char * delm, size_t delm_sz, OpenSOAPByteArrayPtr buf)
      * @brief Read From OpenSOAP Stream Up To Delimiter
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @param
      *    delm const unsigned char * [in] ((|delm|)) Delimiter String
      * @param
      *    delm_sz size_t [in] ((|delm_sz|)) Delimiter Size
      * @param
      *    buf OpenSOAPByteArrayPtr [out] ((|buf|)) Read Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamReadToDelm(OpenSOAPStreamPtr /* [in] */ s,
                             const unsigned char * /* [in] */ delm,
                             size_t /* [in] */ delm_sz,
                             OpenSOAPByteArrayPtr /* [out] */ buf);
    
    /**
      * @fn int OpenSOAPStreamReadChunkedDataStream(OpenSOAPStreamPtr s, OpenSOAPStreamPtr out)
      * @brief Read Chunked Data From OpenSOAP Stream and write out to Stream.
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @param
      *    out OpenSOAPStreamPtr [out] ((|buf|)) Output Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamReadChunkedDataStream(OpenSOAPStreamPtr /* [in] */ s,
                                        OpenSOAPStreamPtr /* [out] */ out);
    
    /**
      * @fn int OpenSOAPStreamReadChunkedData(OpenSOAPStreamPtr s, OpenSOAPByteArrayPtr buf)
      * @brief Read Chunked Data From OpenSOAP Stream.
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @param
      *    buf OpenSOAPByteArrayPtr [out] ((|buf|)) Read Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamReadChunkedData(OpenSOAPStreamPtr /* [in] */ s,
                                  OpenSOAPByteArrayPtr /* [out] */ buf);
    
    /**
      * @fn int OpenSOAPStreamReadWithSize(OpenSOAPStreamPtr s, size_t sz, OpenSOAPByteArrayPtr buf)
      * @brief Read Specified Size From OpenSOAP Stream. If size is 0, read as much as possible.
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @param
      *    sz size_t [in] ((|sz|)) Read size
      * @param
      *    buf OpenSOAPByteArrayPtr [out] ((|buf|)) Read Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamReadWithSize(OpenSOAPStreamPtr /* [in] */ s,
                               size_t /* [in] */ sz,
                               OpenSOAPByteArrayPtr /* [out] */ buf);

    /**
      * @fn int OpenSOAPStreamReadWithSizeStream(OpenSOAPStreamPtr s, size_t sz, OpenSOAPStreamPtr out)
      * @brief Read Specified Size From OpenSOAP Stream. If size is 0, read as much as possible.
      * @param
      *    s OpenSOAPStreamPtr [in] ((|s|)) OpenSOAP Stream
      * @param
      *    sz size_t [in] ((|sz|)) Read size
      * @param
      *    out OpenSOAPStreamPtr [out] ((|out|)) Output Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamReadWithSizeStream(OpenSOAPStreamPtr /* [in] */ s,
                                     size_t /* [in] */ sz,
                                     OpenSOAPStreamPtr /* [out] */ out);

    /**
      * @fn int OpenSOAPStreamWriteByteArray(OpenSOAPStreamPtr s, OpenSOAPByteArrayPtr buf, size_t * write_sz)
      * @brief Write To OpenSOAP Stream
      * @param
      *    s OpenSOAPStreamPtr  [in] ((|s|)) OpenSOAP Stream
      * @param
      *    buf OpenSOAPByteArrayPtr [in] ((|buf|)) Write Buffer
      * @param
      *    write_sz size_t * [out] ((|write_sz|)) Write Size
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStreamWriteByteArray(OpenSOAPStreamPtr /* [in] */ s,
                                 OpenSOAPByteArrayPtr /* [in] */ buf,
                                 size_t * /* [out] */ write_sz);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Stream_H */
