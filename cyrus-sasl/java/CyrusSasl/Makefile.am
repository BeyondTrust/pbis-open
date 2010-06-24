# Makefile.am for the Java SASL library
# Rob Earhart
#
################################################################
#        Copyright 1998 by Carnegie Mellon University
#
#                      All Rights Reserved
#
#Permission to use, copy, modify, and distribute this software and its
#documentation for any purpose and without fee is hereby granted,
#provided that the above copyright notice appear in all copies and that
#both that copyright notice and this permission notice appear in
#supporting documentation, and that the name of Carnegie Mellon University
#not be used in advertising or publicity pertaining to distribution of the
#software without specific, written prior permission.
#
#CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
#SOFTWARE, INCLUDING #ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, 
#IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE FOR ANY SPECIAL, 
#INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
#LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
#OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#PERFORMANCE OF THIS SOFTWARE.
################################################################

javasasl_version = 1:0:0

javasasldir = $(prefix)/lib/java/classes/sasl/CyrusSasl
javahtmldir = $(prefix)/html/sasl

INCLUDES=-I$(top_srcdir)/include $(JAVA_INCLUDES)

javasasl_JAVA = Sasl.java GenericClient.java \
		ClientFactory.java \
	        GenericCommon.java SaslClient.java \
	        SaslClientFactory.java SaslException.java \
	        SaslInputStream.java SaslOutputStream.java\
		SaslUtils.java ServerFactory.java \
                SaslServerFactory.java SaslServer.java \
                GenericServer.java
EXTRA_DIST = $(javasasl_JAVA)
CLASSES = $(javasasl_JAVA:.java=.class)

lib_LTLIBRARIES = libjavasasl.la
libjavasasl_la_SOURCES = javasasl.h javasasl.c
libjavasasl_la_LDFLAGS = -export_dynamic -L../../lib/.libs -lsasl2 -version-info $(javasasl_version) $(wildcard ../lib/*.lo)
BUILT_SOURCES = javasasl.h $(CLASSES)

$(srcdir)/javasasl.c: javasasl.h

javasasl.h: $(CLASSES)
	$(CLASSPATH_ENV) $(JAVAH) -o $@ -jni $(patsubst %.class,CyrusSasl.%,$^)

# force build of class files
$(CLASSES): classjavasasl.stamp

#install-data-local:
# xxx broken
#	$(mkinstalldirs) $(javahtmldir)
#	$(CLASSPATH_ENV) $(JAVADOC) -d $(javahtmldir) sasl
#	-if test ! -h $(javahtmldir)/images; \
#	then \
#	  $(LN_S) $(JAVA_BASE)/docs/api/images $(javahtmldir)/images; \
#	fi
