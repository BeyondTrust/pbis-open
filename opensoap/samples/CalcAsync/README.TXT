Synchronous/Asynchronous/Specified Hop Count/Specified Path Simple Arithmetic Calculation Sample
================================================================================================

This directory contains the source code for a simple arithmetic calculation
service and the corresponding client. The service options include
Synchronous, Asynchronous, Specified Forward Maximum Hop Count and specified
Forward Host Path.
The client program can be executed from either the command line or a web
browser.

With the OpenSOAP API properly installed, use
make -f CalcAsync.mak to compile and generate the following files:

* CalcAsyncClient
  Command line simple arithmetic client program.
* CalcAsyncApp.cgi
  CGI based simple arithmetice client program. This can be executed
  through a web browser.
* CalcAsyncService
  Standard I/O type simple arithmetic service program.
  By registering this service program in inetd, it can be used as a socket type service.
* CalcAsyncService.cgi
  CGI based simple arithmetic service program.

As root, use
make -f CalcAsync.mak install
to install the service programs, SSML file and the client prgrams.

If the SSML file is being installed for the first time, use
# opensoap-server-ctl reload
to restart the OpenSOAP Server and ensure the SSML file is properly registered.


Using the command line version of CalcAsyncClient

Usage: CalcAsyncClient [options] method [op_a] [op_b]
  method         Add[Async], Subtract[Async], Multiply[Async], Divide[Async]
                 or GetResult.
  op_a           first operand (except for GetResult)
  op_b           second operand (except for GetResult)

 [options]
  -s endpoint    SOAP service endpoint URI
  -a soapaction  value of SOAPAction HTTP-Header
  -c char_enc    character encoding of request message (default: UTF-8)

  (messaging options)
  -H hopcount             set hopcount (>= 0)
  -f path1[,path2[,...]]  set forward path by URI (Commas are not allowed)
                          e.g.http://host/cgi-bin/soapInterface.cgi
  -F forward_path_file    set forward path from input file

Operands op_a and op_b, and 1 of 4 arithmetic operations Add, Subtract, Multiply
and Divide are given on the command line.

$ ./CalcAsyncClient Add 1 2
  (Synchronous mode addition of 1, 2)

To execute the service in Asynchronous mode, append Async to the arithmetic
operation in question. In Asynchronous mode, the result is not returned
immediately, and a separate request must be sent to the server to retrieve
the result.

$ ./CalcAsyncClient MultiplyAsync 1 2 > tmp_msg
  (Asynchronous mode multiplication of 1, 2)
$ ./CalcAsyncClient GetResult < tmp_msg
  (Retrieval of the result corrsponding to the message ID of the previous request)

Using command-line options, the service endpoint can be specified
=================================================================
If the service is available on the host "hostname" running an OpenSOAP server, use this
option:
   -s http://hostname/cgi-bin/soapInterface.cgi

If the specified OpenSOAP server "hostname" does not have that service available, then
it will forward the message to another server.
How far the message will be forwarded is controlled by the -H option which limits
the maximum number of hops.

$ ./CalcAsyncClient -s http://host/cgi-bin/soapInterface.cgi -H 4 AddAsync 1 2
  (Server: If the server "host" does not support the service, hop up to 4 times)

Specifying the host to which the request will be forwarded to
=============================================================
The forward path can be read from a file (-F) or from the command line (-f).

$ cat forward_path.txt
http://host1/cgi-bin/soapInterface.cgi
http://host2/cgi-bin/soapInterface.cgi

$ ./CalcAsyncClient -s http://host/cgi-bin/soapInterface.cgi \
                    -F forward_path.txt  AddAsync 1 2
  (Using a file: If the server "host" does not support the service, forward to
   "host1" and then to "host2")

$ ./CalcAsyncClient -s http://host/cgi-bin/soapInterface.cgi \
  -F http://host1/cgi-bin/soapInterface.cgi,http://host2/cgi-bin/soapInterface.cgi \
  AddAsync 1 2
  (Using command lin option: As before if no service available from server "host"
   forward to "host1" and then "host2")

Simple test rules are incorporated into the CalcAsync.mak makefile.

$ make -f CalcAsync.mak stest1
  (Service test using a sample request message)
$ make -f CalcAsync.mak ctest1
  (Test of client program)
$ make -f CalcAsync.mak cstest1
  (Test of service/client communication)
$ make -f CalcAsync.mak cstest1a
$ make -f CalcAsync.mak cstestr
  (Test of service/client interaction in asynchronous mode.
   Use the cstestr option to retrieve the result.)


Similarly, the CGI based client can be checked using a web browser.
If default installation has been used the following URL runs the CGI client.
http://localhost/cgi-bin/CalcAsyncApp.cgi

To verify the SOAP messages sent and received by the client program, check
the "Display Request & Response messages" check-box.
