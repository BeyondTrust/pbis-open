package CyrusSasl;

import java.io.*;

public interface SaslClient
{
    public byte[]
	evaluateChallenge(byte[] challenge)
	throws SaslException;


    public boolean hasInitialResponse();
	
    public boolean isComplete();

    public String getMechanismName();

    public InputStream getInputStream(InputStream source) throws IOException;

    public OutputStream getOutputStream(OutputStream dest) throws IOException;


}
