Certification Authority Public Key Data Registration Sample
===========================================================

This directory contains the source code for the certification authority public
key data registration sample program.

1. Function Outline

  This program takes as input a text file enumerating the registration
details, and adds the public key data to the database.

  Before using this program, it is necessary to set the environment
variable that shows the certification authority database file name.
(Environment variable name : OPENSOAP_CA_DATABASE)
  This environment variable should be set to full path file name of the
database being used.
  Also, for this file name, for administration purposes another file is
automatically generated. This file name is has the same name as the
database file with "_sno" appended to it.

Ex)
  Certification Authority Database file name : /home/CA/CA.db
  On creation of this database file, the file /home/CA/CA.db_sno
  is simultaneously created.

2. Usage

  This program is executed with a single parameter as follows:

 registCA "content file name"

  The content file contains a listing of entries in the format illustrated
below.

 [Format for a single entry]
(Line 1)           Public Key Owner's Name
(Line 2)           Character string indicating Key's effective date limit
                   This string is composed as follows:
                   Year : 4 characters; Month, Day, Hour, Minute, Second : 2 characters each
                   (Ex. "20020211120000")
(Line 3 and after) Public Key(For use by OpenSOAP)
  It is possible to have more than one entry in this file.
  Please refer to the sample file (regist.txt) included showing the content layout.


