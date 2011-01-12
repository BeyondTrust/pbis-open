/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPObject.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(OpenSOAP_CPPLIB_OPENSOAPOBJECT_H)
#define OpenSOAP_CPPLIB_OPENSOAPOBJECT_H
#include <string>
#include <iostream>

namespace COpenSOAP{ 

class opensoap_failed {
public:
	opensoap_failed(const std::string& name, int code) 
		:objectName(name), errorCode(code) {}
	~opensoap_failed() {}

	const char* GetObjectName() const { return objectName.c_str(); }

	int GetErrorCode() const { return errorCode; }

	std::string objectName;
	int errorCode;
};

class Object  
{
public:
	Object() {}
	virtual ~Object() {}

	virtual std::string GetObjectName() const = 0;
protected:
	virtual void Try(int r) const {
		if(OPENSOAP_FAILED(r)){
			throw opensoap_failed(GetObjectName(), r);
		}
	}
};

};// COpenSOAP
#endif // !defined(OpenSOAP_CPPLIB_OPENSOAPOBJECT_H)
