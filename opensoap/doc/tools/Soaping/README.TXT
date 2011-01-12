Soaping Service Tool
====================

This directory contains the Client and Service code of SoaPING, the
OpenSOAP implementation of PING, which measures and displays the
service response time.

* soaping
  Soaping Client program. The details of usage information can be
  shown with -h option[*1].  The results will be shown like [*2].

* SoapingService
  Soaping Service program using standard I/O.

* SoapingService.cgi
  CGI type Soaping Service program.


[*1]
$ soaping -h
Usage: soaping [-h] [-s size[K|M]] [-c count [-i wait]] [-v] [-o] [URL | host]
Options:
  -h         display this information
  -s size    size of string data to send [[K|M]byte] (default: 0)
  -c count   number of times for continuous transmissions (default: 1)
  -i wait    interval time for continuous transmissions [msec] (default: 0)
  -v         verbose mode.  display SOAP message
  -o         via OpenSOAP server (http://host/cgi-bin/soapInterface.cgi)
URL :  Soaping end-point (default: http://localhost/cgi-bin/SoapingService.cgi)
host : end-point addressed by hostname (default: localhot)
       -> http://host/cgi-bin/{SoapingService.cgi,soapInterface.cgi}

[*2]
$ soaping -s 100 -c 5 localhost
SOAPING http://localhost/cgi-bin/SoapingService.cgi : 100 byte string.
soaping-seq=0 time=12.218 msec
soaping-seq=1 time=9.859 msec
soaping-seq=2 time=9.845 msec
soaping-seq=3 time=9.881 msec
soaping-seq=4 time=9.875 msec

--- http://localhost/cgi-bin/SoapingService.cgi soaping statistics ---
5 messages transmitted, 5 received, 0% loss, time 51.678ms
Round-Trip min/avg/max/mdev = 9.845/10.335/12.218/0.948 msec
