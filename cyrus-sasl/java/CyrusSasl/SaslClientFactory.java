package CyrusSasl;

import java.util.Hashtable;

public interface SaslClientFactory
{

    public SaslClient createSaslClient(String[] mechanisms,
				       String authorizationId,
				       String protocol,
				       String serverName,
				       Hashtable props,
				       javax.security.auth.callback.CallbackHandler cbh)
	throws SaslException;

}
