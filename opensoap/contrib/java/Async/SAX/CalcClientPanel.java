//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  CalcClientPanel.java
//  ABSTRACT    :  CalcClient For GUI Class
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
import java.awt.event.*;
import org.xml.sax.*;

public class CalcClientPanel extends Panel {
    protected TextComponent server;
    protected TextComponent val1;
    protected Choice operator;
    protected TextComponent val2;
    protected Label anser;
    protected Label pno;
    protected Label message;

    public CalcClientPanel() throws ClassNotFoundException {
        setLayout(new BorderLayout());
        //--- Sending Message Information Display Layout
        Panel Pan_100 = new Panel();
        Pan_100.setLayout(new BorderLayout());
        Panel Pan_101 = new Panel();
        Pan_101.setLayout(new GridLayout(1,2));
        Pan_101.add(new Label("Server:"));                         // Target Server
        server = new TextField("http://localhost:8070/calc");
        Pan_101.add(server);

        Panel Pan_102 = new Panel();
        Pan_102.setLayout(new GridLayout(1,4));
        val1 = new TextField("",2);                                // Variable 1 Area
        Pan_102.add(val1);
        operator = new Choice();                                   // Operator Area
        Pan_102.add(operator);
        operator.addItem("Add");
        operator.addItem("Subtract");
        operator.addItem("Multiply");
        operator.addItem("Divide");
        operator.addItem("Fault");
        val2 = new TextField("",2);                                // Variable 2 Area
        Pan_102.add(val2);
        Button check = new Button("Process Registration");         // Process Registration Button
        Pan_102.add(check);
        Pan_100.add(Pan_101,"North");
        Pan_100.add(Pan_102,"South");

        //--- Reasponse Message Information Display Layout
        Panel Pan_200 = new Panel();
        Pan_200.setLayout(new BorderLayout());
        Panel Pan_201 = new Panel();
        Pan_201.setLayout(new GridLayout(2,2));
        Pan_201.add(new Label("ProcessNo.:"));
        pno = new Label("");
        Pan_201.add(pno);                                          // Process ID Area
        Pan_201.add(new Label(""));
        Button result = new Button("Get Process RESULT");          // Get Process RESULT Button
        Pan_201.add(result);

        Panel Pan_202 = new Panel();
        Pan_202.setLayout(new GridLayout(1,3));
        Pan_202.add(new Label("Anser:"));                          // Result Anser Area
        anser = new Label(" ");
        Pan_202.add(anser);
        Pan_200.add(Pan_201,"North");
        Pan_200.add(Pan_202,"South");

        //--- Process Information Display Layout
        Panel Pan_300 = new Panel();
        Pan_300.setLayout(new GridLayout(1,1));
        message = new Label(" ");                                  // Process Message
        Pan_300.add(message);
        add(Pan_100,"North");
        add(Pan_200,"Center");
        add(Pan_300,"South");

        check.addActionListener(new ActionListener() {             // Process Registration Button
            public void actionPerformed(ActionEvent evt) {         // Click EventListener
                ASyncRequest();
            }
        });
        result.addActionListener(new ActionListener() {            // Get Process RESULT Button
            public void actionPerformed(ActionEvent evt) {         // Click EventListener
                GetResult();
            }
        });
   }

    //--------------------------------------------------------------------------
    // Process Registration Button Click EventListener
    //--------------------------------------------------------------------------
    public void ASyncRequest() {
        message.setText("Checking...");
        pno.setText("");
        anser.setText("");
        try {
            if (val1.getText() == "" || val2.getText() == ""){
                message.setText("Valiable Input Error");
            } else {
                URL url = new URL(server.getText());
                CalcClientRequest request = new CalcClientRequest(message,pno);
                request.SetVal1(val1.getText());
                request.SetOperator(operator.getSelectedItem());
                request.SetVal2(val2.getText());
                request.invoke(url);
            }
        } catch(IOException e) {
            message.setText(e.getMessage());
        } catch(OpenSoapException e) {
            message.setText(e.getCode() + ' ' + e.getMessage());
        } catch(SAXException e) {
            Exception ex = null == e.getException() ? e : e.getException();
            message.setText(e.getMessage());
        }
    }

    //--------------------------------------------------------------------------
    // Get Process RESULT Button Click EventListener
    //--------------------------------------------------------------------------
    public void GetResult() {
        message.setText("Checking...");
        anser.setText("");
        try {
            if (pno.getText() == ""){
                message.setText("Valiable Input Error");
            } else {
                URL url = new URL(server.getText());
                CalcClientResult result = new CalcClientResult(message,anser);
                result.SetPno(pno.getText());
                result.invoke(url);
            }
        } catch(IOException e) {
            message.setText(e.getMessage());
        } catch(OpenSoapException e) {
            message.setText(e.getCode() + ' ' + e.getMessage());
        } catch(SAXException e) {
            Exception ex = null == e.getException() ? e : e.getException();
            message.setText(e.getMessage());
        }
    }
}
