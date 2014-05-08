OpenSOAP For Java Service README

--------------------------------------------------------------------------------
[Conentts]
1. Introduction
2. Service Specifications
3. Module Descriptions
4. Compilation

--------------------------------------------------------------------------------
1. Introduction
     It is required to have Java2(JDK1.2 or above) to run this project.
     Also, a SAX2.0 compatible XML Server is needed. (This project uses Xerces).
--------------------------------------------------------------------------------
2. Service Specifications
     This service performs 2 terms simple arithmetic. It also performs error (Fault) processing.
     This service also verifies the operation of the Java sample Sync SAX processing.
     (Please refer to the module descriptions for processing details)
     Service Details(Processing for the following items is available)
         Add       Addition
         Subtract  Subtraction
         Multiply  Multiplication
         Divide    Division
         Fault     Generate a FAULT message.
     Execution Method
         java SocketService [PortNumber]

--------------------------------------------------------------------------------
3. Module Descriptions
 3.1.OpenSoapConstants.java
     This class defines all the constants used by this project.
 3.2.OpenSoapException.java
     This class defines the Execptions used by the OpenSOAP Service.
 3.3.XMLWriter.java
     This class performs XML Syntax Correction.
 3.4.CalcServiceResponse.java
     This class parses the received XML.
 3.5.SocketService.java
     This class waits for an Accept on the specified port.
     It receives a SOAP message.

4. Compilation
  javac [-classpath] OpenSoapConstants.java
  javac [-classpath] XMLWriter.java
  javac [-classpath] OpenSoapException.java
  javac [-classpath] CalcServiceResponse.java
  javac [-classpath] SocketService.java

  Compilation should be carried out in the order list ed above.

