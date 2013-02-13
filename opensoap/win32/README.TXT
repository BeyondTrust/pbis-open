=begin
= Introduction
 The files in this directory are used for compilation in the win32
environment and include Visual Studio project and workspace files.

= Before compilation
In order to compile this package, the following are required:

 (1) libxml2
 (2) openssl

== Preparing libxml2
 To begin, create the following directories:
  * C:\LIBXML2
  * C:\LIBXML2\BIN
  * C:\LIBXML2\LIB
 - Download and unpack the libxml2 package(libxml2-verion.tar.gz).
 - Compile using libxml2-version\win32\libxml2\libxml2_so.dsp
 - Copy the contents of libxml2-version\include to C:\LIBXML2
 - Copy libxml2-version\win32\libxml2\libxml2_so\libxml2.dll to C:\LIBXML2\BIN
 - Copy libxml2-version\win32\libxml2\libxml2_so\libxml2.lib to C:\LIBXML2\LIB
 - Add C:\LIBXML2\BIN to the path.

== Preparing openssl
 Install the MSC version of openssl in C:\OPENSSL according to the documentation.

= Compilation
 Open OpenSOAP\OpenSOAP.dsw and compile.

 For server configuration see the file OpenSOAPServer\READMESJ.TXT.
=end
