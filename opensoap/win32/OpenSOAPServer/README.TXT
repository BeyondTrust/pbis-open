OpenSOAP Server for Windows2000 Installation Procedure

------------
Conditions :
------------
 (1) libxml2 is properly installed and configured
    The default installation directory is assumed to be C:\libxml2.
    If the installation directory is different to the default, then all the projects
    registered in OpenSOAPServer.dsw should have the C:\libxml2\ path changed to
    reflect this. Also, libxml2\bin should be added to the system path.
 (2) openssl is properly installed and configured
    The default installation directory is assumed to be C:\openssl.
    If the installation directory is different to the default, then all the projects
    registered in OpenSOAPServer.dsw should have the C:\openssl\ path changed to
    reflect this. Also, openssl\bin should be added to the system path.
 (3) Apache is properly installed and configured
    If the default installation directory has been changed, edit the APACHE_CGI_DIR
    setting in the file installOpenSOAPServer.bat to match this.
 (4) Compilation of the OpenSOAP Server using OpenSOAP.dsw has been properly carried out
    as per win32\README.TXT.

--------------
Installation :
--------------

 (1) Open the OpenSOAP Server OpenSOAPServer.dsw, select "Batch Build" from the "Build"
    pull-down menu, and click on the "Build" button.
    * To verify that the server functions properly build using Release mode and not
      the Debug mode. In all the projects, de-select the Debug mode option and use
      Release mode only to build.

 (2) Execute the batch file etc\installOpenSOAPServer.bat.
    This will create the directory %SystemDrive%\OpenSOAP and will copy the required
    DLL's and exe files into it.
    Also, the execution environment for the OpenSOAP Server will be created under
    %SystemDrive%\usr\local\opensoap\ . Log files and temporary message files will be
    output to this execution environment.

  * It is possible to customize the execution environment. In this case, modify the
    OPENSOAP_ROOT value in the batch file installOpenSOAPServer.bat to reflect any
    change made in the location of the execution environment.
    CAUTION : Any files that reference the contents of the directory /usr/local/opensoap
              should also be customized to refer to the appropriate directory. Also the
              OpenSOAP Server process should be restarted.

  Files Requiring Modification :
        A) \OpenSOAP\etc\StartServerWinService.bat
           - Change the setting OPENSOAP_ROOT to matchthat of the file installOpenSOAPServer.bat .
        B) \OpenSOAP\etc\server.conf
           - Change all references to the execution environment of the server processes.
             For example, if \usr\local\opensoap\ is changed to \opensoap\, then
                 <!-- log -->
                   <log>
                     <path>/usr/local/opensoap/var/log/</path>
                   </log>

             should be changed to

                 <!-- log -->
                   <log>
                     <path>/opensoap/var/log/</path>
                   </log>

             Similarly all references to the path /usr/local/opensoap should also be changed.

 * Previously C: refers to %SystemDrive% and this will depend on how the operating system
   has been installed on the machine being used.

 (3) Add C:\OpenSOAP\bin and C:\OpenSOAP\user\bin to the system path.

 (4) Execute the batch file C:\OpenSOAP\etc\SetUpServerWinService.bat.
    This registers all of OpenSOAPServer processes as Windows Services.
    * To remove the OpenSOAPServer processes, simply run the batch file
      C:\OpenSOAP\etc\DeleteServerWinService.bat.

-----------------------
Execution Environment :
-----------------------
    On installation, the directory C:\usr\local\opensoap will be created. Under this
    directory, the following sub-directories will also be created.
    * Except for (1) below, the items (2) - (4) will change depending on any cutomization
      specified in (2) of the installation process.

 (1) C:\OpenSOAP\etc\server.conf
      This is the location for the OpenSOAP Server configuration files.
      * Referring to the section dealing with server.conf in the file doc\server\README.TXT
        of the package, this file contains settings for path, forwarding server information, etc.

 (2) etc\ssml
      This is the location for the service definition SSML files.
      * When the server process is initialised, it looks here for any information regarding the
        administration of services. (service name).ssml files must be properly defined and placed
        here.

 (3) var\spool
      Temporary message and table repository for server internal processing.

 (4) var\log
      OpenSOAP Server logging files location.


