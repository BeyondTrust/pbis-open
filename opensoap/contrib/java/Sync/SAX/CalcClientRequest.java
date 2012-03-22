/*-----------------------------------------------------------------------------
 * $RCSfile: CalcClientRequest.java,v $
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
//  MODULE      :  CalcClientRequest.java
//  ABSTRACT    :  CalcClient for Body SOAP Message Proc.
//  DATE        :  2002.01.18
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8
import java.io.*;
import java.awt.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class CalcClientRequest extends OpenSoapRequest {
   protected StringBuffer r_anser = null;
   protected String val1 = null,
                    val2 = null,
                    operator = null,
                    responseTag = null,
                    responseNamespaceURI = null,
                    resultTag = null,
                    resultNamespaceURI = null;

   protected final static int NONE = 0,
                              RESPONSE = 1,
                              RESULT = 2,
                              ANSER = 3;
   protected int status = NONE;
   protected Label message;
   protected Label anser;

   public CalcClientRequest(Label message, Label anser) {
      super(OpenSoapConstants.CALC_URI);
      this.message = message;
      this.anser = anser;
   }

   public void startDocument() throws SAXException {
      status = NONE;
      r_anser = null;
      val1 = null;
      val2 = null;
      operator = null;
      responseTag = null;
      resultTag = null;
   }

   public void startElement(String namespaceURI,
                            String localName,
                            String rawName,
                            Attributes atts)
      throws SAXException {

      if(NONE == status) {
         status = RESPONSE;
         responseNamespaceURI = namespaceURI;
         responseTag = localName;
      } else if(RESPONSE == status && rawName.equals("Result") && null == r_anser) {
         status = ANSER;
         r_anser = new StringBuffer();
      }
   }

   public void endElement(String namespaceURI,
                          String localName,
                          String rawName)
      throws SAXException {
         status = RESPONSE;
         message.setText("");
         anser.setText(r_anser.toString());
   }

   public void characters(char[] ch,int start,int len) throws SAXException {
      if(ANSER == status) r_anser.append(ch,start,len);
   }

   public void SetVal1(String val1) {
      this.val1 = val1;
   }
   public void SetVal2(String val2) {
      this.val2 = val2;
   }
   public void SetOperator(String operator) {
      this.operator = operator;
   }

   public void writeRequest(XMLWriter writer) throws IOException {
      writer.write("<ns1:");
      writer.escape(operator);
      writer.write(" xmlns:ns1='");
      writer.write(OpenSoapConstants.CALC_URI);
      writer.write("' SOAP-ENV:encodingStyle='");
      writer.write(OpenSoapConstants.SOAPENCODING_URI);
      writer.write("'><A>");
      writer.escape(val1);
      writer.write("</A><B>");
      writer.escape(val2);
      writer.write("</B>");
      writer.write("</ns1:");
      writer.escape(operator);
      writer.write(">");
   }
}