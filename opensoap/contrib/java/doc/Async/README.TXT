OpenSOAP For Java Sample For ASYNC Version README

--------------------------------------------------------------------------------
[Contents]
1. Introduction
2. Required Modules
 2.1 Java2SE Installation
 2.2 sax Installation
3. Module Descriptions
 3.1 sax
  3.1.1 Module Listing
 3.2 Using The GUI
4. Other
 4.1 Build Batch Files
 4.2 Execute Batch Files

--------------------------------------------------------------------------------
1. Introduction
     It is required to have Java2(JDK1.2 or above) to run this project.
     Also, a SAX2.0 compatible XML Server is needed. (This project uses Xerces).
     For more details on other required modules see [2. Required Modules] and
     install the necessary modules as needed.
--------------------------------------------------------------------------------
2. Required Modules
 2.1 Java2SE Installation
     Use the links
     http://www.java.sun.com/jse/1.3/ja/index.html
     http://www.java.sun.com/j2se/1.3/ja/install-linux-sdk.html
     to download and install the Java2SE.
     Set the environment variables as follows:
       Add the PATH of JavaSE to JAVA_HOME (eg. /usr/java/jdk1.3.1_01)
       Add the location of bin to the JAVA_HOME PATH (eg. /usr/java/jdk1.3.1_01/bin)
     Caution: For WindowsNT/2000 do not use run "autoexec.bat". Instead, right-click
       [My Computer], select [Properties]->[Environment] and register the variables.

 2.2.sax Installation
      Apache Xerces XML Parcer 1.2.3 http://xml.apache.org/cerces-j
--------------------------------------------------------------------------------
3. Module Descriptions
 3.1 sax
  3.1.1 Module Listing
   3.1.1.1 OpenSoapConstants.java   Defines OpenSOAP Constants
     This class defines all the constants used by this project.
   3.1.1.2. XMLWriter.java          XML Syntax Correction Class
     This class performs escape processing on reserved characters ('<', '&', etc).
   3.1.1.3. OpenSoapException.java  OpenSOAP Error Processs Class
     This class manages faultcode/faultstring.
   3.1.1.4. OpenSoapEnvelope.java   SOAP Message Envelope Process Class
     This class parses the envelope.
     This class inherits XMLFilterImpl.
   3.1.1.5. OpenSoapRequest.java    Basic SOAP Message Process Class
     This class implements the SOAP protocol for requests.
     This class prepares the request message, transmits the OpenSOAPEnvelope over
     and decodes the response.
   3.1.1.6. CalcClientRequest.java  Calc Service Message Process Class
     This class inherits the OpenSoapRequest.
     This class generates the request message for the OpenSOAP Server, and gets the transaction ID.
   3.1.1.7. CalcClientResult.java   Calc Service Message Process Receive Class
     This class inherits the OpenSOAPRequest.
     Also, the transaction ID is sent to the OpenSOAP Server and the transaction result is received.
   3.1.1.8. CalcClientPanel.java    User GUI Setting, Event Process Class
     The class that supports the client side user interface.
   3.1.1.9. CalcClient.java         Main process Class
     The Main class on the client side.

 3.3.GUI Usage Description
     The following text graphic is a rough representation of  the user interface
     used by the Calc example. The numbers in parentheses () represent the various
     input and output points of the interface. A listing describing each I/O point
     follows the graphic.
     ----------------------------------------------------
     | Server                   [---------(1)----------]|
     ----------------------------------------------------
     |[--(2)--][(3)|\/][--(4)--][ProcessRegistration(5)]|
     ----------------------------------------------------
     |Process No:  ---------------(6)-----------------  |
     ----------------------------------------------------
     |                          [GetProcessRESULT(7)]   |
     ----------------------------------------------------
     |Answer:      ---------------(8)-----------------  |
     ----------------------------------------------------
     |             ---------------(9)-----------------  |
     ----------------------------------------------------
     (1) Server URI Input
         The OpenSOAP Server Interface name is input here.
         As an example, if the server's URL(IP Address) is 133.50.1.90
         and the interface is /cgi-bin/soapInterfase.cgi, then this becomes
         http://133.50.1.90/cgi-bin/soapInterfase.cgi .
     (2)(4) Numerical values 1, 2
         Input values for the calculation service.
     (3) Arithmetic Function Select(Pull-down Menu)
         Add       Addition
         Subtract  Subtraction
         Multiply  Multiplication
         Divide    Division
         Fault     Generate a FAULT message.
         The selected arithmetic function is apploed to the
         values entered in (2) and (4).
     (5) Execute Button
         On clicking this button, a SOAP request message is sent to the service.
         A transaction ID is returne as a response.
     (6) Transaction ID Display Area
         While the above transaction is taking place the transaction ID is displayed here.
     (7) Result Retrieval Button
         On clicking this button, the transaction ID is sent to the server, and the response
         is retrieved.
     (8) Result Display (Arithmetic Function Result)
         The result of the arithmetic function(The Service) applied to the input
         data is displayed here. In this case the answer to the calculation.
     (9) Overall status display (Process Result)
         Checking : Request Processing, Response Processing
         Blank    : Normal Processing Complete
--------------------------------------------------------------------------------
4. Other
 4.1 Build Batch Files
    For Windows use SAXClientBuild.bat.
    For Unix use SAXClientBuild.sh.
 4.2 Execution Batch Files
    For Windows use SaxClientExec.bat.
    For Unix use SaxClientExec.sh.
