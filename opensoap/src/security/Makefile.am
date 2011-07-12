# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
# 

INCLUDES = -I${top_srcdir} -I${top_srcdir}/include @SSL_INCLUDES@
#INCLUDES = -I${top_srcdir} -I${top_srcdir}/include @XML2_CFLAGS@ @MBFUNCS_CFLAGS@

lib_LTLIBRARIES = \
	libOpenSOAPSecurity.la

libOpenSOAPSecurity_la_SOURCES = \
	sec_genrsakey.c \
	sec_rsasig.c \
	sec_encrypt.c \
	sec_dconv.c \
	sec_hash.c \
	sec_keyfile.c \
	sec_encryptEnvelope.c \
	sec_signEnvelope.c \
	CA_database.c \
	cert_fio.c \
	security_defs.h

libOpenSOAPSecurity_la_LIBADD = \
	@SSL_LIBS@

libOpenSOAPSecurity_la_LDFLAGS = \
	-version-info @OPENSOAP_VERSION_INFO@

MKDEFFILE = ${top_builddir}/${MKDEFFILE_SH}

OpenSOAPSecurity.def: $(libOpenSOAPSecurity_la_SOURCES)
	${MKDEFFILE} $@ $^

EXTRA_DIST = \
	Security_DLL.c

W32_DEF_FILES = \
	OpenSOAPSecurity.def

dist-hook:
	rm -f $(W32_DEF_FILES)
	$(MAKE) $(W32_DEF_FILES)
	for file in $(W32_DEF_FILES); do \
	  cp $$file $(distdir)/$$file; \
	done
