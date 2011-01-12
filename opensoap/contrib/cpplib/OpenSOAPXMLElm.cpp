/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPXMLElm.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "OpenSOAPXMLElm.h"
#include "OpenSOAPString.h"

#include <string>

namespace COpenSOAP {

XMLElm::XMLElm()
:pElm(0), bAddTypeName(false)
{}

XMLElm::XMLElm(OpenSOAPXMLElmPtr elm)
:pElm(elm), bAddTypeName(false)
{}

XMLElm::~XMLElm()
{}

void 
XMLElm::SetNamespace(const std::string& uri, const std::string& prefix) {
	if(!uri.empty())
	{
		Try(OpenSOAPXMLElmSetNamespaceMB(pElm, uri.c_str(), 
							prefix.empty() ? 0 : prefix.c_str()));
	}
}

void 
XMLElm::SetChildValueString(const std::string& name, 
									   const String& str) {
	XMLElm elm = AddChild(name);
		if(bAddTypeName) {
		elm.SetAttributeValueString("xsi:type", "xsd:string");
	}
	void *opstr = const_cast<String&>(str).GetOpenSOAPStringPtr();
	Try(OpenSOAPXMLElmSetValueMB(elm.pElm, "string", &opstr));
}

void XMLElm::SetChildValue(const std::string& name, const std::string& type, void *value)
{
	Try(
		OpenSOAPXMLElmSetChildValueMB(pElm, name.c_str(), type.c_str(), &value)
		);
}

String XMLElm::GetChildValueString(const std::string& name) const
{
	return GetChild(name).GetValueString();
}

String XMLElm::GetValueString() const
{
	OpenSOAPStringPtr str = NULL;
	OpenSOAPStringCreate(&str);
	int r = OpenSOAPXMLElmGetValueMB(pElm, "string", &str);
	if(r == OPENSOAP_XMLNODE_NOT_FOUND) {
		return String();
	} else {
		Try(r);
	}
	return String(str);
}

XMLElm
XMLElm::GetChild(const std::string& name) const
{
	OpenSOAPXMLElmPtr elm = NULL;
	Try(OpenSOAPXMLElmGetChildMB(pElm, name.c_str(), &elm));
	return XMLElm(elm);
}


std::string
XMLElm::
GetCharEncodingString(const std::string &encoding) {
	ByteArray array;
	OpenSOAPXMLElmGetCharEncodingString(pElm, encoding.c_str(), array);
	size_t size = 0;
	const char *beg
		= reinterpret_cast<const char *>(array.begin(size));

	std::string ret(beg, size);
	 
	return ret;
}

}; // COpenSOAP

