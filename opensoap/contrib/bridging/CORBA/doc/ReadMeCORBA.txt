CORBA Bridging

1. OpenSOAP CORBA Bridging Implementation :

1.1 Define the CORBA IDL interface.

1.2 Compile the IDL files using omniidl of the IDL compiler.
    A .cc file and .hh file for the interface will be generated.

1.3 Develop the CORBA Object server program.

1.4 Develop the CORBA Object client program for the bridging interface (namely, the OpenSOAP service).

1.5 Develop the OpenSOAP client program that uses the bridging interface.


2. Please refer to CORBAExeCalc and CORBADllCalc in opensoap/bridging/CORBA/
  to see an example of OpenSOAP CORBA bridging.

