Echo Service & Client
===========================

Essentially the purpose of this client/service program set is simply
to receive and return a value for various test scenarios.
As this service has been designed to accept a number of data types,
unlike the Hello sample, error checking is performed, and includes
code for checking recent extensions to the OpenSOAP
API.(Autentication, Certification, etc).
Eventually, it is hoped that this tool can be used in compatibility
testing, and also to perform automatic regression testing prior to a
release.

* EchoClient
  This is the Echo client program.

  Usage: EchoClient [options] test# data [data2 [data3 ..]]
  Options:
    -h --help      Display help message
    -v --verbose   Verbose mode. Display SOAP Messages
    -d --debug     Debug mode. Display debug messages
    -s --endpoint=ENDPOINT The EchoService endpoint
                           (default: http://localhost/cgi-bin/EchoService.cgi)
    -u --http-user=USER  HTTP Authentication username
                           (default: env ECHO_HTTP_USER)
    -p --http-passwd=PASS  HTTP Authentication password
                           (default: env ECHO_HTTP_PASS)
    -t --http-type=AUTH  HTTP Authentication type(default: 0=automatic,
                            1=Basic, 2=Digest)
    -S --sslver=VER  Specify the SSL Version used
                           (default: 0=ALL, 1=SSLv2, 2=SSLv3, 4=TLSv1 (Bit ORed) )
    -V --verify=LEVEL  Specify the SSL Server Certification Level
                           (default:0=Don't care, 1=Strict)
    --ca_dir=DIR  Specify the directory containing the CA Certificates
    --ca_file=FILE  Specify the filename containing the CA Certificate
    --certchain_file=FILE  Client Authentication chain file name
    --privkey_file=FILE  Client Authentication Private Key file name

 test# = Method name

 test:0 = Echo
  Return the received message as is. * Not yet implemented *

 test:1 = echoString
  Send the specified character string.

  Ex
  $ EchoClient 1 foo

 test:2 = echoStringArray
  Send the specified array of character strings.  * Not yet implemented *

  Ex
  $ EchoClient 2 foo bar

 test:3 = echoInteger
  Send the specified integer.

  Ex
  $ EchoClient 3 1234567890

 test:4 = echoIntegerArray
  Send the specified array of integers.  * Not yet implemented *

  Ex
  $ EchoClient 4 1 2 3 4 5 6 -1 -2 -3

 test:5 = echoFloat
  Send the specified floating point value.

  Ex
  $ EchoClient 5 3.141592

 test:6 = echoFloatArray
  Send the specified array of floating point values.  * Not yet implemented *

  Ex
  $ EchoClient 6 1.1 1.2 1.3 1.4 1.5

 test:7 = echoStruct
  Send the specified data structure.  * Not yet implemented *

  Ex
  $ EchoClient 7 filename ?

 test:8 = echoStructArray
  Send the specified array of data structures.  * Not yet implemented *

  Ex
  $ EchoClient 8 filename1 filename2 filename3 ?

 test:9 = echoVoid
  Send a void.  * Not yet implemented *

  Ex
  $ EchoClient 9

 test:10 = echoBase64
  Send the specified file as a Base64 encoding.

  Ex
  $ EchoClient 10 file

 test:11 = echoDate
  Send the specified date. If no date specified, send current date and time.

  Ex
  $ EchoClient 11
  Ex
  $ EchoClient 11 "Fri Nov 7 11:50:23 JST 2003"

 test:12 = echoDouble
  Send the specified double value.

  Ex
  $ EchoClient 5 3.1415926535897932384626433832795029

* EchoService
   This is the Standard I/O implementation of the Echo Service.

* EchoService.cgi
   This is the CGI implementation of the Echo Service.

As the processing consists of storing the received value in a variable
of the respective data type, a mismatch of value and data type may
result in the generation of an error.
