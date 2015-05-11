//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  OpenSoapException.java
//  ABSTRACT    :  OpenSoapException Class
//                 [ ASYNC. Version ]
//  DATE        :  2002.02.01
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8
//--- Package Name -----------------------------------------------------------//
import java.io.*;
import org.xml.sax.*;

public class OpenSoapException extends SAXException {
    protected String code;

    //--------------------------------------------------------------------------
    // Common Open SOAP Exception
    //  Input Fault Code
    //        Fault Message
    //--------------------------------------------------------------------------
    public OpenSoapException(String code,String string) {
        super(string != null ? string : "Unknown error");
        this.code = code;
    }

    //--------------------------------------------------------------------------
    // Get Fault Code
    //--------------------------------------------------------------------------
    public String getCode() {
        return code;
    }

    //--------------------------------------------------------------------------
    // Make Fault Message
    //  Input XMLWriter
    //--------------------------------------------------------------------------
    public void writeTo(XMLWriter writer) throws IOException {
        writer.write("<?xml version='1.0'?>");
        writer.write("<SOAP-ENV:Envelope xmlns:SOAP-ENV='");
        writer.write(OpenSoapConstants.SOAPENV_URI);
        writer.write("'><SOAP-ENV:Body>");
        writer.write("<SOAP-ENV:Fault><faultcode>SOAP-ENV:");
        writer.escape(code);
        writer.write("</faultcode><faultstring>");
        writer.escape(getMessage());
        writer.write("</faultstring></SOAP-ENV:Fault>");
        writer.write("</SOAP-ENV:Body></SOAP-ENV:Envelope>");
    }
}