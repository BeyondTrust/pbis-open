package CyrusSasl;

import java.util.Hashtable;
import java.net.*;

/**
 * @version 1.0
 * @author Tim Martin
 */

public abstract class GenericCommon
{

  /* These are the jni functions called by the routines in common
   * see javasasl.c for their implementations
   */

    private native void jni_sasl_set_prop_string(int ptr, int propnum, String value);
    private native void jni_sasl_set_prop_int(int ptr, int propnum, int value);
    private native void jni_sasl_set_prop_bytes(int ptr, int propnum, byte[] value);
    private native void jni_sasl_set_server(int ptr, byte []ipnum, int port);
    private native void jni_sasl_set_client(int ptr, byte []ipnum, int port);
    private native void jni_sasl_setSecurity(int ptr, int minssf, int maxssf);
    private native int jni_sasl_getSecurity(int ptr);
    private native byte[] jni_sasl_encode(int ptr, byte[] in,int len);
    private native byte[] jni_sasl_decode(int ptr, byte[] in,int len);
    private native void jni_sasl_dispose(int ptr);

  /**
   * security layer security strength factor
   */
  public static int SASL_SSF      =1;    

  public static int SASL_MAXOUTBUF=2;     /* security layer max output buf unsigned */  
  public static int SASL_REALM    =3;     /* server authentication realm used */
  public static int SASL_GETOPTCTX=4;     /* context for getopt callback */


    /**
     * Local sockaddr_in (use setServer and setClient to set this)
     */
  public static int SASL_IP_LOCAL =5;   

    /**
     * Remote sockaddr_in (use setClient and setServer to set this)
     */

  public static int SASL_IP_REMOTE =6;  

    /**
     * External security factor (use setSecurity to set this)
     */
  public static int SASL_SSF_EXTERNAL=100;  
  public static int SASL_SEC_PROPS   =101;  /* sasl_security_properties_t */


    int ptr;			// this is the actual pointer to sasl_conn_t
    int ssfactive;		// active ssf on this connection

  boolean finished;

  public boolean done() { return finished; }

  /**
   * Set a SASL property that takes a string value
   *
   * @param PROPERTY one of the property constants
   * @param value string value
   */

  public void setproperty(int PROPERTY, String value)
  {
    jni_sasl_set_prop_string(ptr,PROPERTY,value);
  }

  /**
   * Set a SASL property that takes a integer value
   *
   * @param PROPERTY one of the property constants
   * @param value integer value
   */

  public void setproperty(int PROPERTY, int value)
  {
    jni_sasl_set_prop_int(ptr,PROPERTY,value);
  }

  /**
   * Set a SASL property that takes a byte[] value
   *
   * @param PROPERTY one of the property constants
   * @param value byte[] value
   */

  public void setproperty(int PROPERTY, byte[] value)
  {
    jni_sasl_set_prop_bytes(ptr,PROPERTY,value);
  }

  /**
   * Set the SASL properties for the server
   * This sets the IP address and port
   *
   * @param name String of name of server (e.g. cyrus.andrew.cmu.edu)
   * @param port port connected to on that server
   */

  private boolean setRemoteIP(String name,int port)
  {
    byte[]ip=null;
    try {
      InetAddress server=InetAddress.getByName(name);
      ip=server.getAddress();    
    } catch (UnknownHostException e) { 
      return false;
    }

    jni_sasl_set_server(ptr, ip, port);
    return true;
  }

  /**
   * Set the SASL properties for the client
   * This sets the IP address and port
   *
   * @param name String of local cannonical name (e.g. myhostname.andrew.cmu.edu)
   * @param port port connecting
   */

  private boolean setLocalIP(String name, int port)
  {
    byte[]ip=null;
    try {
      InetAddress server=InetAddress.getByName(name);
      ip=server.getAddress();    
    } catch (UnknownHostException e) { 
      return false;
    }

    jni_sasl_set_client(ptr, ip, port);
    return true;
  }

  /**
   * Set the SASL properties for the client
   * This sets the IP address and port
   *
   * @param local local InetAdress
   * @param port port connecting
   */

  public boolean setClient(InetAddress local,int port)
  {
    byte[]ip=local.getAddress();

    jni_sasl_set_client(ptr, ip, port);

    return true;
  }

  /**
   * Set the SASL properties for the client
   * This sets the IP address and port
   * The local IP address is determined with InetAddress.getLocalHost()
   *
   * @param port port connecting
   */

  public boolean setClient(int port)
  {
    try {
      return setClient(InetAddress.getLocalHost(),port);
    } catch (UnknownHostException e) {
      return false;
    }
  }

  /**
   * Sets the security properties for the session
   *
   * @param external external security strength
   * @param minssf minimum security needed
   * @param maxssf maximum security to negotiate
   *
   * @return if the propery was set sucessfully or not
   */


    public boolean setSecurity(int external, int minssf, int maxssf)
    {
	/* setproperty(SASL_SSF_EXTERNAL, external); */
	
	jni_sasl_setSecurity(ptr,minssf,maxssf);
	
	return true;
    }

    public int getSecurity() {
	return jni_sasl_getSecurity(ptr);
    }

  /**
   * Encode a String with the negotiated layer
   *
   * @param in String to be encoded
   * @return the encoded string represented at a byte[] 
   */
  public byte[] encode(byte[] in)
  {
    
    byte[] out=jni_sasl_encode(ptr,in,in.length);

    return out;
  }
					    
  /**
   * Decode a byte[] with the negotiated layer
   *
   * @param in byte[] to be decoded
   * @param len number of bytes to be decoded
   * @return the decoded string represented at a byte[]
   */
  public byte[] decode(byte[] in, int len)
  {
    
    byte[] out=jni_sasl_decode(ptr,in,len);

    return out;
  }

  /**
   * Decode a String with the negotiated layer. NOTE: Be careful with
   * this function. International or high ascii characters may do strange
   * things. The byte[] method is preferred
   *
   * @param in String to be decoded
   * @return the decoded string represented at a byte[]
   */
  public byte[] decode(String in)
  {
    return decode(in.getBytes(),in.length());
  }

  protected void setcommonproperties(Hashtable props)
  {
      int i_ssfmin = 0;
      String s_ssfmin=(String) props.get("javax.security.sasl.encryption.minimum");
      if (s_ssfmin!=null) i_ssfmin = Integer.parseInt(s_ssfmin);

      int i_ssfmax = 256;
      String s_ssfmax=(String) props.get("javax.security.sasl.encryption.maximum");
      if (s_ssfmax!=null) i_ssfmax = Integer.parseInt(s_ssfmax);

      
      int i_external = 0;
      /*      String external=(String) props.getProperty("security.policy.encryption.external",
       */

      setSecurity(i_external,
		  i_ssfmin,
		  i_ssfmax);
      
      String iplocal  = (String) props.get("javax.security.sasl.ip.local");     
      if (iplocal!=null) setLocalIP(iplocal,0);

      String ipremote = (String) props.get("javax.security.sasl.ip.remote");
      if (ipremote!=null) setRemoteIP(ipremote,0);
   
      /*    String maxbuf=props.getProperty("security.maxbuf","65000"); */
    /* xxx this raises an exception for some reason
       setproperty(SASL_MAXOUTBUF,Integer.parseInt(maxbuf)); */
  }


    final protected void finalize () throws Throwable 
    {
	jni_sasl_dispose(ptr);
    }

    protected boolean complete = false;

    public boolean isComplete()
    {
	return complete;
    }

    /* called by JNI layer */
    public void setcomplete(int a)
    {
	complete = true;
    }


}
