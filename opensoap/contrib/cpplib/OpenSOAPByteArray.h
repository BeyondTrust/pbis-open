/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPByteArray.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_CPPLIB_OPENSOAPBYTEARRAY_H
#define OpenSOAP_CPPLIB_OPENSOAPBYTEARRAY_H
#include "OpenSOAPObject.h"

namespace COpenSOAP {

class ByteArray : public Object {
public:
	ByteArray()
		: pointer(0) {
		Try(OpenSOAPByteArrayCreate(&pointer));
	}

	~ByteArray() {
		if (pointer) {
			OpenSOAPByteArrayRelease(pointer);
		}
	}

	std::string GetObjectName() const { return "ByteArray" ;}

	operator OpenSOAPByteArrayPtr () {
		return pointer;
	}

	const unsigned char *
	begin(size_t &size) const {
		const unsigned char *ret = 0;
		int errorCode
			= OpenSOAPByteArrayGetBeginSizeConst(pointer, &ret, &size);
		//
		Try(errorCode);

		return ret;
	}

private:
	OpenSOAPByteArrayPtr pointer;
};

}; //COpenSOAP
#endif //OpenSOAP_CPPLIB_OPENSOAPBYTEARRAY_H
