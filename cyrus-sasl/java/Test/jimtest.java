
import java.net.*;
import java.io.*;
import java.util.Hashtable;
import CyrusSasl.*;

class jimtest
{
    private static PrintWriter os=null;
    private static InputStreamReader ir=null;
    private static Socket s=null;
    private static BufferedReader br=null;

  static void send(String str)
  {
      os.print(str+"\r\n");
      os.flush();
    
  }

    static boolean connect(String servername, int port)
    {
	try
	    {
		s=new Socket(servername,port);
	    } catch (UnknownHostException e){
		System.out.println("invalid host");
		return false;
	    } catch (IOException e) {
		System.out.println("invalid host");
		return false;
	    }

	try {
	    os=new PrintWriter(s.getOutputStream());
	    ir=new InputStreamReader(s.getInputStream());
	    br=new BufferedReader(ir);			

	} catch (IOException e) {
	    System.out.println("IO no work");	
	    return false;      
	}

		
	System.out.println("connected...");

	return true;						
    }

    static String[] parsecapabilities(String line)
    {
	String[] ret = new String[100];
	int size = 0;
	String tmp;
	int pos = 0;

	while (pos < line.length() )
	{
	    char c;
	    tmp = "";

	    do {
		c = line.charAt(pos);
		tmp+=c;
		pos++;
	    } while ((c!=' ') && (pos < line.length()));
	    
	    if (tmp.startsWith("AUTH=")==true)
	    {
		ret[size++] = tmp.substring(5);
	    }
	}
	
	return ret;
    }

    static String[] askcapabilities()
    {
	String line;
	String mechs[];
	
	try {
	    
	    send(". CAPABILITY");
	    
	    do {
		line = br.readLine();
	    } while (line.startsWith("* CAPABILITY")==false);
	    
	    mechs = parsecapabilities(line);

	    do {
		line = br.readLine();
	    } while (line.startsWith(".")==false);

	} catch (IOException e) {
	    System.out.println("IO no work");	
	    return null;      
	}
	
	return mechs;
    }

    static SaslClient start_sasl(String[] mechs, String remoteserver, String localaddr, int minssf, int maxssf)
    {
	SaslClient conn;
	Hashtable props = new Hashtable();
	props.put("javax.security.sasl.encryption.minimum",String.valueOf(minssf));
	props.put("javax.security.sasl.encryption.maximum",String.valueOf(maxssf));
	props.put("javax.security.sasl.ip.local",localaddr);
	props.put("javax.security.sasl.ip.remote",remoteserver);

	Handler cbh = new Handler();

	try {
	    conn = Sasl.createSaslClient(mechs,
					 "tmartin",
					 "imap",
					 remoteserver,
					 props,
					 cbh);

	    if (conn == null) {
		System.out.println("conn is null");
	    }
	    
	    if (conn.hasInitialResponse()) {
		/* xxx */
	    }

	    send(". AUTHENTICATE "+conn.getMechanismName());

	    do {

		String line = br.readLine();

		if (line.startsWith("+ ")==true) {

		    line = line.substring(2);

		    byte[] in = SaslUtils.decode64(line);

		    byte[] out = conn.evaluateChallenge(in);

		    String outline = SaslUtils.encode64(out);

		    send(outline);

		} else if (line.startsWith(". OK")==true) {
		    System.out.println("S: " + line);

		    if (conn.isComplete()==false) {
			System.out.println("Something funny going on here...");
			System.exit(1);
		    }
		    return conn;
		} else {
		    System.out.println("S: "+ line);
		    /* authentication failed */
		    return null;
		} 

	    } while (true);
	    
	} catch (SaslException e) {
	    System.out.println("SASL exception\n");
	} catch (IOException e) {
	    System.out.println("IO exception\n");
	}

	return null;
    }

    static void be_interactive(SaslClient conn)
    {
	try {
	    InputStream saslin = conn.getInputStream(s.getInputStream());
	    OutputStream saslout = conn.getOutputStream(s.getOutputStream());
	    int len;
	    byte[] arr;

	    while (true)
	     {
		 if ((len = System.in.available())>0) {

		     /* read from keyboard */
		     arr = new byte[len+1];
		     System.in.read(arr,0,len);

		     if (arr[len-1]=='\n') {
			 arr[len-1]= (byte) '\r';
			 arr[len]= (byte) '\n';
		     }
		     
		     /* write out to stream */		     
		     saslout.write(arr);
		     saslout.flush();

		 } else if ((len = saslin.available())>0) {

		     /* read from socket */
		     arr = new byte[len];
		     saslin.read(arr);

		     System.out.print(new String(arr));

		 } else {
		     /* sleep */
		 }
	     }

	} catch (SaslException e) {

	} catch (IOException e) {

	}

	
    }

    static void usage()
    {
	System.out.println("Usage:");
	System.out.println("jimtest [-k minssf] [-l maxssf] [-m mech] [-p port] server");
	System.exit(1);
    }

    public static void main (String args[])
    {
	String[] mechs;
	SaslClient conn;

	String arg;
	int i = 0;
	int minssf = 0;
	int maxssf = 9999;
	String onemech = null;
	int port = 143;

        while ((i < (args.length-1) ) && (args[i].startsWith("-"))) {
	    arg = args[i++];
	    	    
	    // use this type of check for arguments that require arguments
	    if (arg.equals("-k")) {
		if (i < args.length)
		    minssf = Integer.parseInt(args[i++]);
		else {
		    System.err.println("-k requires a number");
		    usage();
		}
	    } else if (arg.equals("-l")) {
		if (i < args.length)
		    maxssf = Integer.parseInt(args[i++]);
		else {
		    System.err.println("-l requires a number");
		    usage();
		}
	    } else if (arg.equals("-m")) {
		if (i < args.length)
		    onemech = args[i++];
		else {
		    System.err.println("-m requires parameter");
		    usage();
		}
	    } else if (arg.equals("-p")) {
		if (i < args.length)
		    port = Integer.parseInt(args[i++]);
		else {
		    System.err.println("-p requires a number");
		    usage();
		}
	    } else {
		usage();
	    }
	}

	if (i != args.length-1) usage();

	String servername = args[i];

	if (connect(servername,port)==false) {
	    System.out.println("Unable to connect to host: "+servername);
	    System.exit(1);
	}

	mechs = askcapabilities();

	if (onemech!=null) {
	    mechs = new String[1];
	    mechs[0]=onemech;
	}

	conn = start_sasl(mechs,servername, s.getLocalAddress().getHostName(), minssf,maxssf);

	if (conn == null) {
	    System.out.println("Authentication failed");
	    System.exit(1);
	}

	be_interactive(conn);
    }


}
