OpenSOAP Server Apache DSO Module
==================================

The OpenSOAP Server uses the Apache CGI interface to receive messages
over the HTTP transport. For relatively small message sizes, this is
not a particular problem, but with increased message size, the
performance degrades considerably. As a result, this version of the
OpenSOAP server is now capable of receiving HTTP transport messages
through the Apache DSO module interface. This has resulted in
achieving up to 20% gain in performance.

Also, message attachments which have been unsupported in previous
releases are now supported by the OpenSOAP server. It is possible to
send messages with attachments that comply with WS-Attachments and
SOAP with Attachments (SwA) standards DIME/MIME.

To make use of this modules function, Apache 2 must be configured for
use with DSO modules and the necessary configurations must be made to
httpd.conf.


Compilation
==========

To compile, make sure that the httpd-devel package from the Apache packages
is installed, and that the command apxs (APache eXtenSion tool) is also
installed and can be executed.
If both of the above requirements are met, running configure will
automatically generate the required source files. For compile details
refer to the INSTALL file in the documentation.

On successful make, the generated module file name is mod_opensoap.so and
should be installed in /usr/lib/httpd/modules/ (RedHat9).


Configuration
============

In order to use the mod_opensoap functions, the following must be added
to the contents of the httpd.conf file.

#####The following lines need to be included in Apache2 httpd.conf
#####for OpenSOAP Server DSO
LoadFile /usr/local/opensoap/lib/libOpenSOAPServer.so
LoadFile /usr/local/opensoap/lib/libOpenSOAPInterface.so
LoadModule opensoap_module modules/mod_opensoap.so
<Location /opensoap>
SetHandler opensoap
</Location>


The LoadFile description line lists the OpenSOAP server library files,
and these should be found in one of the following locations
/usr/local/opensoap/lib/libOpenSOAPServer.so
/usr/lib/libOpenSOAPServer.so

For RedHat7.3, it may be necessary to specify the path for the libstdc++
library.
#(libstdc++ is only for RedHat7.3)
LoadFile /usr/lib/libstdc++-3-libc6.2-2-2.10.0.so

For the changes to take effect, both the OpenSOAP server and the httpd
service
must be restarted.

Ex.
# /etc/init.d/httpd restar
# /etc/init.d/opensoap restart
or
# apachectl restart
# opensoap-server-ctl restart


Access and Verification
====================
For access through the DSO module, the endpoint is no longer
soapInterface.cgi
but is instead http://hostname/opensoap

$ soaping http://localhost/opensoap
SOAPING http://localhost/opensoap : 0 byte string.
soaping-seq=0 time=14.619 msec

--- http://localhost/opensoap soaping statistics ---
1 messages transmitted, 1 received, 0% loss, time 14.619ms
Round-Trip min/avg/max/mdev = 14.619/14.619/14.619/0.000 msec
