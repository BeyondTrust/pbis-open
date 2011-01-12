/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSoapRequest.java,v $
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
//  MODULE      :  OpenSoapRequest.java
//  ABSTRACT    :  Request & Response For OpenSoap Message
//  DATE        :  2002.01.18
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8
import java.io.*;
import java.net.*;
import java.util.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public abstract class OpenSoapRequest
   extends DefaultHandler
{
   protected String soapAction;

   public OpenSoapRequest(String soapAction)
   {
      if(null == soapAction)
         soapAction = "\"\"";
      this.soapAction = '"' + soapAction + '"';
   }

   public void invoke(URL server)
      throws IOException, OpenSoapException, SAXException
   {
      HttpURLConnection conn =
         (HttpURLConnection)server.openConnection();
      conn.setDoOutput(true);
      conn.setDoInput(true);

      CharArrayWriter payload = new CharArrayWriter();
      payload.write("<?xml version='1.0'?>");
      payload.write("<SOAP-ENV:Envelope xmlns:SOAP-ENV='");
      payload.write(OpenSoapConstants.SOAPENV_URI);
      payload.write("'>");
      // if you hope "Header" then insert program
      //payload.write("<SOAP-ENV:Header>");
      //payload.write("</SOAP-ENV:Header>");
      payload.write("<SOAP-ENV:Body>");
      writeRequest(new XMLWriter(payload));
      payload.write("</SOAP-ENV:Body></SOAP-ENV:Envelope>");

      conn.setRequestProperty("Content-Length",
                              String.valueOf(payload.size()));
      conn.setRequestMethod("POST");
      conn.setFollowRedirects(true);
      conn.setRequestProperty("Content-Type","text/xml");
      conn.setRequestProperty("SOAPAction",
                              '"' + soapAction + '"');
      Writer writer =
         new OutputStreamWriter(conn.getOutputStream(),"UTF-8");
      payload.writeTo(writer);
      writer.flush();

      conn.connect();

      XMLReader xmlReader =
         XMLReaderFactory.createXMLReader(OpenSoapConstants.SAXPARSER);
      xmlReader.setFeature(OpenSoapConstants.SAXNAMESPACES,true);
      OpenSoapEnvelope soapEnvelope = new OpenSoapEnvelope();
      soapEnvelope.setParent(xmlReader);
      soapEnvelope.setContentHandler(this);
      InputSource is = new InputSource(conn.getInputStream());
      soapEnvelope.parse(is);
   }

   public abstract void writeRequest(XMLWriter writer)
      throws IOException;
}
