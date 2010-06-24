package CyrusSasl;

import java.util.Hashtable;

public interface SaslServerFactory
{

    public SaslServer createSaslServer(String mechanism,
				       String protocol,
				       String serverName,
				       Hashtable props,
				       javax.security.auth.callback.CallbackHandler cbh)
	throws SaslException;

    public String[] getMechanismNames();

}
