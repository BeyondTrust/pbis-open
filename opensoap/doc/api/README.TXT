=begin
= Introduction
 The file groups found underneath this directory include the setup files and
the source reference text files used by ((<doxygen|URL:http://www.doxygen.org/>))
to generate the dcoumentation for languages other than English.
Currently, besides English, only Japanese is supported.
The original Japanese language file are EUC-JP encoded, so be sure to set
your environment codeset to EUC-JP when carrying out this procedure.

= Building The Reference Manual
 Change directory to the appropriate language and run the command "make docs".
The documentation will be created in a sub-directory. The package Doxygen is
required to build the documentation. After Doxygen has been installed, execute
./configure in the top directory before moving to the appropriate language
directory.

=end
