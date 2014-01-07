/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPString.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/OpenSOAP.h>
#include <cstdio>
#include <stdarg.h>
#include "OpenSOAPString.h"
#include "OpenSOAPXMLElm.h"

namespace COpenSOAP {

String::String()
{
	init();

	Try(OpenSOAPStringCreate(&pString));
}

String::String(const std::string& str)
{
	init();

	Try(OpenSOAPStringCreate(&pString));
	
	SetString(str);
}

String::String(OpenSOAPStringPtr postr)
{
	init();

	pString = postr;
}

String::String(const String& cos) 
{
	init();

	pString = cos.pString;
	OpenSOAPStringRetain(pString);

	bCopy = true;
}

String& String::operator = (const String& cos)
{
	if(this == &cos) 
		return *this;

	pString = cos.pString;
	OpenSOAPStringRetain(pString);

	bCopy = true;

	return *this;
}

String& String::operator =(const std::string& str)
{
	SetString(str.c_str());
	
	return *this;
}

String::operator std::string()
{
	return GetString();
}

std::ostream& operator << (std::ostream& os, const String& str)
{
	return os << str.GetString();
}

void String::init()
{
	bCopy = false;
}

String::~String()
{
	OpenSOAPStringRelease(pString);
}

OpenSOAPStringPtr String::RetainOpenSOAPStringPtr()   
{ 
	OpenSOAPStringRetain(pString);
	return pString; 
}

OpenSOAPStringPtr String::GetOpenSOAPStringPtr()
{
	return pString;
}

static char *charAllocator(size_t s) { return new char[s]; }

std::string String::GetString() const 
{
	char* pbuf;
	OpenSOAPStringGetStringMBWithAllocator(pString, 
										  charAllocator,
										  0, &pbuf);
	std::string str(pbuf);
	delete[] pbuf;

	return str;
}

void String::SetString(const std::string& str)
{
	realize();

	Try(OpenSOAPStringSetStringMB(pString, str.c_str()) );
}

static const int BUF_LEN = 1024;

void String::Format(const char *format, ...)
{
	int n, size = BUF_LEN;
	char* p = new char[size];
	
	va_list va;
	while(1) {
		va_start(va, format);
#ifdef WIN32
		n = _vsnprintf(p, BUF_LEN, format, va);
#else
		n = vsnprintf(p, BUF_LEN, format, va);
#endif
		va_end(va);
		
		if(n > -1 && n < size)
			break;
			
		if(n > -1)
			size = n+1;
		else 
			size *= 2;
		delete[] p;
		p = new char[size];
	}
	SetString(p);
}

////////////////////

void String::realize()
{
	if(bCopy) {
		OpenSOAPStringPtr pcopy = pString;
		OpenSOAPStringDuplicate(pcopy, &pString);
		OpenSOAPStringRelease(pcopy);
		bCopy = false;
	}
}

};//COpenSOAP
