SimpleCalc Sample
=================


[1] Outline :

  This directory contains the source code for a simple arithmetic
  calculation service and client programs.
  
  The SimpleCalc service is comprised of the following 4 methods.
  
  Method      Meaning       Parameter1   Parameter2   Return Value
  ----------  --------      -----------  -----------  ------------
  Add         + (Add)       A            B            Result of A + B
  Subtract    - (Subtract)  A            B            Result of A - B
  Multiply    * (Multiply)  A            B            Result of A * B
  Divide      / (Divide)    A            B            Result of A / B
  
  Sample request/response messages for all methods can be found in their
  respective files in the directory SimpleCalcSampleMessages.
  
  
[2] Building the executables : 

  After the OpenSOAP API has been installed, use
    make clean
    make -f SimpleCalc.mak
  to compile and build the following programs.
  
  * SimpleCalcClient
    Simple arithmetice client program.
  
  * SimpleCalcService
    Simple arithmetice service program using standard I/O.
  
  * SimpleCalcService.cgi
    CGI type simple arithmetic service program. This should be copied to a
    directory where it is possible to execute cgi scripts.


[3] Installation :
  
  The program files, configuration files, etc. must be installed in their
  proper directories.
  
  In general the default install command
    make -f SimpleCalc.mak
  should install the components in their proper locations.
  
  However, to install the CGI program, the variable CGI_BIN_DIR in
  the make file should be set accordingly.
  
  Please refer to SimpleCalc.mak for further details.


[4] Execution :
  
  Please refer to SimpleCalc.mak for details on running the programs.
  At the end of this file are sample rulesets illustrating how to run
  the programs.
  
  The rule "ctest1", for example, can be executed as follows:
    
  make -f SimpleCalc.mak ctest1 
