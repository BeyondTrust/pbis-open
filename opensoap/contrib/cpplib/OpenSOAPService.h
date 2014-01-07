/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPService.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(OpenSOAP_CPPLIB_OPENSOAPSERVICE_H)
#define OpenSOAP_CPPLIB_OPENSOAPSERVICE_H

#include "OpenSOAPObject.h"
#include <OpenSOAP/Service.h>
#include <string>

namespace COpenSOAP {

class Service : public Object
{
public:
	std::string GetObjectName() const { return "Service"; }

	Service(const std::string& service_name, const std::string& connect_type, bool is_loop);
	virtual ~Service();

	template<class SERVICE> 
	void
	Register(SERVICE &method) {
		Try(OpenSOAPServiceRegisterMB(pService,
								  method.GetMethodName().c_str(),
								  method.ServiceFunc,
								  &method));
	}

	void Run();

private:
	OpenSOAPServicePtr pService;
};
}; // COpenSOAP
#endif // !defined(OpenSOAP_CPPLIB_OPENSOAPSERVICE_H)
