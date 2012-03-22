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
//  GROUP       :  Use SOAP.jar (Apache SOAP 2.2)
//  MODULE      :  CalcClientRequest.java
//  ABSTRACT    :  CalcClient for Request & Responce Message Processing
//  DATE        :  2002.01.18
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8
//--- Import Liblary ---------------------------------------------------------//
import java.io.*;
import java.net.*; 
import java.util.*; 
import java.awt.*;
import org.apache.soap.*; // Body, Envelope, Fault, Header 
import org.apache.soap.rpc.*; // Call, Parameter, Response 
import org.apache.soap.encoding.soapenc.StringDeserializer;
import org.apache.soap.util.xml.QName;
import org.apache.soap.encoding.SOAPMappingRegistry;

//--- CalcClientRequest ------------------------------------------------------//
public class CalcClientRequest {
    protected String val1 = null,
                     val2 = null,
                     operator = null,
                     server = null;
    protected Label message;
    protected Label anser;
    //--- CalcClientRequest --------------------------------------------------//
    public CalcClientRequest(Label message, Label anser) {
        this.message = message;
        this.anser = anser;
    }

    //--- Request ------------------------------------------------------------//
    public void Request(URL server) throws Exception {
        Envelope ENV = new Envelope();
        String urn = "http://services.opensoap.jp/samples/Calc/"; 

        // define deserializers for the return things (without xsi:type)
        SOAPMappingRegistry smr = new SOAPMappingRegistry ();
        StringDeserializer sd = new StringDeserializer ();
        smr.mapTypes (Constants.NS_URI_SOAP_ENC,
                  new QName ("", "Result"), null, null, sd);

        Call call = new Call(); // prepare the service invocation 
        call.setSOAPMappingRegistry (smr);
        call.setTargetObjectURI( urn ); 
        call.setMethodName( operator ); 
        call.setEncodingStyleURI( "http://schemas.xmlsoap.org/soap/encoding/" ); 
        Vector params = new Vector(); 
        params.addElement( new Parameter( "A", Integer.class, val1, null ) ); 
        params.addElement( new Parameter( "B", Integer.class, val2, null ) ); 
        call.setParams( params ); 


        try {
            System.out.println( "invoke service\n" + "  URL= " + server + "\n  URN =" + urn );
            Response response = call.invoke( server, urn ); // invoke the service 
            System.out.println( "1" );
            if( !response.generatedFault() ) { 
                Parameter result = response.getReturnValue(); // response was OK 
            System.out.println( "2" );
                message.setText("");
                anser.setText(result.getValue().toString());
            } else {
                Fault f = response.getFault(); // an error occurred 
                System.err.println( "Fault= " + f.getFaultCode() + ", " + f.getFaultString() ); 
            }
        } catch( SOAPException e ) { // call could not be sent properly
            System.err.println( "SOAPException= " + e.getFaultCode() + ", " +  e.getMessage() ); 
        } 
   } 
   public void SetVal1(String val1){
      this.val1 = val1;
   }
   public void SetVal2(String val2){
      this.val2 = val2;
   }
   public void SetOperator(String operator){
      this.operator = operator;
   }
}
