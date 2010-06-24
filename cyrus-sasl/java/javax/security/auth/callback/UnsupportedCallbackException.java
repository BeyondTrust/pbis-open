package javax.security.auth.callback;


public class UnsupportedCallbackException extends Exception
{
    Callback callback;

    public UnsupportedCallbackException(Callback callback)
    {
	super();
	this.callback = callback;
    }

    public UnsupportedCallbackException(Callback callback, String msg) 
    {
	super(msg);
	this.callback = callback;
    }

    public Callback getCallback()
    {
	return callback;
    }

}
