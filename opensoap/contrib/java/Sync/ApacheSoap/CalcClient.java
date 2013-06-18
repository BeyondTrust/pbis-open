/*-----------------------------------------------------------------------------
 * $RCSfile: CalcClient.java,v $
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
//  MODULE      :  CalcClient.java
//  ABSTRACT    :  CalcClient for Main
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
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.util.Vector;
import org.apache.soap.*;
import org.apache.soap.rpc.*;

//--- CalcClient -------------------------------------------------------------//
public class CalcClient {
    //--- main method --------------------------------------------------------//
    public final static void main(String args[])
        throws IOException, SOAPException, ClassNotFoundException{

        Frame frame = new Frame();
        frame.add(new CalcClientPanel());
        frame.pack();
        frame.setTitle("OpenSOAP for Java [Calc Client] - ApacheSOAP");
        frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent evt) {
               System.exit(0);
            }
        });
        frame.show();
        try {
            Thread.currentThread().join();
        }
        catch(InterruptedException e) {
        }
   }
}
