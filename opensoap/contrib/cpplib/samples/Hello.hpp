/*-----------------------------------------------------------------------------
 * $RCSfile: Hello.hpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "OpenSOAPMethod.h"

using namespace COpenSOAP;
using namespace std;

struct Hello_in : public Structure
{
	void CreateMessage(XMLElm& elm) const {
		elm.SetChildValue("name", name);
	}
	void ParseMessage(const XMLElm& elm) {
		elm.GetChildValue("name", name);
	}

	string name;
};

struct Hello_out : public Structure
{
	void CreateMessage(XMLElm& elm) const {
		elm.SetChildValue("reply", reply);
	}
	void ParseMessage(const XMLElm& elm) {
		elm.GetChildValue("reply", reply);
	}

	string reply;
};

class Hello_method : public Method
{
public:
	std::string GetSOAPAction()
	{ return ""; }
	std::string GetMethodName()
	{ return "Hello"; }

	std::string GetNamespaceURI()
	{ return "http://opensoap.jp/samples/hello"; }
};
