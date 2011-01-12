Hello Sample
============

This directory contains the Hello Client and Service sample code. 

After the installation of the OpenSOAP API, use "make -f Hello.mak" to
compile and generate the following files:

* HelloClient
  Hello Client program. By changing the 2nd parameter of
  OpenSOAPTransportSetURL() in HelloClient.c, you can select the
  Service (HelloService or HelloService.cgi) on the OpenSOAP Server.

* HelloService
  The Hello Service program using standard I/O. Used with inetd, this
  can be a socket type service.

* HelloService.cgi
  CGI type Hello Service program. To be used, this should be copied to
  a directory from which it is possible to run cgi applications.

Please refer to Hello.mak for instructions on how to run the samples.
