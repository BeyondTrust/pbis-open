SAMPLES\SECURITY\SSPI\GSS\README.TXT

This application is a port of the MIT Kerberos 5 release 1.3 GSS-API Sample
to the Microsoft Windows Kerberos SSP.  This port provides a demonstration
of how to use the various modes of the Kerberos SSP to interoperate with 
implementations of the GSS-API Kerberos 5 protocol as specified in RFC 1964
"GSS-API Kerberos V5 Mechanism".
The original sources can be found at src/appl/gss-sample in the MIT Kerberos
5 distribution.  Sources to a Windows GSS-API GUI client can be found within
the same distribution at src/windows/gss.  (See http://web.mit.edu/kerberos)

Various interoperability test scenarios are possible depending on whether a
Windows 2000/2003 Server running Active Directory or a MIT KDC is used to
store the client (or user) and service principal names; which platform is
used for hosting the sample client; which platform is used for the sample 
service; which protocol options are selected; and whether or not cross-realm
authentication is involved. 

These examples will execute on all versions of Microsoft Windows supporting
the Kerberos SSP.  As of this writing these include Windows 2000 Workstation,
Windows 2000 Server, Windows XP Home, Windows XP Professional, and Windows 
2003 Server.  In this document the term "Windows" is used to mean any of the 
before mentioned operating systems.

See the Kerberos Interoperability Papers:

  http://www.microsoft.com/technet/prodtechnol/windows2000serv/maintain/featusability/kerbinop.asp
  http://www.microsoft.com/technet/prodtechnol/windows2000serv/howto/kerbstep.asp

for general information on configuring Windows for interoperability with 
MIT Kerberos KDCs, clients, and services.  All of the following examples
utilize the Windows Active Directory as the KDC and utilize execute either
one of the client, the server or both client and server on the Windows 
platform.

MIT GSS sample client (Unix or KfW) to SSP sample server (Windows):
 * Create service account in the Windows Server Active Directory
   . Use the Active Directory Users and Computers Administrative Tool
     to create a new user account.
   . Assign a user logon name to the account without spaces
   . Assign a password to the account
   . Select the following Account options:
     - "Use DES encryption types for the account"
     - "Account is trusted for delegation"
     - "Account is sensitive and cannot be delegated"
     - "User cannot change password"
     - "Password never expires"
 * Assign a Service Principal Name (SPN) to the account utilizing the SETSPN.EXE
   program installed from the Operating System's Support Tools folder on 
   the original CD.  An SPN is usually of the form service-name/hostname but
   may be of the form service-name/hostname/domain depending on the 
   deployment.  There can be multiple SPNs assigned to a single account.  
   For this example specify a <service-name> and a <hostname> where the 
   <service-name> is arbitrary and the <hostname> is the fully qualified 
   domain name of the machine on which the SSP sample server will be executed.
   . SETSPN -A <SPN> <account-name>
 * Start gssserver on Windows specifying the selected options of your choice:
   . gssserver.exe [options] <service-name> <password> <REALM>
 * On the machine to be used to execute the MIT gss-client:
   . Using "kinit" obtain a Ticket Getting Ticket (TGT) for a client principal
     capable of obtaining a service ticket for the service principal 
     <service-name>/<hostname>@<REALM> 
   . Execute the MIT gss-client specifying the selected options of your 
     choice:
     - gss-client [options] <hostname> <service-name> <test-message>
 * After successful delivery of the test-message, executing "klist" on the 
   client machine will display:
   . the client principal's TGT "krbtgt/REALM@REALM"
   . the service ticket for the gssserver service principal 
     "service-name/hostname@REALM"

SSP sample client (Windows) to MIT GSS sample server (Unix):
 * Create service account in the Windows Server Active Directory
   . Use the Active Directory Users and Computers Administrative Tool
     to create a new user account.
   . Assign a user logon name to the account without spaces
   . Assign a password to the account
   . Select the following Account options:
     - "Use DES encryption types for the account"
     - "Account is trusted for delegation"
     - "Account is sensitive and cannot be delegated"
     - "User cannot change password"
     - "Password never expires"
 * Assign a Service Principal Name (SPN) to the account utilizing the SETSPN.EXE
   program installed from the Operating System's Support Tools folder on 
   the original CD.  An SPN is usually of the form service-name/hostname but
   may be of the form service-name/hostname/domain depending on the 
   deployment.  There can be multiple SPNs assigned to a single account.  
   For this example specify a <service-name> and a <hostname> where the 
   <service-name> is arbitrary and the <hostname> is the fully qualified 
   domain name of the machine on which the SSP sample server will be executed.
   . SETSPN -A <SPN> <account-name>
 * Generate a MIT Kerberos keytab file for the gss-server application with the
   "ktpass.exe" command:
      ktpass -out <filename> -princ <service-principal> -pass <password>
             -crypto DES-CBC-CRC -ptype KRB5_NT_PRINCIPAL -kvno <kvno>
      where <filename> is an output filename and <service-principal> is of
      the form: <service>/<hostname>@<REALM>
   If you are using Windows 2000 Server, <kvno> is 1.  If you are using
   Windows 2003 Server you must determine the <kvno> value by using the 
   kvno.exe utility to obtain the <kvno> from the service ticket:
        kvno.exe  <service-principal>@REALM
 * Securely move keytab file, <filename>, to the Unix host
 * Migrate the new key entry from <filename> to "/etc/krb5.keytab" with the
   "ktutil" tool:
      % ktutil
      ktutil: rkt <filename>
      ktutil: wkt /etc/krb5.keytab
      ktutil: q
 * Ensure the Unix Kerberos 5 configuration file, "/etc/krb5.conf", contains
   the realm information for the Windows Domain.  The Domain Controller should
   be specified as the "kdc" for the realm <REALM>.
 * Start MIT Kerberos gss-server on the Unix host specifying the selected 
   options of your choice:
   . gss-server [options] <service-name>
 * Start gssclient on Windows specifying the selected options of your choice:
   . gssclient.exe [options] <hostname> <service-name> <test-message>
 * After successful delivery of the test-message, executing "klist.exe" (from
   the Resource Kit) on the Windows machine will display:
   . the client principal's TGT "krbtgt/REALM@REALM"
   . the service ticket for the gssserver service principal 
     "service-name/hostname@REALM"

SSP sample client (Windows) to MIT GSS sample server (Windows):
 * Create service account in the Windows Server Active Directory
   . Use the Active Directory Users and Computers Administrative Tool
     to create a new user account.
   . Assign a user logon name to the account without spaces
   . Assign a password to the account
   . Select the following Account options:
     - "Use DES encryption types for the account"
     - "Account is trusted for delegation"
     - "Account is sensitive and cannot be delegated"
     - "User cannot change password"
     - "Password never expires"
 * Assign a Service Principal Name (SPN) to the account utilizing the SETSPN.EXE
   program installed from the Operating System's Support Tools folder on 
   the original CD.  An SPN is usually of the form service-name/hostname but
   may be of the form service-name/hostname/domain depending on the 
   deployment.  There can be multiple SPNs assigned to a single account.  
   For this example specify a <service-name> and a <hostname> where the 
   <service-name> is arbitrary and the <hostname> is the fully qualified 
   domain name of the machine on which the SSP sample server will be executed.
   . SETSPN -A <SPN> <account-name>
 * Generate a MIT Kerberos keytab file for the gss-server application with the
   "ktpass.exe" command:
      ktpass -out <filename> -princ <service-principal> -pass <password>
             -crypto DES-CBC-CRC -ptype KRB5_NT_PRINCIPAL -kvno <kvno>
      where <filename> is an output filename and <service-principal> is of
      the form: <service>/<hostname>@<REALM>
   If you are using Windows 2000 Server, <kvno> is 1.  If you are using
   Windows 2003 Server you must determine the <kvno> value by using the 
   kvno.exe utility to obtain the <kvno> from the service ticket:
        kvno.exe  <service-principal>@<REALM>
 * Ensure the Windows Kerberos 5 configuration file, "%WINDIR%\krb5.ini", contains
   the realm information for the Windows Domain.  The Domain Controller should
   be specified as the "kdc" for the realm <REALM>.  The default realm should
   be specified as <REALM>
 * Set the environment variable KRB5_KTNAME to point to the keytab file
        SET KRB5_KTNAME=FILE:<filename>
 * Start MIT Kerberos gss-server on the Windows host specifying the selected 
   options of your choice:
   . gss-server.exe [options] <service-name>
 * Start gssclient on Windows specifying the selected options of your choice:
   . gssclient.exe [options] <hostname> <service-name> <test-message>
 * After successful delivery of the test-message, executing "klist.exe" (from
   the Resource Kit) on the Windows machine will display:
   . the client principal's TGT "krbtgt/REALM@REALM"
   . the service ticket for the gssserver service principal 
     "service-name/hostname@REALM"

SSP sample client (Windows) to SSP sample server (Windows):
 * Create service account in the Windows Server Active Directory
   . Use the Active Directory Users and Computers Administrative Tool
     to create a new user account.
   . Assign a user logon name to the account without spaces
   . Assign a password to the account
   . Select the following Account options:
     - "Use DES encryption types for the account"
     - "Account is trusted for delegation"
     - "Account is sensitive and cannot be delegated"
     - "User cannot change password"
     - "Password never expires"
 * Assign a Service Principal Name (SPN) to the account utilizing the SETSPN.EXE
   program installed from the Operating System's Support Tools folder on 
   the original CD.  An SPN is usually of the form service-name/hostname but
   may be of the form service-name/hostname/domain depending on the 
   deployment.  There can be multiple SPNs assigned to a single account.  
   For this example specify a <service-name> and a <hostname> where the 
   <service-name> is arbitrary and the <hostname> is the fully qualified 
   domain name of the machine on which the SSP sample server will be executed.
   . SETSPN -A <SPN> <account-name>
 * Start gssserver on Windows specifying the selected options of your choice:
   . gssserver.exe [options] <service-name> <password> <REALM>
 * Start gssclient on Windows specifying the selected options of your choice:
   . gssclient.exe [options] <hostname> <service-name> <test-message>
 * After successful delivery of the test-message, executing "klist.exe" (from
   the Resource Kit) on the Windows machine will display:
   . the client principal's TGT "krbtgt/REALM@REALM"
   . the service ticket for the gssserver service principal 
     "service-name/hostname@REALM"



The Sample Application Algorithm:

Each time the client is invoked, it performs one or more exchanges
with the server.  Each exchange with the server consists primarily of
the following steps:

        1. A TCP/IP connection is established.

        2. (optional, on by default) The client and server establish a
           GSS-API context, and the server prints the identity of the
           client.

      / 3. The client sends a message to the server.  The message may
     /     be plaintext, cryptographically "signed" but not encrypted,
     |     or encrypted (default).
     |
0 or |  4. The server decrypts the message (if necessary), verifies
more |     its signature (if there is one) and prints it.
times|
     |  5. The server sends either a signature block (the default) or an
     |     empty token back to the client to acknowledge the message.
     \
      \ 6. If the server sent a signature block, the client verifies
           it and prints a message indicating that it was verified.

        7. The client sends an empty block to the server to tell it
           that the exchange is finished.

        8. The client and server close the TCP/IP connection and
           destroy the GSS-API context.

The SSP server's command line usage is:

Usage: gssserver.exe [-port port] [-verbose] [-once] [-logfile file]
       [-confidentiality] [-delegate] [-integrity] [-use_session_key]
       [-replay_detect] [-sequence_detect]
       service_name service_password service_realm

where service_name is the name of the account associated with the SPN
to be used by the client.  The service_password is the password associated
with the aforementioned account.  The service_realm is the Windows 
Domain name.  The command-line options have the following meanings:

-port   The TCP port on which to accept connections.  Default is 4444.

-verbose 
        Print extended messages regarding the state of the application.

-once   Tells the server to exit after a single exchange, rather than
        persisting.

-logfile
        The file to which the server should append its output, rather
        than sending it to stdout.

-confidentiality 
        Set the ASC_REQ_CONFIDENTIALITY flag in the call to 
        AcceptSecurityContext()

-delegate
        Set the ASC_REQ_DELEGATE flag in the call to 
        AcceptSecurityContext()

-integrity
        Set the ASC_REQ_INTEGRITY flag in the call to 
        AcceptSecurityContext()

-use_session_key
        Set the ASC_REQ_USE_SESSION_KEY flag in the call to 
        AcceptSecurityContext()

-replay_detect
        Set the ASC_REQ_REPLAY_DETECT flag in the call to 
        AcceptSecurityContext()  

-sequence_detect
        Set the ASC_REQ_SEQUENCE_DETECT flag in the call to 
        AcceptSecurityContext()


The SSP client's command line usage is:

Usage: gssclient.exe [-port port]
       [-confidentiality] [-delegate] [-integrity] [-use_session_key]
       [-replay_detect] [-sequence_detect] [-mutual_auth]
       [-noreplay] [-nomutual]
       [-f] [-q] [-ccount count] [-mcount count]
       [-na] [-nw] [-nx] [-nm] host service_name msg

where host is the host running the server, service_name is the service
name that the server will establish connections as and msg is the message.  
The command-line options have the following meanings:

-port   The TCP port to which to connect.  Default is 4444.

-mech   The OID of the GSS-API mechanism to use.

-confidentiality 
        Set the ISC_REQ_CONFIDENTIALITY flag in the call to 
        InitializeSecurityContext()

-delegate
        Set the ISC_REQ_DELEGATE flag in the call to 
        InitializeSecurityContext().  This tells the client to 
        delegate credentials to the server.  This means that a forwardable
        TGT will be sent to the server, which will put it in its
        credential cache.  This will not work if your TGT is not marked
        by the KDC.

-integrity
        Set the ISC_REQ_INTEGRITY flag in the call to 
        InitializeSecurityContext().

-use_session_key
        Set the ISC_REQ_USE_SESSION_KEY flag in the call to 
        InitializeSecurityContext().

-replay_detect
        Set the ISC_REQ_REPLAY_DETECT flag in the call to 
        InitializeSecurityContext().  This flag is set by default.  

-sequence_detect
        Set the ISC_REQ_SEQUENCE_DETECT flag in the call to 
        InitializeSecurityContext().

-mutual_auth
        Set the ISC_REQ_MUTUAL_AUTH flag in the call to 
        InitializeSecurityContext().  This flag is set by default.

-noreplay 
        Tells the client not to set the ISC_REQ_REPLAY_DETECT flag in
        the call to InitializeSecurityContext().

-nomutual
        Tells the client not to set the ISC_REQ_MUTUAL_AUTH flag in
        the call to InitializeSecurityContext().

-f      Tells the client that the "msg" argument is actually the name
        of a file whose contents should be used as the message.

-q      Tells the client to be quiet, i.e., to only print error
        messages.

-ccount Specifies how many sessions the client should initiate with
        the server (the "connection count").

-mcount Specifies how many times the message should be sent to the
        server in each session (the "message count").

-na     Tells the client not to do any authentication with the
        server.  Implies "-nw", "-nx" and "-nm".

-nw     Tells the client not to "wrap" messages.  Implies "-nx".

-nx     Tells the client not to encrypt messages.

-nm     Tells the client not to ask the server to send back a
        cryptographic checksum ("MIC").


The original version 1 of this application was originally written by 
Barry Jaspan of OpenVision Technologies, Inc. Version 2 of the application
was implemented by Jonathan Kamens of OpenVision Technologies, Inc.
Additional functionality was added to Version 2 by Jeffrey Altman of 
the Massachusetts Institute of Technology.

The port of version 1 of this application to the Kerberos SSP was 
performed by John Brezak of Microsoft Corporation.  Version 2 of this 
application was ported to the Kerberos SSP by Jeffrey Altman of the 
Massachusetts Institute of Technology.

/*
 * Copyright (C) 2004 by the Massachusetts Institute of Technology.
 * All rights reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 */

/*
 * Copyright 1993 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
