/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPService.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/Service.h>
#include "OpenSOAPService.h"

namespace COpenSOAP {
Service::Service(const std::string& service_name, const std::string& connect_type, bool is_loop)
{
	OpenSOAPServiceCreateMB(&pService, service_name.c_str(), connect_type.c_str(), is_loop); 
}

Service::~Service()
{
	OpenSOAPServiceRelease(pService);
}

void Service::Run()
{
	OpenSOAPServiceRun(pService);
}
}; //COpenSOAP

