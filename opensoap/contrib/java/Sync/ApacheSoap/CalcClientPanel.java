/*-----------------------------------------------------------------------------
 * $RCSfile: CalcClientPanel.java,v $
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
//  MODULE      :  CalcClientPanel.java
//  ABSTRACT    :  CalcClient for GUI 
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
import org.apache.soap.*; // Body, Envelope, Fault, Header 

public class CalcClientPanel extends Panel {
    protected TextComponent server;
    protected TextComponent val1;
    protected Choice operator;
    protected TextComponent val2;
    protected Label anser;
    protected Label message;

    public CalcClientPanel() throws ClassNotFoundException {
        //--- Top Display Layout
        setLayout(new BorderLayout());
        Panel topFields = new Panel();
        topFields.setLayout(new GridLayout(1,2));
        topFields.add(new Label("Server:"));        // Target Server
        server = new TextField("http://localhost:8070/calc");
        topFields.add(server);

        //--- Second Display Layout
        setLayout(new BorderLayout());
        Panel secondFields = new Panel();
        secondFields.setLayout(new GridLayout(1,5));
        val1 = new TextField("",2);                   // Variable 1=val1
        secondFields.add(val1);
        operator = new Choice();                    // Operator=operator
        secondFields.add(operator);
        operator.addItem("Add");
        operator.addItem("Subtract");
        operator.addItem("Multiply");
        operator.addItem("Divide");
        operator.addItem("Fault");

        val2 = new TextField("",2);                   // Variable 2=val2
        secondFields.add(val2);
        Button check = new Button("=");             // Invoke=check
        secondFields.add(check);
        anser = new Label(" ");                     // Anser Panel=anser
        secondFields.add(anser);

        //--- Bottom Display Layout
        Panel bottomFields = new Panel();
        bottomFields.setLayout(new GridLayout(1,1));
        message = new Label(" ");                   // message
        bottomFields.add(message);
        add(topFields,"North");
        add(secondFields,"Center");
        add(bottomFields,"South");

        check.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                checkStockLevel();
            }
        });
    }

    public void checkStockLevel() {
        message.setText("Checking...");
        try {
            if (val1.getText() == "" || val2.getText() == ""){
                message.setText("Valiable Input Error");
            } else {
                URL url = new URL(server.getText());
                CalcClientRequest request = new CalcClientRequest(message,anser);
                request.SetVal1(val1.getText());
                request.SetOperator(operator.getSelectedItem());
                request.SetVal2(val2.getText());
                request.Request(url);
            }
        }
        catch(Exception e) {
            message.setText(e.getMessage());
        }
    }
}
