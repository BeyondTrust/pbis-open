/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Block.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_Block_H
#define OpenSOAP_IMPL_Block_H

#include <OpenSOAP/Block.h>

#include "XMLElm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    struct tagOpenSOAPBlock {
        OpenSOAPXMLElm	super;
    };

	/* */
    int
    OpenSOAPBlockCreate(/* [in, out] */ OpenSOAPBlockPtr *soapBlock);

	/* */
	int
	OpenSOAPBlockReplaceXMLElmChildren(/* [in, out] */ OpenSOAPXMLElmPtr elm);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Block_H */
