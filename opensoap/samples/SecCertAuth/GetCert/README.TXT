Digital Certificate Acquistion Sample
=====================================

This directory contains the source code for the sample
digital certificate acquisition client and service programs.

After installation of the OpenSOAP API, use
  make -f GetCert.mak
to compile and generate the following programs.

* GetCertClient
  Digital Certificate Acquistion Client Program
* GetCertService
  Standard I/O version of the Digital Certificate Acquistion Service Program. By using
  inetd, this service can be made into a socket type service.
* GetCertService.cgi
  CGI version of the digital certificate acquistion service program.
* CA_Refer
  This program performs the actual access of the certification authority database
  for the digital certificate. This program is called from the GetCertService
  and GetCostService.cgi programs.

* Usage, settings

* GetCertClient(Digital Certificate Acquisition Client Program)
  By passing the name of a key owner to the program, a digital certificate
  and the corresponding public key can be retrieved. These values are
  saved in the generated files "result.cert" and "pubKey_res.pem" respectively.
  If it is desired to change the names of these files, change the definitions
  of RESULT_CERT_FILE_NAME and RESULT_PUBKEY_FILE_NAME in the source file
  GetCertClient.c.
  Also, the public key of a certification authority is required in order
  to verify the signature of the digital certificate. This key file is
  defined in GetCertClient.c as CA_PUBKEY_FILE and can be modified as
  needed. (Currently this is defined to be the same as when the OpenSOAP
  Server is initially installed)

* GetCertService(Digital Certificate Acquisition Service Program)
  REFER_PROGRAM_DIR in GetCertService.c defines the actual directory in
  which the search program (CA_Refer) is installed. If CA_Refer is
  installed in another directory, this definition should be changed.

* CA_refer(Certification Authority Database search program)
  To change the default settings, modify the the following definitions in
  the file DBRefer.c.

  CA_DATABASE_FILE : Certification Authority DB file location full path
  CA_PRIVKEY_FILE  : Certification Authority Private Key file name

Furthermore, a sample certification authority database(CA.db) is included
in this directory.
This database contains the public keys corresponding to the following
owners(3 items).

"OpenSOAP-SAMPLE-CA" (Indicates local Certification Authority)
"David Beckham"
"Alessandro Del Piero"

This is the same as that which was created for the "Regist" sample.

