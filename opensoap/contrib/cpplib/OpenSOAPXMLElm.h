/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPXMLElm.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(OpenSOAP_CPPLIB_OPENSOAPXMLELM_H)
#define OpenSOAP_CPPLIB_OPENSOAPXMLELM_H

#include<OpenSOAP/XMLElm.h>
#include<OpenSOAP/String.h>
#include "OpenSOAPObject.h"
#include "OpenSOAPByteArray.h"
#include "OpenSOAPString.h"
#include "OpenSOAPStructure.h"

#include <string>
#include <vector>
#include <typeinfo>

namespace COpenSOAP {

	class XMLElm : public Object
		{
		protected:
			std::string GetObjectName() const 
				{ return "XMLElm"; }
			template <class T> std::string GetTypeName(const T &value) const
				{
					if(typeid(value) == typeid(long)) {
						return "int";
					}else if(typeid(value) == typeid(float)) {
						return "float";
					}else if(typeid(value) == typeid(double)) {
						return "double";
					}else if(typeid(value) == typeid(bool)) {
						return "bool";
					}else {
						return "unknown";
					}
				}
		public:
			XMLElm();
			XMLElm(OpenSOAPXMLElmPtr elm);
			virtual ~XMLElm();

			virtual void SetNamespace(const std::string& uri, const std::string& prefix);
			virtual void SetChildValueString(const std::string& name, const String& opstring);
			virtual void SetChildValue(const std::string& name, const std::string& type, void* value);

			virtual String GetChildValueString(const std::string& name) const;
			virtual String GetValueString() const;

			virtual std::string GetCharEncodingString(const std::string& encoding);

			OpenSOAPXMLElmPtr GetOpenSOAPPtr()
				{ return pElm; }

			bool Good()
				{ return (pElm != 0); }

			void EnableAddTypeName(bool b = true) { bAddTypeName = b; }

			virtual void SetAttributeValueString(const std::string &name, const std::string& value)
				{
					String str(value);
					void* opstr = reinterpret_cast<void*>(str.GetOpenSOAPStringPtr());
					Try(OpenSOAPXMLElmSetAttributeValueMB(pElm, name.c_str(),
														  "string", &opstr));
				}

			template <class T> void SetChildBasicValue(const std::string &name, const T &value)
				{
					std::string type = GetTypeName(value);

					XMLElm elm = AddChild(name);
					if(bAddTypeName) {
						elm.SetAttributeValueString("xsi:type", "xsd:"+type);
					}
					Try(OpenSOAPXMLElmSetValueMB(elm.pElm, type.c_str(),
												 reinterpret_cast<void **>(&const_cast<T &>(value))));
				}

			void SetChildValue(const std::string &name, const Structure& value)
				{
					XMLElm elm = AddChild(name);
					value.CreateMessage(elm); 
				}

			template <class T> void SetChildValue(const std::string &name, const std::vector<T> ar) 
				{
					typename std::vector<T>::iterator it;
					for( it=ar.begin(); it<ar.end(); it++) {
						SetChildValue(name, *it);
					}
				}

			void SetChildValue(const std::string &name, const long &value)
				{ SetChildBasicValue(name, value); }
			void SetChildValue(const std::string &name, const short &value)
				{ SetChildBasicValue(name, value); }
			void SetChildValue(const std::string &name, const float &value)
				{ SetChildBasicValue(name, value); }
			void SetChildValue(const std::string &name, const double &value)
				{ SetChildBasicValue(name, value); }
			void SetChildValue(const std::string &name, const bool &value)
				{ SetChildBasicValue(name, value); }
			void SetChildValue(const std::string &name, const std::string &value)
				{ SetChildValueString(name, String(value)); }
			void SetChildValue(const std::string &name, const String& value)
				{ SetChildValueString(name, value); }

			XMLElm AddChild(const std::string &name)
				{
					OpenSOAPXMLElmPtr pelm = 0;

					Try(OpenSOAPXMLElmAddChildMB(pElm, name.c_str(), &pelm));
					return XMLElm(pelm);
				}

			void GetChildValue(const std::string &name, Structure& t) const
				{
					t.ParseMessage(GetChild(name));
				}

			template <class T> void GetChildValue(const std::string &name, std::vector<T>& ar) const
				{
					XMLElm elm = GetChild(name);
					ar.clear();
					while(elm.Good()) {
						T t;
						elm.GetValue(t);
						ar.push_back(t);
						elm = GetNextChild(elm);
					}
				}

			virtual XMLElm GetChild(const std::string& name) const;

			virtual
				XMLElm
				GetNextChild(const XMLElm& cld) const
				{
					OpenSOAPXMLElmPtr elm = cld.pElm;

					Try(OpenSOAPXMLElmGetNextChild(pElm, &elm));
					return XMLElm(elm);
				}

			virtual
				XMLElm
				GetFirstChild() const
				{
					OpenSOAPXMLElmPtr elm = 0;

					Try(OpenSOAPXMLElmGetNextChild(pElm, &elm));
					return XMLElm(elm);
				}

			template <class T> void GetBasicValue(T &value) const
				{
					std::string type = GetTypeName(value);
		
					Try(OpenSOAPXMLElmGetValueMB(pElm, type.c_str(),
												 reinterpret_cast<void **>(&const_cast<T &>(value))));
				}	
			void GetValue(long& v) const { GetBasicValue(v); }
			void GetValue(short& v) const { GetBasicValue(v); }
			void GetValue(float& v) const { GetBasicValue(v); }
			void GetValue(double& v) const { GetBasicValue(v); }
			void GetValue(bool& v) const { GetBasicValue(v); }
			void GetValue(String &value) const
				{
					value = GetValueString();
				}
			void GetValue(std::string &value) const
				{
					value = static_cast<std::string>(GetValueString());
				}
			void GetValue(Structure &value) const
				{
					value.ParseMessage(*this);
				}

			template <class T> void GetChildBasicValue(const std::string &name, T &value) const
				{
					std::string type = GetTypeName(value);

					Try(OpenSOAPXMLElmGetChildValueMB(pElm, name.c_str(), type.c_str(),
													  reinterpret_cast<void **>(&const_cast<T &>(value))));
				}
			void GetChildValue(const std::string &name, long &value) const
				{		GetChildBasicValue(name, value);	}
			void GetChildValue(const std::string &name, short &value) const
				{		GetChildBasicValue(name, value);	}
			void GetChildValue(const std::string &name, float &value) const
				{		GetChildBasicValue(name, value);	}
			void GetChildValue(const std::string &name, double &value) const
				{		GetChildBasicValue(name, value);	}
			void GetChildValue(const std::string &name, bool &value) const
				{		GetChildBasicValue(name, value);	}
			void GetChildValue(const std::string &name, String &value) const
				{
					value = GetChildValueString(name);
				}
			void GetChildValue(const std::string &name, std::string &value) const
				{
					XMLElm elm = GetChild(name);
					if(elm.Good())
						elm.GetValue(value);
				}

		protected:
			OpenSOAPXMLElmPtr pElm;



			bool bAddTypeName;
		};
}; //COpenSOAP
#endif // !defined(OpenSOAP_CPPLIB_OPENSOAPXMLELM_H)
