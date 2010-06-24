package CyrusSasl;

import java.util.Hashtable;
import javax.security.auth.callback.*;

class ServerFactory implements SaslServerFactory
{
    private int localptr = 0;

    /* JNI functions  */
    private native int jni_sasl_server_init(String appname);
    private native int jni_sasl_server_new(String service,
					   String local_domain,
					   int secflags);


    public ServerFactory()
    {
	/* these parameters aren't needed for getting mech list */
	localptr = jni_sasl_server_new("foo", "bar", 0);
    }

    private boolean init(String appname)
    {
	/* load library */
	try {
	    System.loadLibrary("javasasl");
	} catch (UnsatisfiedLinkError e) {
	    /* xxx */
	    System.out.println("Unable to load javasasl library");
	}

	jni_sasl_server_init(appname);

	return true;
    }

    {
	init("javasasl application");
    }

    public SaslServer createSaslServer(String mechanism,
				       String protocol,
				       String serverName,
				       Hashtable props,
				       javax.security.auth.callback.CallbackHandler cbh)
	throws SaslException
    {
	int cptr;

	cptr = jni_sasl_server_new(protocol,
				   serverName,
				   0);

	if (cptr == 0) {
	    throw new SaslException("Unable to create new Client connection object", 
				    new Throwable());
	}

	return new GenericServer(cptr,mechanism,props,cbh);
    }

    private native String jni_sasl_server_getlist(int ptr, String prefix,
						  String sep, String suffix);

    public String[] getMechanismNames()
    {
	if (localptr == 0)
	    localptr = jni_sasl_server_new("foo", 
					   "bar",
					   0);

	String list = jni_sasl_server_getlist(localptr, "",
					      "\n","\n");

	/* count newlines */
	int newlines = 0;
	int pos =0;

	while (pos < list.length()) {
	    if (list.charAt(pos)=='\n') 
		newlines++;
	    pos++;
	}

	String[]ret = new String[newlines];

	int num =0;
	pos =0;
	String temp="";

	while (pos < list.length()) {	    
	    if (list.charAt(pos)=='\n') {
		ret[num++]=temp;
		temp=new String("");
	    } else {
		temp+=list.charAt(pos);
	    }
	    pos++;
	}
	
	return ret;
    }

}
