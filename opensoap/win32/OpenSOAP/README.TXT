=begin
= Introduction
 The files in this directory are used for compilation in the win32
environment and include Visual Studio project and workspace files.

= File List
 This directory contains the following files:
  * README.TXT              This file
  * OpenSOAP.dsw            Workspace file for API construction.
  * *.dsp                   Project files for API construction.
  * OpenSOAPAPIInstall.bat  Batch file to install the API.
  * Makefile*               File required to build the OpenSOAP package.

= Before compiling
To compile this package, the following tools are required.

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
 Install the MSC version of this package in C:\OPENSSL as per the instructions
in the openssl documentation.

= Compilation
 In Visual Studio, open the file OpenSOAP.dsw and compile. Open the "Batch Build"
 dialog box under the "Build" menu and select the projects to compile. Rebuild
 the project with "Rebuild" of the "Build" menu.

= Installation
 Run the OpenSOAPAPIInstall.bat batch file. This will install the OpenSOAP API
 components under C:\OpenSOAP. After installation, by adding C:\OpenSOAP\bin to
 the PATH, it will be possible to use the OpenSOAP API in applications as a DLL.

= Usage
 
 In Visual Studio, click on "settings..." for the Project(P) pull down menu.
 In the dialog, select the "C/C++" tab. Set the Category(Y) to Preprocessor
 and in the Include File PAth(N) input, add C:\OpenSOAP\include.
 Furthermore, setting the Category to "Code generation", change the Runtime Library(L)
 item to Multi-thread(DLL).
 Next, select the "link" tag and select "input" for Category.
 In the Object/Library module(M) input, add the library.
 The libraries added here may be different for each application being built.
 If you are building the Client application, OpenSOAPClient.lib should be added here.
 For a Service application, the OpenSOAPService.lib is added.
 For a security module application, the library OpenSOAPSecurity.lib is added.
 Next, the path of the added library is added to Added Library Path(D).
 If you are using a DLL, add C:\OpenSOAP\lib\DLL here. If you are using static libraries,
 add C:\OpenSOAP\lib. If both are added, then the first one added takes precedence.
 Finally, when building a service, build as a Console application.

=end
