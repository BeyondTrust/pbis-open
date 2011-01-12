/*-----------------------------------------------------------------------------
 * $RCSfile: Hellos.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include "OpenSOAPInitializer.h"
#include "OpenSOAPMethod.h"

#include "Hello.hpp"

//#include <clocale>

typedef ServiceMethod<Hello_in, Hello_out, Hello_method> Hello;

void Hello::ExecuteService()
{
	out.reply = "Hello! " + in.name;
}

using namespace COpenSOAP;

int 
main(int argc, char* argv[])
{	
	Initializer soap_initializer; 
//	setlocale(LC_CTYPE, "C");
/*		Service srv("HELLO", "cgi", 0);
		Hello he;
		srv.Register(he);
		srv.Run();
	*/
	try
	{
		Hello he;
		he.Run();
	}
	catch(opensoap_failed e)
	{
		cerr << "errorcode:" << e.GetErrorCode() 
		     <<"@"<< e.GetObjectName() << endl;
	}

	return 0;
}
