//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX Server Side Service
//  MODULE      :  CalcServiceResponse.java
//  ABSTRACT    :  Calc Service
//  DATE        :  2002.02.20
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8
import java.io.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class CalcServiceResponse extends DefaultHandler {
    protected StringBuffer     val1 = null,
                               val2 = null,
                               operator = null;
    protected final static int NONE = 0,
                               GET_CALC = 1,
                               VAL1 = 2,
                               VAL2 = 3,
                               OPERATOR = 4;
    protected int              status = NONE;

    //--------------------------------------------------------------------------
    // SAX Event : START DOCUMENT
    //--------------------------------------------------------------------------
    public void startDocument() throws SAXException {
        status = NONE;
        val1 = null;
        val2 = null;
        operator = null;
    }

    //--------------------------------------------------------------------------
    // SAX Event : END DOCUMENT
    //--------------------------------------------------------------------------
    public void endDocument() throws SAXException {
    }

    //--------------------------------------------------------------------------
    // SAX Event : START ELEMENT
    //--------------------------------------------------------------------------
    public void startElement(String namespaceURI,
                             String localName,
                             String rawName,
                             Attributes atts) throws SAXException {
        if((localName.equals("Add") ||
            localName.equals("Subtract") ||
            localName.equals("Multiply") ||
            localName.equals("Divide") ) &&
            namespaceURI.equals(OpenSoapConstants.CALC_URI) &&
            NONE == status) {
            status = OPERATOR;
            operator = new StringBuffer(localName.toString());
        } else if(rawName.equals("A") && null == val1) {
            val1 = new StringBuffer();
            status = VAL1;
        } else if(rawName.equals("B") && null == val2) {
            val2 = new StringBuffer();
            status = VAL2;
        } else {
            status = NONE;
        }
    }

    //--------------------------------------------------------------------------
    // SAX Event : END ELEMENT
    //--------------------------------------------------------------------------
    public void endElement(String namespaceURI,
                           String localName,
                           String rawName) throws SAXException {
        if((localName.equals("Add") ||
            localName.equals("Subtract") ||
            localName.equals("Multiply") ||
            localName.equals("Divide") ) &&
            namespaceURI.equals(OpenSoapConstants.CALC_URI) &&
            GET_CALC == status) {
            status = NONE;
        } else if(rawName.equals("A") && VAL1 == status) {
            status = GET_CALC;
        } else if(rawName.equals("B") && VAL2 == status) {
            status = GET_CALC;
        } else if(rawName.equals("operator") && OPERATOR == status) {
            status = GET_CALC;
        }
    }

    //--------------------------------------------------------------------------
    // SAX Event : CHARACTERS
    //--------------------------------------------------------------------------
    public void characters(char[] ch,int start,int len) throws SAXException {
        if(VAL1 == status){
            val1.append(ch,start,len);
        } else if(VAL2 == status){
            val2.append(ch,start,len);
        } else if(OPERATOR == status){
            operator.append(ch,start,len);
        }
    }

    //--------------------------------------------------------------------------
    // CalcService RESULT Response
    //--------------------------------------------------------------------------
    public void writeResponse( CharArrayWriter payload) throws IOException, OpenSoapException {
        if(val1.toString() == null){
            throw new OpenSoapException("Client", "Missing val1");
        }
        if(val2 == null){
            throw new OpenSoapException("Client", "Missing val2");
        }
        if(operator == null){
            throw new OpenSoapException("Client", "Missing operator");
        }

        double a = Double.parseDouble(val1.toString());
        double b = Double.parseDouble(val2.toString());
		if (true) {
			String op = operator.toString().trim();
			double c = 0;
			if (op.equals("Add")){
				c = a + b;
			} else if (op.equals("Subtract")){
				c = a - b;
			} else if (op.equals("Multiply")){
				c = a * b;
			} else if (op.equals("Divide")){
				c = a / b;
			}
			String responseMessage
				=	"<?xml version='1.0' encoding='UTF-8'?>"
				+	"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\'"
				+	OpenSoapConstants.SOAPENV_URI + "\' "
				+	"xmlns:xsi=\'" + OpenSoapConstants.XSI1999SCHEMA + "\' "
				+	"xmlns:xsd=\'" + OpenSoapConstants.XSD1999SCHEMA + "\'>"
				+	"<SOAP-ENV:Body>"
				+	"<m:" + op + "Response "
				+	"xmlns:m=\'" + OpenSoapConstants.CALC_URI + "\' "
				+	"SOAP-ENV:encodingStyle=\'"
				+	OpenSoapConstants.SOAPENCODING_URI  + "\'>"
				+	"<Result xsi:type=\'xsd:double\'>"
				+ 	Double.toString(c)
				+	"</Result>"
				+	"</m:" + op + "Response>"
				+	"</SOAP-ENV:Body>"
				+	"</SOAP-ENV:Envelope>";
			payload.write(responseMessage);
		}
		else {
			String op = operator.toString();
			double c = 0;

			payload.write("<?xml version='1.0' encoding='UTF-8'?>");
			payload.write("<SOAP-ENV:Envelope xmlns:SOAP-ENV='");
			payload.write(OpenSoapConstants.SOAPENV_URI);
			payload.write("' xmlns:xsi='");
			payload.write(OpenSoapConstants.XSI1999SCHEMA);
			payload.write("' xmlns:xsd='");
			payload.write(OpenSoapConstants.XSD1999SCHEMA);
			payload.write("'>");

			payload.write("<SOAP-ENV:Body>");
			payload.write("<m:");
			payload.write( op);
			payload.write("Response xmlns:m='");
			payload.write(OpenSoapConstants.CALC_URI);
			payload.write("' SOAP-ENV:encodingStyle='");
			payload.write(OpenSoapConstants.SOAPENCODING_URI);
			payload.write("'><Result xsi:type='xsd:double'>");
			if (op.equals("Add")){
				c = a + b;
			} else if (op.equals("Subtract")){
				c = a - b;
			} else if (op.equals("Multiply")){
				c = a * b;
			} else if (op.equals("Divide")){
				c = a / b;
			}
			payload.write(Double.toString(c));
			payload.write("</Result>");
			payload.write("</m:");
			payload.write( op);
			payload.write("Response>");
			payload.write("</SOAP-ENV:Body></SOAP-ENV:Envelope>");
		}
   }
}

