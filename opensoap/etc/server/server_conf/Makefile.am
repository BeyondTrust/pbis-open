# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
##

SERVER_CONF_DIR = @sysconfdir@

SCHEMA_DIR = ${SERVER_CONF_DIR}

sysconf_DATA = ${SERVER_CONF_FILES}

#
SERVER_CONF_IN_FILES = server.conf.in
SERVER_CONF_FILES = ${SERVER_CONF_IN_FILES:.in=}

EXTRA_DIST	=	\
	${SERVER_CONF_IN_FILES} \
	${SERVER_CONF_FILES}

# ssml create rule
${SERVER_CONF_FILES}: ${SERVER_CONF_IN_FILES} Makefile
	@for i in ${SERVER_CONF_IN_FILES} ; do \
		s=`echo $$i | sed 's/\.in$$//'`; \
		sed -e "s%\@serverstatedir\@%@localstatedir@%g" \
			-e "s%\@sysconfdir\@%@sysconfdir@%g" > $$s < $$i; \
	done
