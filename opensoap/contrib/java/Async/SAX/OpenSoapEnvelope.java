//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  OpenSoapEnvelope.java
//  ABSTRACT    :  OpenSoap Envelope Class
//                 [ ASYNC. Version ]
//  DATE        :  2002.02.01
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8
//--- Import Classes ---------------------------------------------------------//
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class OpenSoapEnvelope extends XMLFilterImpl {
    protected static final int NONE = 0,
                               ENVELOPE = 1,
                               HEADER = 2,
                               BODY = 3,
                               FAULT = 4,
                               FAULT_CODE = 5,
                               FAULT_STRING = 6;
    protected int status = NONE;
    protected StringBuffer buffer = null;
    protected String[] data = null;

    //--------------------------------------------------------------------------
    // Common startDocument
    //--------------------------------------------------------------------------
    public void startDocument() throws SAXException {
        status = NONE;
        getContentHandler().startDocument();       //org.xml.sax.helpers.XMLReader
    }

    //--------------------------------------------------------------------------
    // Common startElement
    //--------------------------------------------------------------------------
    public void startElement(String namespaceURI, String localName,
                             String rawName, Attributes atts)
        throws SAXException {
        if(BODY == status) {
            if(localName.equals("Fault") &&
               namespaceURI.equals(OpenSoapConstants.SOAPENV_URI)) {
                status = FAULT;
                data = new String[2];
            } else {
                getContentHandler().startElement(namespaceURI, localName,
                                                 rawName, atts);
            }
        } else if(HEADER == status) {
            getContentHandler().startElement(namespaceURI, localName,
                                             rawName, atts);
        } else if(localName.equals("Envelope") && NONE == status) {
            if(namespaceURI.equals(OpenSoapConstants.SOAPENV_URI)) {
                status = ENVELOPE;
            } else {
                throw new OpenSoapException("VersionMismatch",
                                            "Unknown SOAP version");
            }
        } else if(localName.equals("Body") &&
                  namespaceURI.equals(OpenSoapConstants.SOAPENV_URI) &&
                  ENVELOPE == status) {
            status = BODY;
        } else if(localName.equals("Header") &&
                  namespaceURI.equals(OpenSoapConstants.SOAPENV_URI) &&
                  ENVELOPE == status) {
            status = HEADER;
        } else if(localName.equals("faultcode") && status == FAULT) {
            status = FAULT_CODE;
            buffer = new StringBuffer();
        } else if(localName.equals("faultstring") && status == FAULT) {
            status = FAULT_STRING;
            buffer = new StringBuffer();
        }
    }

    //--------------------------------------------------------------------------
    // Common endElement
    //--------------------------------------------------------------------------
    public void endElement(String namespaceURI, String localName, String rawName)
        throws SAXException {
        if(BODY == status) {
            getContentHandler().endElement(namespaceURI, localName, rawName);
        } else if(HEADER == status) {
            getContentHandler().endElement(namespaceURI, localName, rawName);
        } else if(localName.equals("Envelope") &&
                  namespaceURI.equals(OpenSoapConstants.SOAPENV_URI) &&
                  ENVELOPE == status) {
            status = NONE;
        } else if(localName.equals("Body") &&
                  namespaceURI.equals(OpenSoapConstants.SOAPENV_URI) &&
                  BODY == status) {
            status = ENVELOPE;
        } else if(localName.equals("Header") &&
                  namespaceURI.equals(OpenSoapConstants.SOAPENV_URI) &&
                  HEADER == status) {
            status = ENVELOPE;
        } else if(localName.equals("Fault") &&
                  namespaceURI.equals(OpenSoapConstants.SOAPENV_URI) &&
                  status == FAULT) {
            throw new OpenSoapException(data[0],data[1]);
        } else if(localName.equals("faultcode") && status == FAULT_CODE) {
            status = FAULT;
            data[0] = buffer.toString();
            buffer = null;
        } else if(localName.equals("faultstring") && status == FAULT_STRING) {
            status = FAULT;
            data[1] = buffer.toString();
            buffer = null;
        }
    }

    //--------------------------------------------------------------------------
    // Common characters
    //--------------------------------------------------------------------------
    public void characters(char[] ch,int start,int len) throws SAXException {
        if(BODY == status) {
            getContentHandler().characters(ch,start,len);
        } else if(HEADER == status) {
            getContentHandler().characters(ch,start,len);
        } else if(FAULT_CODE == status || FAULT_STRING == status) {
            buffer.append(ch,start,len);
        }
    }

    //--------------------------------------------------------------------------
    // skippedEntity
    //--------------------------------------------------------------------------
    public void skippedEntity(String name) throws SAXException {
        if(BODY == status) {
            getContentHandler().skippedEntity(name);
        } else if(BODY == status) {
            getContentHandler().skippedEntity(name);
        }
    }

    //--------------------------------------------------------------------------
    // ignorableWhitespace
    //--------------------------------------------------------------------------
    public void ignorableWhitespace(char[] ch, int start, int len)
        throws SAXException {
        if(BODY == status) {
            getContentHandler().ignorableWhitespace(ch,start,len);
        } else if(HEADER == status) {
            getContentHandler().ignorableWhitespace(ch,start,len);
        }
    }

    //--------------------------------------------------------------------------
    // processingInstruction
    //--------------------------------------------------------------------------
    public void processingInstruction(String target,String data)
        throws SAXException {
        if(BODY == status) {
            getContentHandler().processingInstruction(target,data);
        } else if(HEADER == status) {
            getContentHandler().processingInstruction(target,data);
        }
    }
}
