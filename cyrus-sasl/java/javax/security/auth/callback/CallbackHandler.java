
package javax.security.auth.callback;

public abstract interface CallbackHandler
{
    public void handle(Callback[] callbacks)
	throws java.io.IOException, UnsupportedCallbackException;
}
