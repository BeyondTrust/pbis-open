package CyrusSasl;

import javax.security.auth.callback.*;
import java.io.*;

public class GenericServer extends GenericCommon implements SaslServer 
{

    private byte[]initial_response;
    private String mechanism;
    private javax.security.auth.callback.CallbackHandler cbh;
    private boolean started = false;

    /* JNI functions */
    private native byte[] jni_sasl_server_start(int ptr,
						String mech, byte[]in, int inlen);

    private native byte[] jni_sasl_server_step(int ptr,
					       byte[] in,
					       int inlen);

    GenericServer(int cptr, String mechanism,
		  java.util.Hashtable props,
		  javax.security.auth.callback.CallbackHandler cbh)
    {
	ptr=cptr;
	this.cbh = cbh;
	this.mechanism = mechanism;
	started = false;


	/* set properties */
	super.setcommonproperties(props);	
    }


    public byte[] evaluateResponse(byte[] response) throws SaslException
    {
	byte[] out;
	byte[] in;
	int inlen;

	if (response == null)
	{
	    in=null;
	    inlen = 0;
	} else {
	    in = response;
	    inlen = response.length;
	}

	if (started == false) {
	    out=jni_sasl_server_start(ptr, mechanism,in,inlen);
	    started = true;
	} else {
	    out=jni_sasl_server_step(ptr,in,inlen);
	}

	return out;
    }
    
    public String getMechanismName()
    {
	return mechanism;
    }

    public InputStream getInputStream(InputStream source) throws IOException
    {
	if (getSecurity() > 0) {
	    return new SaslInputStream(source,this);
	} else {
	    // no security layer, no indirection needed
	    return source;
	}
    }

    public OutputStream getOutputStream(OutputStream dest) throws IOException
    {
	if (getSecurity() > 0) {
	    return new SaslOutputStream(dest,this);
	} else {
	    // no security layer, no indirection needed
	    return dest;
	}
    }
}
