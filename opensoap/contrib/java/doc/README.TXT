OpenSOAP For Java Sample README

--------------------------------------------------------------------------------
[Contents]
1. Introduction
2. Required Modules
 2.1 Java2SE Installation
 2.2 Using ApacheSOAP
 2.3 Using sax
3. Module Descriptions
 3.1 Using ApacheSOAP
  3.1.1 Module Listing
  3.1.2
 3.2 Using sax
  3.2.1 Module Listing
4. Other Issues
 4.1 Build Batch File
 4.2 Execution Batch File

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

 2.2 Using ApacheSOAP
      Apache Xerces XML Parcer 1.2.3 http://xml.apache.org/cerces-j
      Apache SOAP 2.0(2.2)           http://xml.apache.org/soap
      JavaMail(1.2)                  http://java.sun.com/products/javamail/
      JavaBeens(1.0.1) Activeation Framework
                          = http://java.sun.com/products/javabeans/glasgow/jaf.html
      jaf(http://java.sun.com/products/beans/glasgow/jaf.html)
          Add xerces.jar, soap.jar, mail.jar,activation.jar to CLASSPATH.

 2.3 Using sax
      Apache Xerces XML Parcer 1.2.3 http://xml.apache.org/cerces-j
--------------------------------------------------------------------------------
3. Module Descriptions
 3.1 Using ApacheSOAP
  3.1.1 Module Listing
     CalcClient.java         Main Process Class
     CalcClientPanel.java    User GUI Setting, Event Process Class
     CalcClientRequest.java  SOAP Message Transmission Process Class
   3.1.1.1. CalcClient.java
     A GUI Panel (Using CalcClientPanel.java) has been prepared.
   3.1.1.2 CalcClientPanel.java
     This class sets the event to be triggered when the button is clicked.
     When an event is triggered (when using CalcClientRequest.java) a SOAP
     message is transmitted.
   3.1.1.3. CalcClientRequest.java
     This class obtains the Server URL from the GUI Panel, and from the other GUI
     parameters creates the message Elements, and invokes using ApacheSOAP(SOAP.jar).
     Caution: In the following Message example, the Result type is not defined.
             When received by soap.jar, "Not Found Deserializer" error occurs, and an
             Exception is thrown. In this class by using the following mapping this is
             circumvented.
        SOAPMappingRegistry smr = new SOAPMappingRegistry ();
        StringDeserializer sd = new StringDeserializer ();
        smr.mapTypes (Constants.NS_URI_SOAP_ENC,
                      new QName ("", "Result"), null, null, sd);

  3.1.2 Message Example
   3.1.2.1 Request
     POST /calc HTTP/1.0
     Host: localhost
     Content-Type: text/xml; charset=utf-8
     Content-Length: 478
     SOAPAction: "http://services.opensoap.jp/samples/Calc/"

     <?xml version='1.0' encoding='UTF-8'?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
                        xmlns:xsi="http://www.w3.org/1999/XMLSchema-instance"
                        xmlns:xsd="http://www.w3.org/1999/XMLSchema">
       <SOAP-ENV:Body>
         <ns1:Add xmlns:ns1="http://services.opensoap.jp/samples/Calc/"
                  SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
           <A xsi:type="xsd:int">2</A>
           <B xsi:type="xsd:int">5</B>
         </ns1:Add>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.1.2.2 Response
     <?xml version="1.0" encoding="utf-8"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
       <SOAP-ENV:Body>
         <m:AddResponse xmlns:m="http://services.opensoap.jp/samples/Calc/">
           <Result>2.000000</Result>
         </m:AddResponse>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.1.2.3 Fault Message
     HTTP/1.1 200 OK
     Content-Type: text/xml; charset="utf-8"
     Content-Length: 388

     <?xml version="1.0"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
                        SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
       <SOAP-ENV:Body>
         <SOAP-ENV:Fault>
           <faultcode>OPENSOAP:ffffffff</faultcode>
           <faultstring>OPENSOAP Service error</faultstring>
           <detail>OPENSOAP Service Internal error</detail>
         </SOAP-ENV:Fault>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

 3.2 Using sax
  3.2.1 Module Listing
     CalcClient.java         Main process Class
     CalcClientPanel.java    User GUI Setting, Event Process Class
     CalcClientRequest.java  Calc Service Message Process Class
     OpenSoapConstants.java  Defines OpenSOAP Constants
     OpenSoapEnvelope.java   SOAP Message Envelope Process Class
     OpenSoapRequest.java    Basic SOAP Message Process Class
     OpenSoapException.java  OpenSOAP Error Processs Class
     XMLWriter.java          XML Syntax Correction Class
   3.2.1.1 OpenSoapConstants.java
     This class defines all the constants used by this project.
   3.2.1.2 XMLWriter.java
     This class performs escape processing on reserved characters ('<', '&', etc).
   3.2.1.3 OpenSoapException.java
     This class manages faultcode/faultstring.
   3.2.1.4 OpenSoapEnvelope.java
     This class parses the envelope.
     This class inherits XMLFilterImpl.
   3.2.1.5 OpenSoapRequest.java
     This class implements the SOAP protocol for requests.
     This class prepares the request message, transmits the OpenSOAPEnvelope over
     and decodes the response.
   3.2.1.6 CalcClientRequest.java
     This class inherits the OpenSoapRequest.
   3.2.1.7 CalcClientPanel.java
     The class that supports the client side user interface.
   3.2.1.8 CalcClient.java
     The Main class on the client side.

  3.2.2 Message Example
   3.2.2.1 Request
     POST /calc HTTP/1.1
     Content-Length: 306
     Content-Type: text/xml
     SOAPAction: ""http://services.opensoap.jp/samples/Calc/""
     User-Agent: Java1.3.0
     Host: localhost:8070
     Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2
     Connection: keep-alive

     <?xml version='1.0'?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV='http://schemas.xmlsoap.org/soap/envelope/'>
       <SOAP-ENV:Body>
         <ns1:Add xmlns:ns1='http://services.opensoap.jp/samples/Calc/'
                  SOAP-ENV:encodingStyle='http://schemas.xmlsoap.org/soap/encoding/'>
           <A>1</A>
           <B>1</B>
         </ns1:Add>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.2.2.2 Response
     <?xml version="1.0" encoding="UTF-8"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
       <SOAP-ENV:Body>
         <m:AddResponse xmlns:m="http://services.opensoap.jp/samples/Calc/">
           <Result>2.000000</Result>
         </m:AddResponse>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

   3.2.2.3 Fault Message
     HTTP/1.1 200 OK
     Content-Type: text/xml
     Content-Length: 388

     <?xml version="1.0"?>
     <SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
                        SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
       <SOAP-ENV:Body>
         <SOAP-ENV:Fault>
           <faultcode>OPENSOAP:ffffffff</faultcode>
           <faultstring>OPENSOAP Service error</faultstring>
           <detail>OPENSOAP Service Internal error</detail>
         </SOAP-ENV:Fault>
       </SOAP-ENV:Body>
     </SOAP-ENV:Envelope>

 3.3 Client User Interface
     The following text graphic is a rough representation of  the user interface
     used by the Calc example. The numbers in parentheses () represent the various
     input and output points of the interface. A listing describing each I/O point
     follows the graphic.
     --------------------------------------------------
     | Server [ (1)                         ]         |
     --------------------------------------------------
     | [(2)]     [(3)\/]     [(4)]    [(5)]   (6)     |
     --------------------------------------------------
     | [(7)]                                          |
     --------------------------------------------------
     (1) Server URI Input
         The input is of the form (Server URL):(Port No.).
         As an example, if the server's URL(IP Address) is 133.50.1.90
         and the service port number is 8764, then the URI input here
         is http://133.50.1.90:8764
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
         On clicking this button, a SOAP message is sent to the service
         and a response is returned.
     (6) Result Display (Arithmetic Function Result)
         The result of the arithmetic function(The Service) applied to the input
         data is displayed here. In this case the answer to the calculation.
     (7) Overall status display (Process Result)
         Checking : Request Processing, Response Processing
         Blank    : Normal Processing Complete
--------------------------------------------------------------------------------
4. Other Issues
 4.1 Build Batch Files
  4.1.1 Using ApacheSOAP
        For Windows use ApacheSOAPBuild.bat.
        Make the appropriate modifications according to the descriptions in the
        file to the environment being used.
  4.1.2 Using sax
        For Windows use SAXClientBuild.bat.
        Make the appropriate modifications according to the descriptions in the
        file to the environment being used.
 4.2 Execution Batch Files
  4.1.1 Using ApacheSOAP
        For Windows use ApacheSOAPExec.bat.
        For Unix use ApacheSOAPExec.sh.
  4.1.2 Using sax
        For Windows use SaxClientExec.bat.
        For Unix use SaxClientExec.sh.
  4.1.3 ApacheSOAP Attachment Tunnel Monitor
        For Windows use Tunnel.bat.
        For Unix use Tunnel.sh.
        Start up the TCP Tunnel Monitor that is included when ApacheSOAP is installed.
        To run normally, execute the following command:
        >java org.apache.soap.util.net.TcpTunnelGui 8070 localhost 8080
        This description applies only to the Batch File .
        In this case a tunnel is opened between ports 8070 and 8080 to allow monitoring, and
        these settings should be changed according to the system environment settings being used.

Supplement : If a message similar to "Insufficient Resources For Environment Variables"
             is displayed when the batch file is executed(DOS) it means there is not
             enough available memory for the Environment Variables. In this case use
             the following procedure to set the Environment Variables:
             (1) Open a DOS Prompt Window
             (2) Simultaneously press the ALT and Space keys to display the pull-down menu
             (3) Select Properties
             (4) The DOS Prompt Properties Panel is displayed
             (5) Select the Memory Tag
             (6) Select a larger value in the Environment Variable Initial Size(V) Pull-down Menu
             (7) Click on OK
             (8) Close the DOS Prompt and reboot

