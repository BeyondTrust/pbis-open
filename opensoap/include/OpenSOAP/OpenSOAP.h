/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAP.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_OpenSOAP_H
#define OpenSOAP_OpenSOAP_H

#include <OpenSOAP/Defines.h>

/**
 * @file OpenSOAP/OpenSOAP.h
 * @brief OpenSOAP API initialization/termination
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    /**
	 * @fn int OpenSOAPInitialize(void *param)
	 * @brief OpenSOAP API Initialization
	 * @param
	 *   param	Optional parameter. At current version,
	 *          if NULL then call setlocale()
	 * @return
	 *    Error code.
	 */
    int
    OPENSOAP_API
    OpenSOAPInitialize(/* [in] */ void *param);

    /**
	 * @fn int OpenSOAPUltimate(void)
	 * @brief OpenSOAP API Termination
	 * @return
	 *    Error code.
	 */
    int
    OPENSOAP_API
    OpenSOAPUltimate(void);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_OpenSOAP_H */
