 Running the OpenSOAP Server

(0) To run the OpenSOAP Server a HTTP Server is required.
At this time, operation with apache httpd has been verified.

(1) In accordance INSTALL.txt, build and install the OpenSOAP Server.
Pay particular attention to the directory where soapInterface.cgi is installed.
This is used to connect apache httpd and the OpenSOAP Server.
The default setting places it in /home/httpd/cgi-bin, and, if necessary, use
the --with-cgi-bin to specify the CGI executable directory.

(2) By default, a service uses standard I/O for connection, but this behaviour
can be changed to socket connection by making the appropriate modifications to
the SSML settings file. Also, refer to RegistService.txt for instructions on
registering the service with inetd (xinetd).

A warning will be issued indicating that it is an error to run the service
programs as root.

Furthermore, by using the SSML settings, connecting to a service by specifying
the endpoint used by HTTP is now supported.
Please refer to the SSML_Readme.txt file for details.

(3) Edit the server settings file(server.conf) to be found under /usr/local/opensoap/etc/.

* Log files for all server processes

  <log>
    <path>/usr/local/opensoap/var/log/</path>
  </log>

* Server internal SOAP Message temporary directory

  <spool>
    <soap_message>
      <path>/usr/local/opensoap/var/spool/</path>
    </soap_message>

* Server internal asynchronous message temporary directory

    <async_table>
      <path>/usr/local/opensoap/var/spool/</path>
    </async_table>
  </spool>

* Server Processes ID management directory

  <run>
    <pid>
      <path>/usr/local/opensoap/var/run/</path>
    </pid>

* Server Processes Socket management directory

    <socket>
      <path>/usr/local/opensoap/var/run/</path>
    </socket>
  </run>

* Server Internal Signature Authentication management directory

  <security>
    <keys>
      <path>/usr/local/opensoap/etc/</path>
    </keys>
  </security>

* Service management SSML file registration directory

  <ssml>
    <path>/usr/local/opensoap/etc/ssml/</path>
  </ssml>

* Use the following setting when specifying the return EndPoint for an
  asynchronous request message. The following setting is required in order
  to indicate to the message responder where to send the message, and must
  be set to this server's location.
  Also, in order to check message transfer loop, if the server has another name
  or IP address, these should be listed as shown below.

  In the following example the response will be sent to
    http://myhost.opensoap.jp/cgi-bin/soapInterface.cgi

  <backward>
    <url>http://myhost.opensoap.jp/cgi-bin/soapInterface.cgi</url>
    <url>http://192.168.0.123/cgi-bin/soapInterface.cgi</url>
    <url>http://soap-server.opensoap.jp/cgi-bin/soapInterface.cgi</url>
  </backward>

* When the appropriate service is not available in this server, use the following
  setting to specify the EndPoint server to which the message will be forwarded.
  The server EndPoint is set in the <url>.
  In the following example, the message will be forwarded to
    http://yourhost.opensoap.jp/cgi-bin/soapInterface.cgi

  <forwarder>
    <url>http://yourhost.opensoap.jp/cgi-bin/soapInterface.cgi</url>
  </forwarder>

* Regarding server control messages for asynchronous message ID, etc.,
  the following setting is used to specify whether the server's signature
  is attached or not. When adding a signature, set this to true.

  <add_signature>false</add_signature>

* Setting the maximum size for a received SOAP message.
  It is now possible to set a limit on the maximum size for a received SOAP
  message. The message size is defined in bytes, but it is also possible
  to use 500k, 1M etc.
  Use negative values (ie. -1) to set no maximum limit.
  A value of 0 means no SOAP message will be accepted.

  <limit>
    <soap_message_size>1M</soap_message_size>
  </limit>

* For asynchronous processing, for incomplete internally queued processes,
  it is possible to set a cancellation time for the spool data. This is
  defined as seconds. Values less than 0 are not allowed.
    (If this is not set, it defaults to 3600 seconds)

  <limit>
    <asynchronizedTTL>3600</asynchronizedTTL>
  </limit>

  ** It is possible to set this value in the SSML file for each service, and
     this value, if set in the SSML file, takes precedence over all others.

  ** Also, it is possible to specify this value in each individual SOAP
     message header. In this instance, if the value specified in the SOAP
     header is LESS than that of the SSML file, then it takes precedence.
     If there is no value specified in the SSML file, then the value in the 
     header is used if it is less than that specified in the server.conf file.

       Example usage:
       server.conf  |  10  10  10  10  10  10
       SSML         |   -  20   5  20  20   -
       SOAP-header  |   -   -   -  30  15  15
       --------------------------------------
       Value used   |  10  20   5  20  15  10

