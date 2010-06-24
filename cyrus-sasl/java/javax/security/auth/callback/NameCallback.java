package javax.security.auth.callback;

public class NameCallback implements Callback
{
    private String prompt;
    private String defaultName;
    private String name;

    public NameCallback(String prompt)
    {
	this.prompt = prompt;
    }

    public NameCallback(String prompt, String defaultName) 
    {
	this.prompt = prompt;
	this.defaultName = defaultName;
    }

    public String getDefaultName() 
    {
	return defaultName;
    }
    
    public String getPrompt()
    {
	return prompt;
    }
    
    public String getName()
    {
	return name;
    }

    public void setName(String name) 
    {
	this.name = name;
    }
}

