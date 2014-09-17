//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  OpenSoapRequest.java
//  ABSTRACT    :  Request & Response For OpenSoap Message
//                 [ ASYNC. Version ]
//  DATE        :  2002.02.01
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

public abstract class OpenSoapRequest extends DefaultHandler {
    protected String soapAction;

    //--------------------------------------------------------------------------
    // Common OpenSoapRequest
    //  Input SOAP Action String URI
    //--------------------------------------------------------------------------
    public OpenSoapRequest(String soapAction) {
        if(null == soapAction) {
            soapAction = "\"\"";
        }
        this.soapAction = '"' + soapAction + '"';
    }

    //--------------------------------------------------------------------------
    // Common Invoke
    //  Input SOAP Service Server URL
    //--------------------------------------------------------------------------
    public void invoke(URL server)
        throws IOException, OpenSoapException, SAXException {
        HttpURLConnection conn =
            (HttpURLConnection)server.openConnection();
        conn.setDoOutput(true);
        conn.setDoInput(true);

        // Make Envelope
        CharArrayWriter payload = new CharArrayWriter();
        payload.write("<?xml version='1.0'?>\n");
        payload.write("<SOAP-ENV:Envelope xmlns:SOAP-ENV='" + 
                       OpenSoapConstants.SOAPENV_URI +
                      "'>\n");
        writeRequestHead(new XMLWriter(payload));           // Burunch Header Part
        writeRequestBody(new XMLWriter(payload));           // Burunch Body Part
        payload.write("</SOAP-ENV:Envelope>\n");

        // Http Header
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

    //--------------------------------------------------------------------------
    // Constracture
    //--------------------------------------------------------------------------
    public abstract void writeRequestHead(XMLWriter writer)
        throws IOException;
    public abstract void writeRequestBody(XMLWriter writer)
        throws IOException;
}