/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Locale.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Locale_H
#define OpenSOAP_Locale_H

#include <OpenSOAP/Defines.h>

/**
 * @file OpenSOAP/Locale.h
 * @brief OpenSOAP API Locale Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
      * @fn int OpenSOAPLocaleGetCurrentCodeset(const char **codeset)
      * @brief Get Current Codeset
      * @param
      *    codeset const char ** [out] ((|codeset|)) codeset return buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPLocaleGetCurrentCodeset(/* [out] */ const char **codeset);

    /**
      * @fn int OpenSOAPLocaleIsCurrentCodeset(const char *codeset, int *isCurrentCodeset)
      * @brief Compare with current codeset
      * @param
      *    codeset const char * [in] ((|codeset|)) codeset
      * @param
      *    isCurrentCodeset int * [out] ((|isCurrentCodeset|)) Result return buffer. If codeset is current codeset then non-zero. Otherwise zero.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPLocaleIsCurrentCodeset(/* [in]  */ const char *codeset,
                                   /* [out] */ int *isCurrentCodeset);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Locale_H */
