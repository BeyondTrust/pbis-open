# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------

POD2MAN = @POD2MAN@

SERVER_FILE = opensoap-server-ctl.8

POD2MAN_FILES = soaping.1 ${SERVER_FILE}
POD2MAN_SOURCES = ${POD2MAN_FILES:=.pod}

SERVER_LINK = \
	idManager.8 \
	msgDrvCreator.8 \
	queueManager.8 \
	spoolManager.8 \
	srvConfAttrMgr.8 \
	ssmlAttrMgr.8 \
	ttlManager.8

MAN_FILES = ${POD2MAN_FILES} ${SERVER_LINK}

man_MANS = ${MAN_FILES}
mandir = @mandir@/ja

EXTRA_DIST = ${POD2MAN_SOURCES} ${MAN_FILES}

mans: ${POD2MAN_FILES}

${SERVER_LINK}: ${SERVER_FILE}
	@echo .so man8/${SERVER_FILE} > $@
#	${LN_S} $< $@

${POD2MAN_FILES}: ${POD2MAN_SOURCES}
	@if test "x${POD2MAN}" = "x"; then \
	  echo "Please install pod2man and execute ./configure at top directory"; \
	fi
	for f in $^ ; do \
	  f2=`echo $$f | sed -e 's/\.pod$$//'` ; \
	  f3=`echo $$f | sed -e 's/\..\.pod$$/.pod/'` ; \
	  cp -i $$f $$f3 ; \
	  f4=`echo $$f2 | sed -e 's/^.*\.//g'` ; \
	  ${POD2MAN} --center="OpenSOAP" --release="OpenSOAP @VERSION@" \
	  --section=$$f4 $$f3 $$f2 ; \
	  rm $$f3 ; \
	done

dist-hook: mans
