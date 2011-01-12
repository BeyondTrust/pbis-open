/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSoapEnvelope.java,v $
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
//  MODULE      :  OpenSoapEnvelope.java
//  ABSTRACT    :  OpenSoap Envelope Class
//  DATE        :  2002.01.18
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
   protected static final String SOAP_URI =
      "http://schemas.xmlsoap.org/soap/envelope/";
   protected StringBuffer buffer = null;
   protected String[] data = null;

   public void startDocument() throws SAXException {
      status = NONE;
      getContentHandler().startDocument();       //org.xml.sax.helpers.XMLReader
   }

   public void startElement(String namespaceURI,
                            String localName,
                            String rawName,
                            Attributes atts)
      throws SAXException {
      if(BODY == status)
         if(localName.equals("Fault") && namespaceURI.equals(SOAP_URI)) {
            status = FAULT;
            data = new String[2];
         } else
            getContentHandler().startElement(namespaceURI,
                                             localName,
                                             rawName,
                                             atts);
      else if(localName.equals("Envelope") && NONE == status)
         if(namespaceURI.equals(SOAP_URI))
            status = ENVELOPE;
         else
            throw new OpenSoapException("VersionMismatch",
                                    "Unknown SOAP version");
      else if(localName.equals("Body") &&
              namespaceURI.equals(SOAP_URI) &&
              ENVELOPE == status)
         status = BODY;
      else if(localName.equals("Header") &&
              namespaceURI.equals(SOAP_URI) &&
              ENVELOPE == status)
         status = HEADER;
      else if(status == HEADER) {
         String mu = atts.getValue("mustUnderstand");
         if(mu != null && mu.equals("1"))
            throw new OpenSoapException("MustUnderstand",
                                    rawName + " unknown");
      } else if(localName.equals("faultcode") && status == FAULT) {
         status = FAULT_CODE;
         buffer = new StringBuffer();
      } else if(localName.equals("faultstring") && status == FAULT) {
         status = FAULT_STRING;
         buffer = new StringBuffer();
      }
   }

   public void endElement(String namespaceURI,
                          String localName,
                          String rawName)
      throws SAXException {
      if(BODY == status)
         getContentHandler().endElement(namespaceURI,
                                        localName,
                                        rawName);
      else if(localName.equals("Envelope") &&
              namespaceURI.equals(SOAP_URI) &&
              ENVELOPE == status)
         status = NONE;
      else if(localName.equals("Body") &&
              namespaceURI.equals(SOAP_URI) &&
              BODY == status)
         status = ENVELOPE;
      else if(localName.equals("Header") &&
              namespaceURI.equals(SOAP_URI) &&
              HEADER == status)
         status = ENVELOPE;
      else if(localName.equals("Fault") &&
              namespaceURI.equals(SOAP_URI) &&
              status == FAULT)
         throw new OpenSoapException(data[0],data[1]);
      else if(localName.equals("faultcode") &&
              status == FAULT_CODE)
      {
         status = FAULT;
         data[0] = buffer.toString();
         buffer = null;
      }
      else if(localName.equals("faultstring") &&
              status == FAULT_STRING)
      {
         status = FAULT;
         data[1] = buffer.toString();
         buffer = null;
      }
   }

   public void characters(char[] ch,int start,int len) throws SAXException {
      if(BODY == status)
         getContentHandler().characters(ch,start,len);
      else if(FAULT_CODE == status ||
              FAULT_STRING == status)
         buffer.append(ch,start,len);
   }

   public void skippedEntity(String name) throws SAXException {
      if(BODY == status)
         getContentHandler().skippedEntity(name);
   }

   public void ignorableWhitespace(char[] ch,
                                   int start,
                                   int len)
      throws SAXException {
      if(BODY == status)
         getContentHandler().ignorableWhitespace(ch,start,len);
   }
   
   public void processingInstruction(String target,String data)
      throws SAXException {
      if(BODY == status)
         getContentHandler().processingInstruction(target,data);
   }
}
