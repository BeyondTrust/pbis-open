package javax.security.auth.callback;

public class PasswordCallback implements Callback
{
    private String prompt;
    private boolean echoOn = false;
    private String password;

    public PasswordCallback(String prompt)
    {
	this.prompt = prompt;
    }

    public PasswordCallback(String prompt, boolean echoOn)
    {
	this.prompt = prompt;
	this.echoOn = echoOn;
    }
    
    public boolean isEchoOn()
    {
	return echoOn;
    }
    
    public String getPassword()
    {
	return password;
    }

    public void setPassword(char[] password) 
    {
	this.password = new String(password);
    }
}
