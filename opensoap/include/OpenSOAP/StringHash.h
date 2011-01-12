/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: StringHash.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_StringHash_H
#define OpenSOAP_StringHash_H

#include <OpenSOAP/String.h>

/**
 * @file OpenSOAP/StringHash.h
 * @brief OpenSOAP API StringHash Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPStringHash OpenSOAPStringHash
     * @brief OpenSOAPStringHash Structure Type Definition
     */
    typedef struct tagOpenSOAPStringHash OpenSOAPStringHash;

    /**
     * @typedef OpenSOAPStringHash    *OpenSOAPStringHashPtr
     * @brief OpenSOAPStringHash Pointer Type Definition
     */
    typedef OpenSOAPStringHash    *OpenSOAPStringHashPtr;

    /**
      * @fn int OpenSOAPStringHashCreate(OpenSOAPStringHashPtr * strh)
      * @brief Create OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr * [out] ((|strh|)) Created OpenSOAP String Hash Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashCreate(OpenSOAPStringHashPtr * /* [out] */ strh);

    /**
      * @fn int OpenSOAPStringHashRelease(OpenSOAPStringHashPtr strh)
      * @brief Release OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashRelease(OpenSOAPStringHashPtr  /* [in] */ strh);

    /**
      * @fn int OpenSOAPStringHashClear(OpenSOAPStringHashPtr strh)
      * @brief Clear OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in, out] ((|strh|)) OpenSOAPString Hash
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashClear(OpenSOAPStringHashPtr /* [in, out] */ strh);

    /**
      * @fn int OpenSOAPStringHashRemoveKey(OpenSOAPStringHashPtr strh, OpenSOAPStringPtr key, void ** val)
      * @brief Remove Key From OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in, out] ((|strh|)) OpenSOAPString Hash
      * @param
      *    key OpenSOAPStringPtr [in] ((|key|)) Key Value
      * @param
      *    val void ** [out] ((|val|)) Location of registered value. If NULL, no value is returned.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashRemoveKey(OpenSOAPStringHashPtr /* [in, out] */ strh,
                                OpenSOAPStringPtr /* [in] */ key,
                                void ** /* [out] */ val);
    
    /**
      * @fn int OpenSOAPStringHashSetValue(OpenSOAPStringHashPtr strh, OpenSOAPStringPtr key, void * val)
      * @brief Register Value In OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in, out] ((|strh|)) OpenSOAPString Hash
      * @param
      *    key OpenSOAPStringPtr [in] ((|key|)) Key Value
      * @param
      *    val void * [in] ((|val|)) Registered Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashSetValue(OpenSOAPStringHashPtr /* [in, out] */ strh,
                               OpenSOAPStringPtr /* [in] */ key,
                               void * /* [in] */ val);
    
    /**
      * @fn int OpenSOAPStringHashSetValueMB(OpenSOAPStringHashPtr strh, const char * key, void * val)
      * @brief Register Value In OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in, out] ((|strh|)) OpenSOAPString Hash
      * @param
      *    key const char * [in] ((|key|)) Key Value
      * @param
      *    val void * [in] ((|val|)) Registered Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashSetValueMB(OpenSOAPStringHashPtr /* [in, out] */ strh,
                                 const char * /* [in] */ key,
                                 void * /* [in] */ val);

    /**
      * @fn int OpenSOAPStringHashSetValueWC(OpenSOAPStringHashPtr strh, const wchar_t * key, void * val)
      * @brief Register Value In OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in, out] ((|strh|)) OpenSOAPString Hash
      * @param
      *    key const wchar_t * [in] ((|key|)) Key Value
      * @param
      *    val void * [in] ((|val|)) Registered Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashSetValueWC(OpenSOAPStringHashPtr /* [in, out] */ strh,
                                 const wchar_t * /* [in] */ key,
                                 void * /* [in] */ val);
    
    /**
      * @fn int OpenSOAPStringHashGetValue(OpenSOAPStringHashPtr strh, OpenSOAPStringPtr key, void ** val)
      * @brief Get value from OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @param
      *    key OpenSOAPStringPtr [in] ((|key|)) Key Value
      * @param
      *    val void ** [out] ((|val|)) Returned Value Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashGetValue(OpenSOAPStringHashPtr /* [in] */ strh,
                               OpenSOAPStringPtr /* [in] */ key,
                               void ** /* [out] */ val);

    /**
      * @fn int OpenSOAPStringHashGetValueMB(OpenSOAPStringHashPtr strh, const char * key, void ** val)
      * @brief Get value from OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @param
      *    key const char * [in] ((|key|)) Key Value
      * @param
      *    val void ** [out] ((|val|)) Returned Value Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashGetValueMB(OpenSOAPStringHashPtr /* [in] */ strh,
                                 const char * /* [in] */ key,
                                 void ** /* [out] */ val);

    /**
      * @fn int OpenSOAPStringHashGetValueWC(OpenSOAPStringHashPtr strh, const wchar_t * key, void ** val)
      * @brief Get value from OpenSOAPString Hash
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @param
      *    key const wchar_t * [in] ((|key|)) Key Value
      * @param
      *    val void ** [out] ((|val|)) Returned Value Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashGetValueWC(OpenSOAPStringHashPtr /* [in] */ strh,
                                 const wchar_t * /* [in] */ key,
                                 void ** /* [out] */ val);

    /**
      * @fn int OpenSOAPStringHashGetSize(OpenSOAPStringHashPtr strh, size_t * sz)
      * @brief Get Number Of OpenSOAPString Hash Registrations
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @param
      *    sz size_t * [out] ((|sz|)) Return Value Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashGetSize(OpenSOAPStringHashPtr /* [in] */ strh,
                              size_t * /* [out] */ sz);

    /**
      * @fn int OpenSOAPStringHashGetKeys(OpenSOAPStringHashPtr strh, size_t * sz, OpenSOAPStringPtr * keys)
      * @brief Get OpenSOAPString Hash Keys(All)
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @param
      *    sz size_t * [in, out] ((|sz|))
      * @param
      *    keys OpenSOAPStringPtr * [out] ((|keys|)) Buffer containing registered keys
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashGetKeys(OpenSOAPStringHashPtr /* [in] */ strh,
                              size_t * /* [in, out] */ sz,
                              OpenSOAPStringPtr * /* [out] */ keys);
    /**
      * @fn int OpenSOAPStringHashGetValues(OpenSOAPStringHashPtr strh, size_t * sz, void ** vals)
      * @brief Get OpenSOAPString Hash Values(ALL)
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @param
      *    sz size_t * [in, out] ((|sz|))
      * @param
      *    vals void ** [out] ((|vals|)) Returned value buffer. No relation to key order generated by OpenSOAPStringHashGetKeys().
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashGetValues(OpenSOAPStringHashPtr /* [in] */ strh,
                                size_t * /* [in, out] */ sz,
                                void ** /* [out] */ vals);

    /**
      * @fn int OpenSOAPStringHashApplyToValues(OpenSOAPStringHashPtr strh, int  (*aply)(void *, void *), void * opt)
      * @brief Apply all OpenSOAPString Hash registered values to specified function
      * @param
      *    strh OpenSOAPStringHashPtr [in] ((|strh|)) OpenSOAPString Hash
      * @param
      *    aply() int [in] ( * ((|aply|)) )(void *val, void *opt) Function Pointer.
      * @param
      *    opt void * [in] ((|opt|)) Option Parameters for function.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPStringHashApplyToValues(OpenSOAPStringHashPtr /* [in, out] */ strh,
                                    int  /* [in] */ (*aply)(void *, void *),
                                    void * /* [in] */ opt);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_StringHash_H */
