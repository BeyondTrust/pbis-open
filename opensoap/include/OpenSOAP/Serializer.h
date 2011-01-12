/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Serializer.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Serializer_H
#define OpenSOAP_Serializer_H

#include <OpenSOAP/String.h>

/**
 * @file OpenSOAP/Serializer.h
 * @brief OpenSOAP API Serializer Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
      * @typedef int (*OpenSOAPSerializerFunc)(void *from_value, OpenSOAPStringPtr to)
      * @brief OpenSOAP Serializer Function Type
      * @param
      *    from_value void * [in] ((|from_value|)) Serializer Object Assign Pointer
      * @param
      *    to OpenSOAPStringPtr [out] ((|to|)) Result Value Pointer
      * @return
      *    Error Code
      */
    typedef int
    (*OpenSOAPSerializerFunc)(/* [in]  */ void *from_value,
                              /* [out] */ OpenSOAPStringPtr to);

    /**
      * @typedef int (*OpenSOAPDeserializerFunc)(OpenSOAPStringPtr from, void *to_value)
      * @brief OpenSOAP Deserializer Function Type
      * @param
      *    from OpenSOAPStringPtr [in] ((|from|)) Deserializer Object Assign Pointer
      * @param
      *    to_value void * [out] ((|to_value|)) Result Value Pointer
      * @return
      *    Error Code
      */
    typedef int
    (*OpenSOAPDeserializerFunc)(/* [in]  */ OpenSOAPStringPtr from,
                                /* [out] */ void *to_value);

    /**
      * @fn int OpenSOAPSerializerRegisterMB(const char *soap_typename, OpenSOAPSerializerFunc serializer, OpenSOAPDeserializerFunc deserializer)
      * @brief Serializer and deserializer register function.(MB)
      * @param
      *    soap_typename  const char * [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    serializer OpenSOAPSerializerFunc [in] ((|serializer|)) Serializer Function
      * @param
      *    deserializer OpenSOAPDeserializerFunc [in] ((|deserializer|)) Deserializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSerializerRegisterMB(/* [in] */ const char *soap_typename,
                                 /* [in] */ OpenSOAPSerializerFunc serializer,
                                 /* [in] */ OpenSOAPDeserializerFunc deserializer);

    /**
      * @fn int OpenSOAPSerializerRegisterWC(const wchar_t *soap_typename, OpenSOAPSerializerFunc serializer, OpenSOAPDeserializerFunc deserializer)
      * @brief Serializer and deserializer register function.(WC)
      * @param
      *    soap_typename const wchar_t * [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    serializer OpenSOAPSerializerFunc [in] ((|serializer|)) Serializer Function
      * @param
      *    deserializer OpenSOAPDeserializerFunc [in] ((|deserializer|)) Deserializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSerializerRegisterWC(/* [in] */ const wchar_t *soap_typename,
                                 /* [in] */ OpenSOAPSerializerFunc serializer,
                                 /* [in] */ OpenSOAPDeserializerFunc deserializer);

    /**
      * @fn int OpenSOAPGetSerializer(OpenSOAPStringPtr soap_typename, OpenSOAPSerializerFunc *serializer)
      * @brief Get Serializer(OpenSOAPString)
      * @param
      *    soap_typename OpenSOAPStringPtr [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    serializer OpenSOAPSerializerFunc * [out] ((|serializer|)) Serializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPGetSerializer(/* [in]  */ OpenSOAPStringPtr soap_typename,
                          /* [out] */ OpenSOAPSerializerFunc *serializer);

    /**
      * @fn int OpenSOAPGetSerializerMB(const char *soap_typename, OpenSOAPSerializerFunc *serializer)
      * @brief Get Serializer(MB)
      * @param
      *    soap_typename const char * [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    serializer OpenSOAPSerializerFunc * [out] ((|serializer|)) Serializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPGetSerializerMB(/* [in]  */ const char *soap_typename,
                            /* [out] */ OpenSOAPSerializerFunc *serializer);

    /**
      * @fn int OpenSOAPGetSerializerWC(const wchar_t *soap_typename, OpenSOAPSerializerFunc *serializer)
      * @brief Get Serializer(WC)
      * @param
      *    soap_typename const wchar_t * [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    serializer OpenSOAPSerializerFunc * [out] ((|serializer|)) Serializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPGetSerializerWC(/* [in]  */ const wchar_t *soap_typename,
                            /* [out] */ OpenSOAPSerializerFunc *serializer);
    
    /**
      * @fn int OpenSOAPGetDeserializer(OpenSOAPStringPtr soap_typename, OpenSOAPDeserializerFunc *deserializer)
      * @brief Get Deserializer(OpenSOAPString)
      * @param
      *    soap_typename OpenSOAPStringPtr [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    deserializer OpenSOAPDeserializerFunc * [out] ((|deserializer|)) Deserializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPGetDeserializer(/* [in]  */ OpenSOAPStringPtr soap_typename,
                            /* [out] */ OpenSOAPDeserializerFunc *deserializer);

    /**
      * @fn int OpenSOAPGetDeserializerMB(const char *soap_typename, OpenSOAPDeserializerFunc *deserializer)
      * @brief Get Deserializer(MB)
      * @param
      *    soap_typename const char * [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    deserializer OpenSOAPDeserializerFunc * [out] ((|deserializer|)) Deserializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPGetDeserializerMB(/* [in]  */ const char *soap_typename,
                              /* [out] */ OpenSOAPDeserializerFunc *deserializer);

    /**
      * @fn int OpenSOAPGetDeserializerWC(const wchar_t *soap_typename, OpenSOAPDeserializerFunc *deserializer)
      * @brief Get Deserializer(WC)
      * @param
      *    soap_typename const wchar_t * [in] ((|soap_typename|)) SOAP Type Name
      * @param
      *    deserializer OpenSOAPDeserializerFunc * [out] ((|deserializer|)) Deserializer Function
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPGetDeserializerWC(/* [in]  */ const wchar_t *soap_typename,
                              /* [out] */ OpenSOAPDeserializerFunc *deserializer);

    /**
      * @fn int OpenSOAPSerializerRegistDefaults(void)
      * @brief OpenSOAP default Serializers/Deserializers Registration
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSerializerRegistDefaults(void);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Serializer_H */
