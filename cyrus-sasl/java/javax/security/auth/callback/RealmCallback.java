package javax.security.auth.callback;

public class RealmCallback implements Callback
{
    private String prompt;
    private String defaultName;
    private String name;

    public RealmCallback(String prompt)
    {
	this.prompt = prompt;
    }

    public RealmCallback(String prompt, String defaultName) 
    {
	this.prompt = prompt;
	this.defaultName = defaultName;
    }

    public String getDefaultRealm() 
    {
	return defaultName;
    }
    
    public String getPrompt()
    {
	return prompt;
    }
    
    public String getRealm()
    {
	return name;
    }

    public void setRealm(String name) 
    {
	this.name = name;
    }
}

