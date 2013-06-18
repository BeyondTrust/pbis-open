/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPInitializer.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_CPPLIB_OPENSOAPINITIALIZER_H
#define OpenSOAP_CPPLIB_OPENSOAPINITIALIZER_H

#include <OpenSOAP/OpenSOAP.h>

namespace COpenSOAP{
class Initializer {
public:
 	Initializer(void *opt = 0) {
 		OpenSOAPInitialize(opt);
 	}

 	~Initializer() {
 		OpenSOAPUltimate();
 	}
};
}; // OpenSOAP
#endif //OpenSOAP_CPPLIB_OPENSOAPINITIALIZER_H
