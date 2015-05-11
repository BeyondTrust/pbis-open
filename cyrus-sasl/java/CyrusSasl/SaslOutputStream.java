package CyrusSasl;

import java.io.*;

public class SaslOutputStream extends OutputStream
{
    static final boolean DoEncrypt = true;
    static final boolean DoDebug = false;

    private static int MAXBUFFERSIZE=1000;
    private GenericCommon conn;
    OutputStream out;
    
    private byte[] buffer=new byte[MAXBUFFERSIZE];
    private int buffersize=0;

    public SaslOutputStream(OutputStream out, GenericCommon conn)
    {
	if (DoDebug) {
	    System.err.println("DEBUG constructing SaslOutputStream");
	}
	this.conn=conn;
	this.out=out;
    }

    private void write_if_size() throws IOException
    {
	if (DoDebug) {
	    System.err.println("DEBUG write_if_size(): buffersize " + 
			       buffersize);
	}
	if ( buffersize >=MAXBUFFERSIZE)
	    flush();
    }

    public synchronized void write(int b) throws IOException
    {
	buffer[buffersize]=(byte) b;
	buffersize++;
	write_if_size();
    }

    public synchronized void write(byte b[]) throws IOException
    {
	write(b,0,b.length);
    }

    public synchronized void write(byte b[],
				   int off,
				   int len) throws IOException
    {
	if (DoDebug) {
	    System.err.println("DEBUG writing() len " + len);
	}
	if (len+buffersize < MAXBUFFERSIZE) {
	    for (int lup=0;lup<len;lup++) {   
		buffer[buffersize+lup]=b[lup+off];
	    }
	    buffersize+=len;
	    
	    write_if_size();
	    
	} else {
	    flush();
	    
	    if (DoEncrypt && conn != null) {
		// ok, this is a messy way of doing byte[] sub-arraying
		String str=new String(b,off,len);
		out.write( conn.encode(str.getBytes()) );
	    } else {
		out.write(b);
	    }
	    out.flush();
	}

	if (DoDebug) {
	    System.err.println("DEBUG writing(): done");
	}
    }

    public synchronized void flush() throws IOException
    {
	if (DoDebug) {
	    System.err.println("DEBUG flushing(): buffersize " + buffersize);
	}
	if (buffersize==0) return;

	if (DoEncrypt && conn != null) {
	    // ok, this is a messy way of doing byte[] sub-arraying
	    String str = new String(buffer, 0, buffersize);
	    out.write( conn.encode(str.getBytes()) );
	} else {
	    out.write(buffer, 0, buffersize);
	}
	out.flush();
	buffersize=0;
	if (DoDebug) {
	    System.err.println("DEBUG flushing(): done");
	}
    }

    public synchronized void close() throws IOException
    {
	flush();
	out.close();
    }


}
