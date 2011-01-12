/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPStructure.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_CPPLIB_OPENSOAPSTRUCTURE_H
#define OpenSOAP_CPPLIB_OPENSOAPSTRUCTURE_H

#include <string>
#include <cassert>

namespace COpenSOAP {

class XMLElm;
class Structure
{
public:
	Structure() {}
	virtual ~Structure() {}

	virtual void CreateMessage(XMLElm& elm) const = 0;
	virtual void ParseMessage(const XMLElm& elm) = 0;

	virtual std::string GetNamespaceURI()
	{ return ""; }
	virtual std::string GetNamespacePrefix()
	{ return ""; }
};

class InStructure : public Structure
{
public:
	void CreateMessage(XMLElm& elm) const = 0;
private:
	void ParseMessage(const XMLElm& elm) {
		assert(false);
	};
};

class OutStructure : public Structure
{
private:
	void CreateMessage(XMLElm& elm) const {
		assert(false);
	};
public:
	void ParseMessage(const XMLElm& elm) = 0;
};

class EmptyStructure : public Structure{
public:
	void CreateMessage(XMLElm& elm) const {};
	void ParseMessage(const XMLElm& elm) {};
};

}; // COpenSOAP
#endif //OpenSOAP_CPPLIB_OPENSOAPSTRUCTURE_H
