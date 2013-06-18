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
//  GROUP       :  Use SAX
//  MODULE      :  CalcClient.java
//  ABSTRACT    :  CalcClient For Sax Version Main
//  DATE        :  2002.01.18
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

public class CalcClient
{
    public final static void main(String args[])
        throws IOException, OpenSoapException, SAXException,
               ClassNotFoundException
    {
        Frame frame = new Frame();
        frame.add(new CalcClientPanel());
        frame.pack();
        frame.setTitle("OpenSOAP for Java [Calc Client]");
        frame.addWindowListener(new WindowAdapter()
        {
            public void windowClosing(WindowEvent evt)
            {
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
