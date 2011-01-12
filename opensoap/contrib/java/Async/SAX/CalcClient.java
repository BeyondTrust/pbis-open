//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  CalcClient.java
//  ABSTRACT    :  CalcClient For Sax Version Main
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
import java.awt.*;
import java.util.*;
import org.xml.sax.*;
import java.awt.event.*;
import org.xml.sax.helpers.*;

public class CalcClient {
    public final static void main(String args[])
        throws IOException, OpenSoapException, SAXException, ClassNotFoundException {
        Frame frame = new Frame();                                  // Making Form
        frame.add(new CalcClientPanel());                           // Control Add
        frame.pack();
        frame.setTitle("OpenSOAP for Java [Calc Client] ASYNC.");
        frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent evt) {
               System.exit(0);
            }
        });
        frame.show();
        try {
            Thread.currentThread().join();
        } catch(InterruptedException e) {
        }
   }
}
