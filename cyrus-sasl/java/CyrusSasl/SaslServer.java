package CyrusSasl;

import java.io.*;

public interface SaslServer
{
    public byte[]
	evaluateResponse(byte[] challenge)
	throws SaslException;


    public boolean isComplete();

    public String getMechanismName();

    public InputStream getInputStream(InputStream source) throws IOException;

    public OutputStream getOutputStream(OutputStream dest) throws IOException;

}