--------------------------------------
Starting the OpenSOAP Server Process :
--------------------------------------
 (1) Execute C:\OpenSOAP\etc\StartServerWinService.bat .

--------------------------------------
Stopping the OpenSOAP Server Process :
--------------------------------------
 (1) Execute C:\OpenSOAP\etc\StopServerWinService.bat .

--------------------------------------------------
Reloading the OpenSOAP Server configuration data :
--------------------------------------------------
    If the service SSML files
    under ssml\ are modified, it is necessary to restart the OpenSOAP Server.
 (1) Reloading the OpenSOAP Server can be achieved by executing the batch file
      C:\OpenSOAP\etc\reloadServerWinService.bat .

    * In order to reflect any changes made to the configuration files the OpenSOAP
      Server MUST be reloaded. This can be achieved by executing the batch file above
      or by a combination of stopping the server followed by starting the server.

------------------------------
Test Sample Execution Example:
------------------------------
  Below, for illustrative purposes, is the procedure for running the Hello sample service.

  (1) Layout of the sample client and service

      Perform a build using the HelloClient_DLL.dsp and HelloService_DLL.dsp located under
      the directory win32\OpenSOAPSamples\ of the package. Copy the files HelloClient.exe
      and HelloService.exe in the directory Debug_DLL  to C:\opensopa\user\bin.

      * For the HelloClient sample program, the 2nd parameter of the call to the
        OpenSOAPTransportSetURL() function determines whether the service is called in CGI
        mode or through the OpenSOAP Server. This must be set to calling through the server.
        The default compile switch (#if 1) calls the service in CGI mode. In order to
        call the service through the OpenSOAP Server, edit this switch to (#if 0)
        in HelloClient.c and re-build.

      * With regard to the service modules, win32\OpenSOAPSamplesHelloService_cgi_DLL.dsp etc,
        the files *_cgi_DLL.dsp correspond not to programs being called through the OpenSOAP server,
        but rather programs that are called directly through CGI. When using the project file
        win32\OpenSOAPSamples\OpenSOAPSamples.dsw to do a build make sure to build the services
        for the server and not for CGI.

  (2) Service Registration using SSML files

      Copy the file samples\Hello\Hello.ssml to the appropriate location (as defined in server.conf),
      modify the the service installation path to match where the service program is located and
      restart the OpenSOAP server.

      Before modification
        <StdIO>
          <exec prog='/usr/local/opensoap/services/Hello/HelloService'/>
        </StdIO>

      After modification
        <StdIO>
          <exec prog='/opensoap/user/bin/HelloService'/>
        </StdIO>

  (3) Run the sample

     From thecommand prompt run the HelloClient program.
     Ex.
        > HelloClient OpenSOAP

    Instructions to run the test sample can be found in the directory etc\SimpleCalcSample .
    Please refer to the etc\SimpleCalcSample\README.TXT file for more details.


-----------------
Troubleshooting :
-----------------

  If the operation of the server is not according to expectations check the following:

  (1) Has the system path been properly set ?
     By default, it is required to add the following to the system path:
     C:\libxml2\bin;C\openssl\bin;C:\opensoap\bin;C:\opensoap\user\bin

  (2) Is the server.conf file in its proper loaction, and are its contents correct ?
     By default, this file must be located in C:\opensoap\etc.
     As the initial locations for logging, spooling and SSML files are under /usr/local/opensoap/...
     you MUST modify the contents of the server.conf file if any changes are made to these locations.
     Also, any directories that are specified MUST exist.

  (3) Have the relevant SSML files been placed in their proper locations, and are the contents
     of these files correct ?
     The SSML files must be placed in the location defined by the file C:\opensoap\etc\server.conf.
     Also, check that the service execution module path has been properly defined in the SSML file.
     For example,
        <StdIO>
            <exec prog='/opensoap/user/bin/HelloService'/>
        </StdIO>

  (4) Have the service execution modules been properly constructed ?
     In particular, any services that are to be called over CGI will not function
     properly when called by the OpenSOAP server.

  (5) Is the Apache HTTP Server running ?
     If not, start the Apache HTTP server.

