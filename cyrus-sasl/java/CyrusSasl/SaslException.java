package CyrusSasl;

import java.io.IOException;

public class SaslException extends IOException
{
    private int foo;

    public SaslException()
    {
	super();
	foo = 3;

    }

    public SaslException(String message)
    {
	super(message);
    }
    
    public SaslException(String message,
			 Throwable ex)
    {

    }

    public Throwable getException()
    {
	return null;
    }

    public void printStackTrace()
    {

    }





}
