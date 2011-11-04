package CyrusSasl;

import java.io.*;

public class SaslInputStream extends InputStream
{
    static final boolean DoEncrypt = true;
    static final boolean DoDebug = false;
    private static int BUFFERSIZE = 16384;

    // if bufferend < bufferstart, we've wrapped around
    private byte[] buffer=new byte[BUFFERSIZE];
    private int bufferstart = 0;
    private int bufferend = 0;
    private int size = 0;

    private GenericCommon conn;

    public InputStream in;
    
    public SaslInputStream(InputStream in, GenericCommon conn)
    {
	if (DoDebug) {
	    System.err.println("DEBUG constructing SaslInputStream");
	}
	this.in = in;
	this.conn = conn;
    }

    public synchronized int available() throws IOException
    {
	int ina = in.available();
	if (ina > 1) ina = 1;
	
	return size + ina;
    }
    
    private void buffer_add(byte[] str,int len) throws IOException
    {
	if (str == null) {
	    // nothing to add
	    return;
	}
	
	byte[] b = str;
	
	/* xxx this can be optimized */
	for (int lup=0;lup<len;lup++) {
	    buffer[bufferend]=b[lup];
	    bufferend = (bufferend + 1) % BUFFERSIZE;

	    size++;
	    if (size >= BUFFERSIZE) {
		throw new IOException();
	    }
	}
    }
    
    private void buffer_add(byte[] str) throws IOException
    {
	buffer_add(str,str.length);
    }

    private void readsome() throws IOException
    {
	int len=in.available();

	if (DoDebug) {
	    System.err.println("DEBUG in readsome(), avail " + len);
	}

	if (len > BUFFERSIZE || len == 0)
	    len = BUFFERSIZE;
	
	byte[]tmp=new byte[len];
	len = in.read(tmp);
	
	if (len>0) {
	    if (DoEncrypt) {
		buffer_add( conn.decode(tmp,len) );
	    } else {
		buffer_add(tmp, len);
	    }
	}
    }

    public synchronized void close() throws IOException
    {
	super.close();
    }

    public synchronized void reset() throws IOException
    {
	throw new IOException();
    }

    public synchronized void mark(int readlimit)
    {
	return;
    }
    
    public boolean markSupported()
    {
	return false;
    }

    /* read a single byte */
    public synchronized int read() throws IOException
    {
	int ret;
	
	if (DoDebug) {
	    System.err.println("DEBUG in read(), size " + size);
	}
	if (size == 0) {
	    readsome();
	}
	
	if (size == 0) {
	    if (DoDebug) {
		System.err.println("DEBUG read() returning -1");
	    }
	    return -1;
	}
	
	ret = buffer[bufferstart];
	bufferstart = (bufferstart + 1) % BUFFERSIZE;
	size--;

	if (DoDebug) {
	    System.err.println("DEBUG read() returning " + ret);
	}
	return ret;
    }

    public synchronized int read(byte b[]) throws IOException
    {
	return read(b,0,b.length);
    }

    public synchronized int read(byte b[],
				 int off,
				 int len) throws IOException 
    {
	if (DoDebug) {
	    System.err.println("DEBUG in read(b, off, len), size " + size);
	}
	if (off < 0 || len < 0) {
	    throw new IndexOutOfBoundsException();
	}
	if (len == 0) {
	    return 0;
	}

	// block only if we need to
	if (size == 0) {
	    readsome();
	    if (size == 0) {
		if (DoDebug) {
		    System.err.println("DEBUG read(b, off, len) returning -1");
		}
		return -1;
	    }
	}

	int l;
	for (l = off; l < len + off; l++) {
	    if (bufferstart == bufferend) break;

	    b[l] = buffer[bufferstart];
	    bufferstart = (bufferstart + 1) % BUFFERSIZE;
	    size--;
	}

	if (DoDebug) {
	    System.err.println("DEBUG read() returning " + (l - off));
	}
	return l - off;
    }
    
    public synchronized long skip(long n) throws IOException
    {
	if (n<=0) return 0;

	long toskip = n;
	while (toskip > 0) {
	    if (size == 0) {
		readsome();
		if (size == 0) {
		    return n - toskip;
		}
	    }
	    
	    if (toskip > size) {
		toskip -= size;
		bufferstart = bufferend = size = 0;
	    } else {
		// we've got all the data we need to skip
		size -= toskip;
		bufferstart = (int) ((bufferstart + toskip) % BUFFERSIZE);
	    }
	}
	
	// skipped the full amount
	return n;
    }
}

