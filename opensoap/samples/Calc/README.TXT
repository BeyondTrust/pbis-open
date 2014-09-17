Simple Arithmetic Calculation Sample
====================================

This directory contains the sample code for the client and service
programs of a simple arithmetic service.
Note : This sample uses http://tempuri.org/message/ as the Service
       Namespace so that it can be verified with the MS Soap Toolkit.

Using this sample:
After installation of the OpenSOAP API, compile with the command
  make clean; make -f Calc.mak
to generate the program files listed below.
Next, as described in the Calc.mak file, use
  make -f Calc.mak service-install
to install the service program, the SSML file, etc.
Edit Calc.mak and set the value of MSSOAPSAMPLESERVER to the machine
where the MS Soap Toolkit is installed and running. Use
  make -f Calc.mak ctest1
to verify connection to the MS Soap Toolkit.

* CalcClient
  Simple arithmetic client program.

* CalcService
  Simple arithmetic service program utilizing standard I/O. By using inetd,
  it is possible to make this a socket type service.

* CalcService.cgi
  CGI type simple arithmetic service program.
