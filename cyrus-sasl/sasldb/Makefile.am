# Makefile.am for the SASLdb library
# Rob Siemborski
# $Id: Makefile.am,v 1.31.2.1 2009/04/27 17:58:26 murch Exp $
# Copyright (c) 2000 Carnegie Mellon University.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. The name "Carnegie Mellon University" must not be used to
#    endorse or promote products derived from this software without
#    prior written permission. For permission or any other legal
#    details, please contact  
#      Office of Technology Transfer
#      Carnegie Mellon University
#      5000 Forbes Avenue
#      Pittsburgh, PA  15213-3890
#      (412) 268-4387, fax: (412) 268-7395
#      tech-transfer@andrew.cmu.edu
#
# 4. Redistributions of any form whatsoever must retain the following
#    acknowledgment:
#    "This product includes software developed by Computing Services
#     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
#
# CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
# FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
# AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
# OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

# Library version info - here at the top, for sanity
# Note that this doesn't necessaraly follow the libsasl2 verison info
sasl_version = 1:23:0

INCLUDES=-I$(top_srcdir)/include -I$(top_builddir)/include @SASL_DB_INC@

extra_common_sources = db_none.c db_ndbm.c db_gdbm.c db_berkeley.c

EXTRA_DIST = NTMakefile

noinst_LTLIBRARIES = libsasldb.la
# noinst_LIBRARIES = libsasldb.a

libsasldb_la_SOURCES = allockey.c sasldb.h
EXTRA_libsasldb_la_SOURCES = $(extra_common_sources)
libsasldb_la_DEPENDENCIES = $(SASL_DB_BACKEND)
libsasldb_la_LIBADD = $(SASL_DB_BACKEND) 

# Prevent make dist stupidity
# libsasldb_a_SOURCES =
# EXTRA_libsasldb_a_SOURCES =

# libsasldb.a: libsasldb.la $(SASL_DB_BACKEND_STATIC)
# 	$(AR) cru .libs/$@ $(SASL_DB_BACKEND_STATIC)


