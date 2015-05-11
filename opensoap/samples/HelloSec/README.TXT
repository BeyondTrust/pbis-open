HelloSec Sample
===============


[1] Outline:

  This directory contains an example of how to use the security concepts of
  OpenSOAP in a Web Service application program. The HelloSec sample is used
  to demonstrate these features and the code for the client and service programs
  can be found in this directory.
  
  The HelloSec Web Service accepts the character string specified by the parameter
  MyName of the Hello method, and returns the character string "Hello (character string)!"
  in the Reply parameter.
  
  For request or response messages, a digital signature is applied after the
  parameters have been encrypted.
  
  For received request or response messages, decryption of parameters is
  carried out after the signature has been verified.
  
  Please refer to the files hello_sec_request.xml and hello_sec_response.xml
  to see examples of actual transmitted request and response messages.
  
  Encryption, Decryption, Signature Attachment and Signature Verification make use of
  the Key files as described below.
  
  Key File Name             Meaning
  ------------------------  --------------------
  privKey_HelloClient.pem   Client Private Key
  pubKey_HelloService.pem   Service Public Key    
  pubKey_HelloClient.pem    Client Private Key
  privKey_HelloService.pem  Service Public Key   

  Specifically, each key usage is as described below:

  Used by function            Program          Purpose                 Key File Name
  ------------------------    -------------    --------                ------------------------

  Generate Request Message    Client           Encryption              pubKey_HelloService.pem
                                               Signature Attachment    privKey_HelloClient.pem

  Analyze Request Message     Service          Signature Verification  pubKey_HelloClient.pem
                                               Decryption              privKey_HelloService.pem

  Generate Response Message   Service          Encryption              pubKey_HelloClient.pem
                                               Signature Attachment    privKey_HelloService.pem

  Analyze Response Message    Client           Signature Verification  pubKey_HelloService.pem
                                               Decryption              privKey_HelloClient.pem


[2] Making the executable files :

  After installing the OpenSOAP API, execute the following commands:
    make clean
    make -f HelloSec.mak
  The following files will be created.

  * HelloSecClient
    The HelloSec client program.
  
  * HelloSecService
    The HelloSec service program utilizing standard I/O.
  
  * HelloSecService.cgi
    CGI type HelloSec service program. Place this executable in a directory in which
    it is possible to run CGI scripts.


[3] Installation :

  The program files, configuration files, and the data files MUST be
  installed in their appropriate directories.
  
  For most environments, simply executing the command
    make -f HelloSec.mak install
  will correctly install the various components in their proper places.
  
  However, for proper installation of the CGI service programs, it may
  be necessary to specify where CGI scripts are to be placed using the
  CGI_BIN_DIR variable of the make file.
  
  In HelloSec.mak, the files to be installed and their respective locations
  are listed below:
  
  Installed File              Install Directory
  --------------------------  ---------------------------
  HelloSecService.cgi         /home/httpd/cgi-bin
  HelloSecService             /usr/local/opensoap/services
  HelloSec.ssml               /usr/local/opensoap/conf/ssml
  privKey_HelloService.pem    /usr/local/opensoap/services/HelloSec
  pubKey_HelloClient.pem      /usr/local/opensoap/services/HelloSec
  
  For more details please refer to the HelloSec.mak file.


[4] Execution :
  
  For details on executing the samples, please refer to HelloSec.mak.
  Examples and test rules are described at the end of the file.
  
  For example, the rule "ctest" can be executed as illustrated below.
  
  make -f HelloSec.mak ctest
  
  The test rules are explained here.
  
  * stest (Service standalone test)
    This performs a simple test of the CGI version of the service program by
    sending a previously formed request message(text file) to the service program.
    
  * ctest (Client/Service connectivity test)
    The client program issues a test request to the CGI version of the service.
    
  * cstest (Client/Server/Service connectivity test)
    The client issues a test request to the OpenSOAP Server, which in turn issues a request
    to the standard I/O version of the service.
    For this test to carried out properly, it is necessary that the OpenSOAP Server
    be started with the following command:
      opensoap-server-ctl start


