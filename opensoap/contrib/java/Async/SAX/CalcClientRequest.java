//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  CalcClientRequest.java
//  ABSTRACT    :  CalcClient for Body SOAP Message Proc.
//                 [ ASYNC. Version ]
//  DATE        :  2002.02.01
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
    protected StringBuffer r_anser = null,
                           r_message_id = null;
    protected String val1 = null,
                     val2 = null,
                     operator = null,
                     responseTag = null,
                     responseNamespaceURI = null,
                     resultTag = null,
                     resultNamespaceURI = null;
    protected static final int NONE = 0,
                               ENVELOPE = 1,
                               HEADER = 2,
                               BODY = 3,
                               FAULT = 4,
                               FAULT_CODE = 5,
                               FAULT_STRING = 6,
                               MESSAGEID = 7,
                               ANSER = 8,
                               RESPONSE = 9;
    protected int status = NONE;
    protected Label message;
    protected Label pno;

    public CalcClientRequest(Label message, Label pno) {
        super(OpenSoapConstants.CALC_URI);
        this.message = message;
        this.pno = pno;
    }

    public void startDocument() throws SAXException {
        status = NONE;
        r_message_id = null;
        val1 = null;
        val2 = null;
        operator = null;
        responseTag = null;
        resultTag = null;
    }

    public void startElement(String namespaceURI, String localName, String rawName, Attributes atts)
        throws SAXException {
        if(NONE == status) {
            status = RESPONSE;
            responseNamespaceURI = namespaceURI;
            responseTag = localName;
        } else if(RESPONSE == status && localName.equals("message_id") && null == r_message_id) {
            status = HEADER;
            r_message_id = new StringBuffer();
        }
    }

    public void endElement(String namespaceURI, String localName, String rawName)
        throws SAXException {
        if ( localName.equals("message_id")) {
            status = HEADER;
            message.setText("");
            pno.setText(r_message_id.toString());
        }
    }

    public void characters(char[] ch,int start,int len) throws SAXException {
        if(HEADER == status) r_message_id.append(ch,start,len);
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

    public void writeRequestHead(XMLWriter writer) throws IOException {
        writer.write("<SOAP-ENV:Header>\n");
        writer.write("  <opensoap-header:opensoap-header-block xmlns:opensoap-header='" +
                     OpenSoapConstants.SOAP_HEADER_URI +
                     "'>\n");
        writer.write("    <opensoap-header:ttl opensoap-header:type='second'>80</opensoap-header:ttl>\n");
        writer.write("    <opensoap-header:async>true</opensoap-header:async>\n");
        writer.write("  </opensoap-header:opensoap-header-block>\n");
        writer.write("</SOAP-ENV:Header>\n");
    }
    public void writeRequestBody(XMLWriter writer) throws IOException {
        writer.write("<SOAP-ENV:Body>\n");
        writer.write("<ns1:");
        writer.escape(operator);
        writer.write(" xmlns:ns1='");
        writer.write(OpenSoapConstants.CALC_URI);
        writer.write("' SOAP-ENV:encodingStyle='");
        writer.write(OpenSoapConstants.SOAPENCODING_URI);
        writer.write("'>\n  <A>");
        writer.escape(val1);
        writer.write("</A>\n  <B>");
        writer.escape(val2);
        writer.write("</B>\n</ns1:");
        writer.escape(operator);
        writer.write(">\n");
        writer.write("</SOAP-ENV:Body>\n");
    }
}