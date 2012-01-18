/*-----------------------------------------------------------------------------
 * $RCSfile: Helloc.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <iostream>
#include "OpenSOAPInitializer.h"
#include "OpenSOAPMethod.h"
#include "OpenSOAPStructure.h"

#include "Hello.hpp"

using namespace COpenSOAP;

typedef ClientMethod <Hello_in, Hello_out, Hello_method> Hello;

int 
main(int argc, char* argv[])
{
	COpenSOAP::Initializer soap_initializer;

	Hello he;
	he.SetEndpoint("http://localhost/cgi-bin/hellos.cgi");
	
	if(argc == 2){
		he.in.name = argv[1];
	}else {
		he.in.name = "dora";
	}

	try{
		he.PrintEnvelopeTo(&cout);
		he.Invoke();
	}catch(opensoap_failed e){
		cout << "error " << hex << e.GetErrorCode() 
			<< "@"<< e.GetObjectName() << endl;
	}
	cout << "Reply: " << he.out.reply << endl;

	return 0;
}
