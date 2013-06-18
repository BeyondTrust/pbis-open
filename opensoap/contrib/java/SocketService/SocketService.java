/*-----------------------------------------------------------------------------
 * $RCSfile: SocketService.java,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX Server Side Service
//  MODULE      :  SocketService.java
//  ABSTRACT    :  Calc Service
//  DATE        :  2002.02.20
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8

import java.util.*;
import java.io.*;
import java.net.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class SocketService {
	/** log output flag */
	public boolean logOutputFlag;
	/** port no */
	private int portNo;
	/** listenSocket */
	private ServerSocket listenSocket;

	/**
	   コンストラクタ
	   @param
		port	ポート番号
	   @exception
		IOException
	*/
	public
	SocketService(int port) throws IOException {
		// log output flag
		this.logOutputFlag = true;
		// port No
		this.portNo = port;
		// create listen socket
		this.listenSocket = new ServerSocket(this.portNo);
	}

	/**
	   destruct
	*/
	public
	void
	destruct() throws IOException {
		if (this.listenSocket != null) {
			this.listenSocket.close();
		}
	}

	/**
	   サービス実行
	   @exception
		IOException
		OpenSoapException
	*/
	public
	void
	serviceRun() throws IOException, OpenSoapException {
		// recv socket
		Socket recvSocket = null;
		// input stream
		DataInputStream  inStream = null;
		// output stream
		DataOutputStream  outStream = null;
		try {
			while (true) {
				// recv socket
				recvSocket = listenSocket.accept();
				// get input stream
				inStream = new DataInputStream(recvSocket.getInputStream());
				// get output stream
				outStream = new DataOutputStream(recvSocket.getOutputStream());
				// proc SOAP message
				procSOAPMessage(inStream, outStream);

				// input stream close
				inStream.close();
				inStream = null;
				// output stream close
				outStream.close();
				outStream = null;

				// socekt close
				recvSocket.close();
				recvSocket = null;
			}
		}
		finally {
			if (inStream != null) {
				inStream.close();
			}
			if (outStream != null) {
				outStream.close();
			}
			if (recvSocket != null) {
				recvSocket.close();
			}
		}
	}

	/**
	   Process SOAP Message
	   @param
		inStream	input data stream
	   @param
		outStream	output data stream
	*/
	private
	void
	procSOAPMessage(DataInputStream inStream,
					DataOutputStream outStream) throws IOException, OpenSoapException {
		// get request message
		String requestMessage = recvRequestMessage(inStream);
		//
		if (this.logOutputFlag) {
			System.out.println("-- request soap message --");
			if (requestMessage != null) {
				System.out.println(requestMessage);
			}
			else {
				System.out.println("request message is null");
			}
			System.out.println("-- request soap message --");
		}
		// invoke
		String responseMessage = invokeServiceProc(requestMessage);

		//
		sendResponseMessage(outStream, responseMessage);
		
		// log output
		if (this.logOutputFlag) {
			System.out.println("-- response soap message --");
			if (responseMessage != null) {
				System.out.println(responseMessage);
			}
			else {
				System.out.println("response message is null");
			}
			System.out.println("-- response soap message --");
		}
	}

	/**
	   Recv Request SOAP Message
	   @param
		inStream	input Data Stream
	*/
	private
	String
	recvRequestMessage(DataInputStream inStream) throws IOException, OpenSoapException {
		//
		final String postCommand = new String("POST");
		//
		String line = null;
		// read first line
		if (this.logOutputFlag) {
			System.out.println("-- request header --");
		}
		while (true) {
			line = readLine(inStream);
			if (line.length() >= postCommand.length()) {
				break;
			}
		}
		if (this.logOutputFlag) {
			System.out.println(line);
		}
		if (!line.substring(0, postCommand.length()).equals(postCommand)) {
			// not POST command
			return null;
		}
		//
		int contentLength = 0;
		//
		String charEnc = new String("UTF-8");
		// Header analyze
		while (true) {
			line = readLine(inStream);
			if (this.logOutputFlag) {
				System.out.println(line);
			}
			// line's length is zero
			if (line == null || line.length() == 0) {
				break;
			}
			// string tokenizer
			StringTokenizer headerTokenizer
				= new StringTokenizer(line, ":");
			String headerKey = headerTokenizer.nextToken().trim();
			String headerValue = headerTokenizer.nextToken().trim();
			if (headerKey.equals("Content-Length")) {
				contentLength = Integer.parseInt(headerValue);
			}
			else if (headerKey.equals("Content-Type")) {
				StringTokenizer contentTypeTokenizer
					= new StringTokenizer(headerValue, ";");
				String contentType = contentTypeTokenizer.nextToken().trim();
				if (!contentType.equalsIgnoreCase("text/xml")) {
					// content-type in not XML
					return null;
				}
				//
				while (contentTypeTokenizer.hasMoreTokens()) {
					final String charsetKey = new String("charset");
					String contentTypeElm
						= contentTypeTokenizer.nextToken().trim();
					if (contentTypeElm.substring(0, charsetKey.length())
						.equalsIgnoreCase(charsetKey)) {
						StringTokenizer charsetTokenizer
							= new StringTokenizer(contentTypeElm, "=");
						charsetTokenizer.nextToken();
						if (charsetTokenizer.hasMoreTokens()) {
							String charset
								= charsetTokenizer.nextToken().trim();
							int charsetLength = charset.length();
							if (charsetLength > 0) {
								char first = charset.charAt(0);
								char last  = charset.charAt(charsetLength - 1);
								if (first == last &&
									(first == '\"' || first == '\'')) {
									charset
										= charset
										.substring(1, charsetLength - 1);
								}
							}
							charEnc = charset;
						}
					}
				}
			}
		}
		if (this.logOutputFlag) {
			System.out.println("-- request header --");
		}

		byte[]	httpBody = null;

		if (contentLength != 0) {
			httpBody = new byte[contentLength];
			inStream.read(httpBody);
		}
		else {
			ByteArrayOutputStream	outBody = new ByteArrayOutputStream();

			while (inStream.available() > 0) {
				outBody.write(inStream.readByte());
			}
			httpBody =outBody.toByteArray();
		}

		return new String(httpBody, charEnc);
	}

	/**
	   get SOAP Fault message
	   @param
		faultCode	fault code
	   @param
		faultString	fault string
	   @return
		String		SOAP fault message
	*/
	private
	String
	getSOAPFaultMessage(String faultCode,
						String faultString) {
		return 	"<?xml version='1.0'?>"
			+	"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\'"
			+	OpenSoapConstants.SOAPENV_URI + "\'>"
			+	"<SOAP-ENV:Body>"
			+	"<SOAP-ENV:Fault>"
			+	"<faultcode>SOAP-ENV:" + faultCode + "</faultcode>"
			+	"<faultstring>"
			+	faultString
			+	"</faultstring>"
			+	"</SOAP-ENV:Fault>"
			+	"</SOAP-ENV:Body>"
			+	"</SOAP-ENV:Envelope>";
	}

	/**
	   invoke Service Proc
	   @param
		requestMessage	request message
	   @return
						response message
	*/
	private
	String
	invokeServiceProc(String requestMessage) throws IOException {
		if (requestMessage == null) {
			return getSOAPFaultMessage("Client",
									   "Require POST Command");
		}
		//
		CharArrayWriter payload = new CharArrayWriter();
		try {
			// メッセージのパース
			XMLReader xr = XMLReaderFactory.createXMLReader(
				OpenSoapConstants.SAXPARSER);
			xr.setFeature(OpenSoapConstants.SAXNAMESPACES,true);
			CalcServiceResponse response = new CalcServiceResponse();
			xr.setContentHandler(response);
			StringReader sr = new StringReader(requestMessage.toString()); 
			org.xml.sax.InputSource inputXML = new org.xml.sax.InputSource(sr); 
			xr.parse(inputXML);
			// レスポンス作成
			response.writeResponse(payload);
		}
		catch(OpenSoapException openSoapException) { // OpenSOAPエラー出力
			openSoapException.writeTo(payload);
		}
		catch (SAXException saxException) {
			return getSOAPFaultMessage("Service",
									   saxException.getMessage());
		}
		
		return payload.toString();
	}

	/**
	   Send response message
	   @param
		outStream		output stream
	   @param
		responseMessage	response message
	*/
	private
	void
	sendResponseMessage(DataOutputStream outStream,
						String responseMessage) throws IOException {
		String crlf = new String("\r\n");
		String charset  = new String("UTF-8");
		byte[]  response = responseMessage.getBytes(charset);

		String httpHeader
			= "HTTP/1.0 200 OK" + crlf
			+ "Content-Type: text/xml; charset=\"" + charset + "\"" + crlf
			+ "Content-Length: " + response.length + crlf
			+ crlf;
		
		// レスポンス出力
		outStream.writeBytes(httpHeader);
		outStream.write(response, 0, response.length);
	}

    //--------------------------------------------------------------------------
    // Socket Line Reader
    //--------------------------------------------------------------------------
    private static String readLine(DataInputStream inS) throws IOException, OpenSoapException {
		// 
		StringBuffer s = new StringBuffer();
		//
		char[] crlf = { '\r', '\n'};
		int crlfIndex = 0;
		while (inS.available() > 0 && crlfIndex != crlf.length) {
			char c = (char)inS.readByte();
			s.append(c);
			if (crlf[crlfIndex] == c) {
				++crlfIndex;
			}
			else {
				crlfIndex = 0;
			}
		}

		if (crlfIndex == crlf.length) {
			// CR/LF
			// chop
			s.setLength(s.length() - crlf.length);
		}

		return s.toString();
		/*
        try{
            StringBuffer s = new StringBuffer();
            for( int i = 0 ; ; ){
				int c = inS.readUnsignedByte();
                Character element = new Character((char)c);
				s.append( element );
                if( c == '\n' || c == '\r' || s.toString().indexOf("</SOAP-ENV:Envelope>") >= 0) {
                    break;
                }
            }
            return s.toString();
        }
		catch(EOFException e) {      // EOF
            return null;
        }
		*/
    } // end of readLine()

	

    //--------------------------------------------------------------------------
    // Service Main
    //--------------------------------------------------------------------------
    public static void main(String argv[]) throws IOException, OpenSoapException {
		if (true) {
			// check parameter
			if (argv.length != 1) {
				System.out.println("Usage: java SocketService [<port>]");
				System.exit(1);
			}
			// port no
			int port = Integer.parseInt(argv[0]);
			// Socket Service
			SocketService serv = null;
			try {
				// create service
				serv = new SocketService(port);
			} catch(IOException e){
				System.out.println("Exception(Port): " + e);
				System.exit(1);
			}
			try {
				// service execute
				serv.serviceRun();
			}
			catch (OpenSoapException ose) {
			}
			catch (IOException ioe) {
				System.out.println("Exception(I/O): " + ioe);
			}
			finally {
				serv.destruct();
			}
		}
		else {
			String soapMessage;				// メッセージ格納域
			int sw;							// メッセージ開始SW
			ServerSocket servSock = null;

			if (argv.length != 1) {			// パラメータチェック
				System.out.println("Usage: java SocketService [<port>]");
				System.exit(1);
			}
			int port = Integer.parseInt(argv[0]);

			try {							// ソケットの作成
				servSock = new ServerSocket( port );
			} catch(IOException e){
				System.out.println( "Exception(Port): " + e );
				System.exit(1);
			}

			for(;;){
				Socket theSock = null;		// ソケット初期化
				try {						// コネクション待ち
					theSock = servSock.accept();
				} catch(IOException e){
					System.out.println( "Exception(accept): " + e );
					System.exit(1);
				}
				sw = 0;
				soapMessage = "";
				try {						// 送受信ストリーム作成
					DataOutputStream outS =
						new DataOutputStream(theSock.getOutputStream());
					DataInputStream  inS  =
						new DataInputStream(theSock.getInputStream());
					while (true) {
						String r = readLine(inS); // TCPデータ読込
						//
						System.out.println(r);

						//
						if (sw == 1) {
							soapMessage = soapMessage + r; // XML文章格納
						}
						else if (r.length() == 0) {
							sw = 1;
							continue;
						}
						if (r.indexOf("</SOAP-ENV:Envelope>") >= 0) { // SOAPメッセージ終了
						
							System.out.println("-- request soap message --");
							System.out.println(soapMessage);
							System.out.println("-- request soap message --");
							CharArrayWriter payload = new CharArrayWriter();
							try {
								// メッセージのパース
								XMLReader xr = XMLReaderFactory.createXMLReader(
									OpenSoapConstants.SAXPARSER);
								xr.setFeature(OpenSoapConstants.SAXNAMESPACES,true);
								CalcServiceResponse response = new CalcServiceResponse();
								xr.setContentHandler(response);
								StringReader sr = new StringReader(soapMessage.toString()); 
								org.xml.sax.InputSource inputXML = new org.xml.sax.InputSource(sr); 
								xr.parse(inputXML);
								// レスポンス作成
								response.writeResponse(payload);
							} catch(OpenSoapException e) { // OpenSOAPエラー出力
								e.writeTo(payload);
							} catch (Exception e) {	// I/Oエラー出力
								System.out.println( "Exception(read): " + e );
								e.printStackTrace();
								System.exit(1);
							}
							// レスポンス出力
							outS.writeBytes( "HTTP/1.1 200 OK\r\n");
							outS.writeBytes( "Content-Type: text/xml; charset=\"UTF-8\"\r\n");
							outS.writeBytes( "Content-Length: " + payload.size() + "\r\n\r\n");
							outS.writeBytes( payload.toString());
							outS.flush();
							outS.close();
							inS.close();
							theSock.close();

							System.out.println("-- response soap message--");
							System.out.println(payload.toString());
							System.out.println("-- response soap message--");
							break;
						}
					}
				}
				catch(Exception e) {
					System.out.println( "Exception(I/O): " + e );
					System.exit(1);
				}
			}
		}
	}
}
