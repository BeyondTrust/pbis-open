/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: LocaleImpl.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_Locale_H
#define OpenSOAP_IMPL_Locale_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	extern
	int
	OpenSOAPLocaleGetCodesetList(/* [in]  */ const char *codeset,
								 /* [in]  */ size_t codesetLen,
								 /* [out] */ const char * const **codesetList);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Locale_H */
