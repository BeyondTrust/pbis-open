/*-----------------------------------------------------------------------------
 * $RCSfile: HelloService.c,v $
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
	
	OpenSOAPStringPtr myName = NULL;
	size_t myNameLen = 0;
	char *myNameString = NULL;
	static char noNameString[] = "who are you?";
	
	OpenSOAPStringPtr reply = NULL;
	size_t replyLen = 0;
	char *replyString = NULL;
	
	/* parse request message */
	
	OpenSOAPEnvelopeGetBodyBlockMB(request, "Hello", &body);
	OpenSOAPBlockGetChildValueMB(body, "MyName", "string", &myName);
	OpenSOAPStringGetLengthMB(myName, &myNameLen);
	if (myNameLen == 0) {
		myNameString = malloc(strlen(noNameString) + 1);
		strcpy(myNameString, noNameString);
	}
	else {
		myNameString = malloc(myNameLen + 1);
		OpenSOAPStringGetStringMB(myName, &myNameLen, myNameString);
	}
	fprintf(stderr, "HelloService: my name = %s\n", myNameString);
	
	/* make response */
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response, "HelloResponse", &body);
	OpenSOAPBlockSetNamespaceMB(body,
								"http://services.opensoap.jp/samples/Hello/",
								"m");

	replyLen = strlen(myNameString);
	replyString = malloc(replyLen + 9);
	sprintf(replyString, "Hello, %s!", myNameString);
	fprintf(stderr, "HelloService: reply = %s\n", replyString);
	
	OpenSOAPStringCreateWithMB(replyString, &reply);
	OpenSOAPBlockSetChildValueMB(body, "Reply", "string", &reply);
	
	free(myNameString);
	free(replyString);
	
	return 0;
}

int
main(void) {
	
	OpenSOAPServicePtr service = NULL;
	
	OpenSOAPInitialize(NULL);
	
	OpenSOAPServiceCreateMB(&service, "HelloService", CONNECT_TYPE, 0);
	
	OpenSOAPServiceRegisterMB(service, "Hello", ServiceFunc, NULL);
	
	OpenSOAPServiceRun(service);
	
	OpenSOAPServiceRelease(service);
	
	OpenSOAPUltimate();
	
	return 0;
}