* For synchronous process, it is possible to set the processing time-out in
  seconds for each SOAP message. Values less than 0 are not allowed.
    (If this is not set, it defaults to 600 seconds)

  <limit>
    <synchronizedTTL>600</synchronizedTTL>
  </limit>

  ** Using this value in the SSML file or in the SOAP message header is the
     same as for the <asynchronizedTTL> usage described above.

* Setting a limit on the number of allowable transfers of a SOAP message.
  If this is set to 0, no transfer is allowed. Negative values are not allowed.
   (If this is not set, the internal system default of 4 is used)

  <limit>
    <ttl>
      <hoptimes>4</hoptimes>
    </ttl>
  </limit>

  ** As before, usage of this value in th SSML file and SOAP header is the same
     as for the <asynchronizedTTL> value described above.

* The following section is a list of settings relating to log outputs.

  <Logging><System><LogType>
        Log type. syslog=output to syslog. file=output to a file.
        The default is output to syslog.

  <Logging><System><LogFormat>
        Log output contents. generic=generic log data. detail=detailed log data.
        The default is generic.

  <Logging><System><Option>
        Options for LogType=syslog. Set parameters for syslog.
        Set the value as an addition of required options.

        1=PID:                Display Process ID
        2=CONSOLE:            Output to console
        4=ODELAY:             Connect to syslog when initializing logs
        8=NDELAY:             Connect to syslog when writing first log data
        16=NOWAIT:            When writing to syslog, don't wait for child
	                      processes that may have been created
        32=PERROR:            Send log data to stderr also.

  <Logging><System><Facility>
        Options for LogType=syslog. Set the syslog facility.

        0=KERN
        8=USER
        24=DAEMON
        128=LOCAL0
        136=LOCAL1
        144=LOCAL2

        Default is 8.

  <Logging><System><DefaultOutputLevel>
        Options for LogType=syslog. Only used when connecting to syslog.
        Not currently used . Currently only used for development 
        purposes, and cannot be modified.

  <Logging><System><LogLevel>
        Shared options. Sets the log output level.  Outputs messages for 
        levels up to specified level.

        0=EMERG:              Critical Error
        2=ALERT:              Serious Error
        3=ERR:                Error
        4=WARN:               Warning
        5=NOTICE:             Notification
        6=INFO:               Information
        7-16=DEBUG:           Additional detailed messages
                              (For debugging purposes)

  <Logging><Application><LogType>
        Same as for the <Logging><System><LogType> logging option.
        The default is file.

  <Logging><Application><LogFormat>
        Same as for the <Logging><System><LogFormat> logging option.
        The default is detail.

  <Logging><Application><FileName>
        Option for LogType=file.
        This is the complete path name for the log file.

  <Logging><Application><LogLevel>
        Same as for the <Logging><System<LogLevel> option.

* It is possible to seperate the log files for each of the OpenSOAP 
  Server processes by specifying the "process name" as follows, (similar
  to the <Logging><Application> setting above):

  <Logging><"process name">


(4) The service settings files, SSML files, are place under /usr/local/opensoap/etc/ssml/ .
    Refer to the file SSML_Readme.txt, in the same directory, for a description of SSML usage.

(5) Verify that the apache httpd is running.

If not, start the httpd server.

(6) Initiating the server process.

This is performed through the path ${bindir}.(The default is /usr/local/opensoap/sbin)

(6-00) Execute the command ${bindir}/opensoap-server-ctl start

* 1: To stop the server use the command ${bindir}/opensoap-server-ctl stop

* 2: When additions are made to the SSML execute the command ${bindir}/opensoap-server-ctl reload .

=============
 Process List
=============

* 3: In the case of (6-00) above, execute the commands (6-01) to (6-09) below.

(6-01) OpenSOAPMgr
        This is the management process for all the server sub-processes, 
        and controls execution, termination, etc.

(6-02) srvConfAttrMgr 
          Server configuration attribute manager process

(6-03) ssmlAttrMgr 
          SSML attribute manager process

(6-04) idManager 
          Internal messaging manager process

(6-05) msgDrvCreator 
          Message processing distribution manager process

(6-06) queueManager 
          Message queue manager process

(6-07) queueManager 1 
          Transfer message queue manager process

(6-08) spoolManager 
          Asynchronous response message spool manager process

(6-09) ttlManager 
          Message TTL manager process

* 4: Use this OpenSOAP Server process list to manually terminate any remaining processes.

(7) Run the client program.

======================================
 OpenSOAP Server Debug/Bug Reporting


(1) All program logs can be found under /usr/local/opensoap/var/log/ .
Please check here when any errors are encountered.(Default server.conf)

(2) Use the CXXFLAGS=-DDEBUG configure option to generate detailed logs.

(3) If any program terminates abnormally, its name will not be displayed
on execution of the command opensoap-server-ctl stop. This indicates that
the program terminated prematurely. Please feel free to report any errors
or bugs that are encountered. 

