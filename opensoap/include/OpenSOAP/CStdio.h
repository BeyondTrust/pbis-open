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
#ifndef OpenSOAP_CStdio_H
#define OpenSOAP_CStdio_H

#include <OpenSOAP/ByteArray.h>

#include <stdio.h>

/**
 * @file OpenSOAP/CStdio.h
 * @brief OpenSOAP API CStdio Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPCStdio OpenSOAPCStdio
     * @brief OpenSOAPCStdio Structure Type Definition
     */
    typedef struct tagOpenSOAPCStdio OpenSOAPCStdio;

    /**
     * @typedef OpenSOAPCStdio    *OpenSOAPCStdioPtr
     * @brief OpenSOAPCStdio Pointer Type Definition
     */
    typedef OpenSOAPCStdio    *OpenSOAPCStdioPtr;
    
    /**
      * @fn int OpenSOAPCStdioCreate(OpenSOAPCStdioPtr *s)
      * @brief OpenSOAP CStdio Instance Create
      * @param
      *    s OpenSOAPCStdioPtr * [out] ((|s|)) OpenSOAP CStdio pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioCreate(/* [out] */ OpenSOAPCStdioPtr *s);
    
    /**
      * @fn int OpenSOAPCStdioCreateWithFILEPtr(FILE *is, FILE *os, OpenSOAPCStdioPtr *s)
      * @brief OpenSOAP CStdio Instance Create
      * @param
      *    is FILE * [in] ((|is|)) FILE pointer
      * @param
      *    os FILE * [in] ((|os|)) FILE pointer
      * @param
      *    s OpenSOAPCStdioPtr * [out] ((|s|)) OpenSOAP CStdio pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioCreateWithFILEPtr(/* [in]  */ FILE *is,
                                    /* [in]  */ FILE *os,
                                    /* [out] */ OpenSOAPCStdioPtr *s);
    
    /**
      * @fn int OpenSOAPCStdioCreateForStdio(OpenSOAPCStdioPtr *s)
      * @brief OpenSOAP CStdio Instance Create for Stdio
      * @param
      *    s OpenSOAPCStdioPtr * [out] ((|s|)) OpenSOAP CStdio pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioCreateForStdio(/* [out] */ OpenSOAPCStdioPtr *s);
    
    /**
      * @fn int OpenSOAPCStdioClose(OpenSOAPCStdioPtr s)
      * @brief Close and Disconnect OpenSOAP CStdio
      * @param
      *    s OpenSOAPCStdioPtr [in] ((|s|)) OpenSOAP CStdio
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioClose(/* [in] */ OpenSOAPCStdioPtr s);

    /**
      * @fn int OpenSOAPCStdioRelease(OpenSOAPCStdioPtr s)
      * @brief OpenSOAP CStdio Release
      * @param
      *    s OpenSOAPCStdioPtr [in] ((|s|)) OpenSOAP CStdio
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioRelease(/* [in] */ OpenSOAPCStdioPtr s);

    /**
      * @fn int OpenSOAPCStdioSetBinaryMode(OpenSOAPCStdioPtr s)
      * @brief Set OpenSOAP CStdio Stream to BINARY mode
      * @param
      *    s OpenSOAPCStdioPtr [in] ((|s|)) OpenSOAP CStdio
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioSetBinaryMode(/* [in] */ OpenSOAPCStdioPtr s);
    
    /**
      * @fn int OpenSOAPCStdioSetTextMode(OpenSOAPCStdioPtr s)
      * @brief Set OpenSOAP CStdio Stream to TEXT mode
      * @param
      *    s OpenSOAPCStdioPtr [in] ((|s|)) OpenSOAP CStdio
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioSetTextMode(/* [in] */ OpenSOAPCStdioPtr s);

    /**
      * @fn int OpenSOAPCStdioRead(OpenSOAPCStdioPtr s, unsigned char *buf, size_t *buf_sz)
      * @brief Read From OpenSOAP CStdio Stream
      * @param
      *    s OpenSOAPCStdioPtr [in] ((|s|)) OpenSOAP CStdio
      * @param
      *    buf unsigned char * [out] ((|buf|)) Read Buffer
      * @param
      *    buf_sz size_t * [in, out] ((|buf_sz|)) Read Buffer size. Returns actual size read.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioRead(/* [in]      */ OpenSOAPCStdioPtr s,
                       /* [out]     */ unsigned char *buf,
                       /* [in, out] */ size_t *buf_sz);
    
    /**
      * @fn int OpenSOAPCStdioWrite(OpenSOAPCStdioPtr s, const unsigned char *buf, size_t *buf_sz)
      * @brief Write to OpenSOAP CStdio Stream
      * @param
      *    s OpenSOAPCStdioPtr [in] ((|s|)) OpenSOAP CStdio
      * @param
      *    buf const unsigned char * [in] ((|buf|)) Write Buffer
      * @param
      *    buf_sz size_t * [in, out] ((|buf_sz|)) Write Buffer size. Returns actual size written
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioWrite(/* [in]      */ OpenSOAPCStdioPtr s,
                        /* [in]      */ const unsigned char *buf,
                        /* [in, out] */ size_t *buf_sz);

    /**
      * @fn int OpenSOAPCStdioReadToDelm(OpenSOAPCStdioPtr s, const unsigned char *delm, size_t delm_sz, OpenSOAPByteArrayPtr buf)
      * @brief Read to Delimiter from OpenSOAP CStdio Stream
      * @param
      *    s OpenSOAPCStdioPtr [in] ((|s|)) OpenSOAP CStdio
      * @param
      *    delm const unsigned char * [in] ((|delm|)) Delimiter string
      * @param
      *    delm_sz size_t [in] ((|delm_sz|)) Delimiter length
      * @param
      *    buf OpenSOAPByteArrayPtr [out] ((|buf|)) Read Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPCStdioReadToDelm(/* [in]  */ OpenSOAPCStdioPtr s,
                             /* [in]  */ const unsigned char *delm,
                             /* [in]  */ size_t delm_sz,
                             /* [out] */ OpenSOAPByteArrayPtr buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_CStdio_H */
