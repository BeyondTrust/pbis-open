/*-----------------------------------------------------------------------------
 * $RCSfile: SoapingService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

#include <stdio.h>
#include <string.h>

#if !defined(CONNECT_TYPE)
# define CONNECT_TYPE "stdio"
#endif /* CONNECT_TYPE */

static
int
ServiceFunc(OpenSOAPEnvelopePtr request,
			OpenSOAPEnvelopePtr *response,
			void *opt) {
	
	OpenSOAPBlockPtr body = NULL;
	
	OpenSOAPStringPtr myString = NULL; 
	long time_sec = 0;
	long time_usec = 0;
	
	/* parse request message */
	OpenSOAPStringCreate(&myString);
	OpenSOAPEnvelopeGetBodyBlockMB(request, "Soaping", &body);
	OpenSOAPBlockGetChildValueMB(body, "TimeSec", "int", &time_sec);

	OpenSOAPEnvelopeGetBodyBlockMB(request, "Soaping", &body);
	OpenSOAPBlockGetChildValueMB(body, "TimeUSec", "int", &time_usec);

	OpenSOAPEnvelopeGetBodyBlockMB(request, "Soaping", &body);
	OpenSOAPBlockGetChildValueMB(body, "SendString", "string", &myString);

	/* make response */
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response, "SoapingResponse", &body);
	OpenSOAPBlockSetNamespaceMB(body,
				    "http://services.opensoap.jp/Soaping/",
				    "m");
	OpenSOAPBlockSetChildValueMB(body, "ReplyTimeSec", "int", &time_sec);
	OpenSOAPBlockSetChildValueMB(body, "ReplyTimeUSec", "int", &time_usec);
	OpenSOAPBlockSetChildValueMB(body, "ReplyString", "string", &myString);

	OpenSOAPStringRelease(myString);
	return 0;
}

int
main(void) {
	
	OpenSOAPServicePtr service = NULL;
	
	OpenSOAPInitialize(NULL);
	
	OpenSOAPServiceCreateMB(&service, "SoapingService", CONNECT_TYPE, 0);
	
	OpenSOAPServiceRegisterMB(service, "Soaping", ServiceFunc, NULL);
	
	OpenSOAPServiceRun(service);
	
	OpenSOAPServiceRelease(service);
	
	OpenSOAPUltimate();
	
	return 0;
}
