#!/bin/sh -p

IMAPSERVER=cyrus.andrew.cmu.edu

LD_LIBRARY_PATH=/usr/local/lib:/usr/openwin/lib:/usr/lib
export LD_LIBRARY_PATH
java  -cp .:/usr/java/jre/lib/rt.jar:/usr/local/lib/java/classes/sasl jimtest ${IMAPSERVER}



