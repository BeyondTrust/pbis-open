/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPString.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(OpenSOAP_CPPLIB_OPENSOAPSTRING_H)
#define OpenSOAP_CPPLIB_OPENSOAPSTRING_H

#include<OpenSOAP/String.h>
#include<string>
#include<iostream>
#include"OpenSOAPObject.h"

namespace COpenSOAP {
class XMLElm;

class String  : public Object
{
public:
	std::string GetObjectName() const { return "String"; }

	String();
	String(const std::string& str);
	String(OpenSOAPStringPtr p);

	String(const String& costr); //copy constructor
	String& operator = (const String& cos);
	String& operator = (const std::string& str);

	virtual ~String();
	
	OpenSOAPStringPtr RetainOpenSOAPStringPtr() ;
	OpenSOAPStringPtr GetOpenSOAPStringPtr();

	std::string GetString() const;
	void SetString(const std::string& str);

	void CreateMessage(const std::string& name, XMLElm& elm);

	void Format(const char* format, ...);

	operator std::string();

private:
	void realize();
	void init();

	OpenSOAPStringPtr pString;

	bool bCopy;
};
}; // COpenSOAP

std::ostream& operator << (std::ostream& os, const COpenSOAP::String& str);

#endif // !defined(OpenSOAP_CPPLIB_OPENSOAPSTRING_H)
