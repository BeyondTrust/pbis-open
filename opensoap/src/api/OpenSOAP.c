/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAP.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: OpenSOAP.c,v 1.14 2003/07/03 00:33:06 bandou Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/ErrorCode.h>
#include <OpenSOAP/Serializer.h>
#include "Serializer.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <locale.h>

#if defined(WIN32) && !defined(__GNUC__)
#define NONGCC_WIN32 1
#endif /* */

/* */
static long refCounter = 0;

/* Socket system lock object */
extern
void
OpenSOAPSocketSystemLockInitialize();

extern
void
OpenSOAPSocketSystemUltimate();

/*
=begin
= OpenSOAP API initialize and ultimate
=end
*/


/*
=begin
---OpenSOAPInitialize(param)
   OpenSOAP API initalize. In this function, Serializer/Deserializer
   regist. If use COM in OpenSOAP API, then CoInitilizeEx is called.

   :Parameters
     :void * [in] ((|param|))
       Initalize parameter. Currently, this parameter is no mean.
   :Return Value
     :int
=end
*/
extern
int
OPENSOAP_API
OpenSOAPInitialize(/* [in] */ void *param) {
    int ret = OPENSOAP_NO_ERROR;

	if (!refCounter) {
#ifndef NONGCC_WIN32
		if (!param) {
			if (!setlocale(LC_ALL, "")) {
				ret = OPENSOAP_SETLOCALEFAILURE;
			}
		}
#endif
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPSerializerRegistDefaults();
			if (OPENSOAP_SUCCEEDED(ret)) {
				OpenSOAPSocketSystemLockInitialize();
			}
		}
	}

	if (OPENSOAP_SUCCEEDED(ret)) {
		++refCounter;
	}
    
    return ret;
}

/* ultimate OpenSOAP API */
/*
=begin
---OpenSOAPInitialize(param)
   OpenSOAP API ultimate.  If use COM in OpenSOAP API,
   then CoUninitilize is called.

   :Parameters
     no parameters.
   :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPUltimate() {
    int ret = OPENSOAP_NOT_CATEGORIZE_ERROR;

	if (refCounter) {
		--refCounter;
		ret = OPENSOAP_NO_ERROR;
		if (!refCounter) {
			ret = OpenSOAPSerializerRegisterUltimate();

			OpenSOAPSocketSystemUltimate();
		}
	}
	
    return ret;
}
