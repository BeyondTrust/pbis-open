/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSoapException.java,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  OpenSoapException.java
//  ABSTRACT    :  OpenSoapException Class
//  DATE        :  2002.01.18
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

   public OpenSoapException(String code,String string) {
      super(string != null ? string : "Unknown error");
      this.code = code;
   }

   public String getCode() {
      return code;
   }

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