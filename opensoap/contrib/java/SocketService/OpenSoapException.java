//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX Server Side Service
//  MODULE      :  OpenSoapException.java
//  ABSTRACT    :  OpenSoapException Class
//  DATE        :  2002.02.20
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
    public void writeTo(CharArrayWriter payload) throws IOException {
        payload.write("<?xml version='1.0'?>");
        payload.write("<SOAP-ENV:Envelope xmlns:SOAP-ENV='");
        payload.write(OpenSoapConstants.SOAPENV_URI);
        payload.write("'><SOAP-ENV:Body>");
        payload.write("<SOAP-ENV:Fault><faultcode>SOAP-ENV:");
        payload.write( code);
        payload.write("</faultcode><faultstring>");
        payload.write(getMessage());
        payload.write("</faultstring></SOAP-ENV:Fault>");
        payload.write("</SOAP-ENV:Body></SOAP-ENV:Envelope>");
    }
}