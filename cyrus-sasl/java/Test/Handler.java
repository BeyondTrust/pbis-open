
import java.io.*;
import javax.security.auth.callback.*;

class Handler implements javax.security.auth.callback.CallbackHandler{

    String authid;
    String userid;
    String password;
    String realm;
    
    public Handler()
    {

    }

    public Handler(String authid, String userid, String password, String realm)
    {
	this.authid = authid;
	this.userid = userid;
	this.password = password;
	this.realm = realm;
    }



    private String getinput(String prompt)
    {
	System.out.println(prompt);
	System.out.print(">");

	String result="";
	    
	try {
	    int c;
	    do {
		c = System.in.read();
		if (c!='\n')
		    result+=(char)c;
	    } while (c!='\n');
	    
	    System.out.println("res = "+result);
	} catch (IOException e) {

	}
	
	return result;
    }

    private void getauthid(NameCallback c)
    {
	if (authid!=null) {
	    c.setName(authid);
	    return;
	}

	/*	authid = System.getProperty("user.name");
	if (authid!=null) {
	    c.setName(authid);
	    return;
	    } */

	c.setName( getinput(c.getPrompt()));
    }

    private void getpassword(PasswordCallback c)
    {
	if (password!=null) {
	    c.setPassword(password.toCharArray());
	    return;
	}

	c.setPassword( (getinput("Enter password")).toCharArray());
    }

    private void getrealm(RealmCallback c)
    {
	if (realm!=null) {
	    c.setRealm(realm);
	    return;
	}	  
	
	c.setRealm( getinput(c.getPrompt()) );
    }

    public void invokeCallback(Callback[] callbacks)
	throws java.io.IOException, UnsupportedCallbackException
    {
	for (int lup=0;lup<callbacks.length;lup++)
	{
	    Callback c = callbacks[lup];

	    if (c instanceof NameCallback) {
		getauthid((NameCallback) c);
	    } else if (c instanceof PasswordCallback) {
		getpassword((PasswordCallback) c);
	    } else if (c instanceof RealmCallback) {
		getrealm((RealmCallback) c);
	    } else {
		System.out.println("TODO!");
		System.exit(1);
	    }
	}
    }

    public void handle(Callback[] callbacks) 
	throws java.io.IOException, UnsupportedCallbackException 
    {
	invokeCallback(callbacks);
    }
}
