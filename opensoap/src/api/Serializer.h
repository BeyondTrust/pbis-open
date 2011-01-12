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
#ifndef OpenSOAP_Impl_Serializer_H
#define OpenSOAP_Impl_Serializer_H

#include <OpenSOAP/Serializer.h>

#define	OPENSOAP_SERIALIZER_NAME(typeName) 	OpenSOAPSerializer##typeName
#define	OPENSOAP_DESERIALIZER_NAME(typeName) 	OpenSOAPDeserializer##typeName

#define OPENSOAP_SERIALIZER_PAIR(typeName) \
	OpenSOAPSerializer##typeName, \
	OpenSOAPDeserializer##typeName

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	int
	OpenSOAPSerializerRegisterUltimate(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Impl_Serializer_H */
