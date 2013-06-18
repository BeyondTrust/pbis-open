package CyrusSasl;

import java.util.Hashtable;
import javax.security.auth.callback.*;

public class Sasl
{

    private static SaslClientFactory client_factory = null;
    private static SaslServerFactory server_factory = null;

    /*
   Creates a SaslClient using the parameters supplied. It returns null
   if no SaslClient can be created using the parameters supplied. Throws
   SaslException if it cannot create a SaslClient because of an error.

   The algorithm for selection is as follows:

   1. If a factory has been installed via setSaslClientFactory(), try it
      first. If non-null answer produced, return it.
   2. Use the packages listed in the javax.security.sasl.client.pkgs
      property from props to load in a factory and try to create a
      SaslClient, by looking for a class named ClientFactory. Repeat
      this for each package on the list until a non-null answer is
      produced. If non-null answer produced, return it.
   3. Repeat previous step using the javax.security.sasl.client.pkgs
      System property.
   4. If no non-null answer produced, return null.

   Parameters are:

      mechanisms     The non-null list of mechanism names to try. Each
                     is the IANA-registered name of a SASL mechanism.
                     (e.g. "GSSAPI", "CRAM-MD5").



      authorizationID The possibly null protocol-dependent
                     identification to be used for authorization, e.g.
                     user name or distinguished name. When the SASL
                     authentication completes successfully, the entity
                     named by authorizationId is granted access. If
                     null, access is granted to a protocol-dependent
                     default (for example, in LDAP this is the DN in
                     the bind request).

      protocol       The non-null string name of the protocol for
                     which the authentication is being performed, e.g
                     "pop", "ldap".

      serverName     The non-null fully qualified host name of the
                     server to authenticate to.

      props          The possibly null additional configuration
                     properties for the session, e.g.

    */

    public static SaslClient
	createSaslClient(String[] mechanisms,
			 String authorizationID,
			 String protocol,
			 String serverName,
			 Hashtable props,
			 javax.security.auth.callback.CallbackHandler cbh)    throws SaslException
    {
	if (client_factory == null)
	{
	    client_factory = new ClientFactory();
	}

	return client_factory.createSaslClient(mechanisms,
					       authorizationID,
					       protocol,
					       serverName,
					       props,
					       cbh);
    }

    public static void setSaslClientFactory(SaslClientFactory fac) {
	client_factory = fac;
    }

    public static void setSaslServerFactory(SaslServerFactory fac) {
	server_factory = fac;
    }


    public static SaslServer CreateSaslServer(String mechanism,
					      String protocol,
					      String serverName,
					      Hashtable props,
					      javax.security.auth.callback.CallbackHandler cbh)
					      throws SaslException
    {
	if (server_factory == null)
	{
	    server_factory = new ServerFactory();
	}

	return server_factory.createSaslServer(mechanism,
					       protocol,
					       serverName,
					       props,
					       cbh);
    }

    public static String[] getMechanismNames()
    {
	if (server_factory == null)
	{
	    server_factory = new ServerFactory();
	}

	return server_factory.getMechanismNames();	
    }





}
