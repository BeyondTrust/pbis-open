//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  CalcClientResult.java
//  ABSTRACT    :  CalcClient for Body SOAP Message Proc.(Result)
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

public class CalcClientResult extends OpenSoapRequest {
    protected StringBuffer r_anser = null,
                           r_message_id = null;
    protected String pno = null,
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
                               ANSER = 7,
                               RESPONSE = 8;
    protected int status = NONE;
    protected Label message;
    protected Label anser;

    public CalcClientResult(Label message, Label anser) {
        super(OpenSoapConstants.CALC_URI);
        this.message = message;
        this.anser = anser;
    }

    public void startDocument() throws SAXException {
        status = NONE;
        pno = null;
        responseTag = null;
        resultTag = null;
    }

    public void startElement(String namespaceURI, String localName, String rawName, Attributes atts)
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

    public void endElement(String namespaceURI, String localName, String rawName)
        throws SAXException {
        if (rawName.equals("Result")){
            status = ANSER;
            message.setText("");
            anser.setText(r_anser.toString());
        }
    }

    public void characters(char[] ch,int start,int len) throws SAXException {
        if(ANSER == status) {
            r_anser.append(ch,start,len);
        }
    }

    public void SetPno(String pno) {
        this.pno = pno;
    }

    public void writeRequestHead(XMLWriter writer) throws IOException {
        writer.write("<SOAP-ENV:Header>\n");
        writer.write("  <opensoap-header:opensoap-header-block xmlns:opensoap-header='" +
                      OpenSoapConstants.SOAP_HEADER_URI +
                     "'>\n");
        writer.write("    <opensoap-header:message_id>");
        writer.escape(pno);
        writer.write("</opensoap-header:message_id>\n");
        writer.write("  </opensoap-header:opensoap-header-block>\n");
        writer.write("</SOAP-ENV:Header>\n");
    }

    public void writeRequestBody(XMLWriter writer) throws IOException {
        writer.write("<SOAP-ENV:Body/>\n");
    }
}
