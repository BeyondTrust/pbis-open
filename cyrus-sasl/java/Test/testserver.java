
import CyrusSasl.*;
import java.net.*;
import java.io.*;
import java.util.*;

class testserver {

    static ServerSocket ssock;

    private static PrintWriter os=null;
    private static InputStreamReader ir=null;
    private static Socket s=null;
    private static BufferedReader br=null;

    private static void give_capabilities() throws IOException
    {
	String []list = Sasl.getMechanismNames();

	String cap="* CAPABILITY IMAP4 IMAP4rev1 ACL QUOTA LITERAL+ NAMESPACE UIDPLUS X-NON-HIERARCHICAL-RENAME NO_ATOMIC_RENAME";
	
	for (int lup=0;lup<list.length;lup++)
	    cap+= (" AUTH="+list[lup]);

	send(cap);

	send(". OK foo");
    }

    private static void do_auth(String mech, Socket remoteclient, int minssf, int maxssf) throws IOException
    {
	SaslClient conn;
	Hashtable props = new Hashtable();
	props.put("javax.security.sasl.encryption.minimum",String.valueOf(minssf));
	props.put("javax.security.sasl.encryption.maximum",String.valueOf(maxssf));
	props.put("javax.security.sasl.ip.local",s.getLocalAddress().getHostName());
	props.put("javax.security.sasl.ip.remote",
		  remoteclient.getInetAddress().getHostAddress());

	ServerHandler cbh = new ServerHandler();

	try {
	    
	    SaslServer saslconn = Sasl.CreateSaslServer(mech,
							"imap",
							s.getLocalAddress().getHostName(),
							props,
							cbh);

	    byte[]in = null;

	    while (true)
	    {
		byte[] out = saslconn.evaluateResponse(in);
		
		if (saslconn.isComplete()==true) {
		    break;
		} else {
		    String outline = "+ ";
		    if (out!=null) {
			System.out.println("outlen = "+ (out.length));
			outline = "+ "+SaslUtils.encode64(out);
		    }
		    send(outline);
		} 

		String line = br.readLine();

		System.out.println("in = "+line);

		if (line!=null)
		    in = SaslUtils.decode64(line);
		else
		    in = null;
	    }

	    send(". OK Authenticated");

	    System.out.println("Authenticated!!!\n");

	} catch (SaslException e) {
	    send(". NO Authentication failed");
	}
    }

    private static void pretend_to_be_imapd() throws IOException
    {
	String line;

	send("* OK pretend imapd. Use the '.' tag");

	while (true) {
	    
	    line = br.readLine();

	    if (line == null) {
		System.out.println("I think the client quit on us");
		System.exit(0);
	    }

	    if (line.startsWith(". ")==false) {
		send("* BAD Must use '.' tag");
		continue;
	    }

	    line=line.substring(2);

	    if (line.equalsIgnoreCase("CAPABILITY")==true) {
		
		give_capabilities();

		continue;
	    }

	    if (line.toUpperCase().startsWith("AUTHENTICATE ")==true) {
		line = line.substring(13);
		
		System.out.println("mechanism = "+line);

		do_auth(line,s,0,0);

		continue;
	    }

	    if (line.equalsIgnoreCase("NOOP")==true) {
		send(". OK lalala");
		continue;
	    }

	    if (line.equalsIgnoreCase("LOGOUT")==true) {

		send(". OK yeah i'll exit. seya");
		s.close();
		System.exit(0);
	    }

	    send("* BAD don't support whatever you tried");
	}


       
    }

    static void send(String str)
    {
	os.print(str+"\r\n");
	os.flush();
    }

    public static void main (String args[])
    {
	int port = 2143;
	
	try {
	
	    ssock = new ServerSocket(port);

	    System.out.println("Listening on port "+port);
	    
	    s = ssock.accept();
	    os=new PrintWriter(s.getOutputStream());
	    ir=new InputStreamReader(s.getInputStream());
	    br=new BufferedReader(ir);			


	    pretend_to_be_imapd();

	} catch (IOException e) {
	    System.out.println("IO exception");
	}
    }



}
