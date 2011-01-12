# -----------------------------------------------------------------------------
#  $RCSfile: Makefile.am,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------

EXTRA_DIST = \
	CORBADllCalc.mak \
	CORBADllCalcClient.c \
	CORBADllCalcService.cpp \
	ReadMeEUC.txt \
	calc.idl

## noinst_PROGRAMS = \
## 	CORBADllCalcClient \
## 	CORBADllCalcService \
## 	CORBADllCalcService.cgi
## 
## EXTRA_DIST = CORBADllCalc.mak ReadMeEUC.txt
## 
## cgi_bin_PROGRAMFILES = CORBADllCalcService.cgi
## 
## CORBADllCalcClient_SOURCES = CORBADllCalcClient.c
## CORBADllCalcClient_LDADD = \
## 	$(top_builddir)/src/api/libOpenSOAPClient.la
## 
## CORBADllCalcService_SOURCES = CORBADllCalcService.cpp
## CORBADllCalcService_LDADD = \
## 	calcSK.o \
## 	$(top_builddir)/src/api/libOpenSOAPService.la \
## 	$(top_builddir)/src/api/libOpenSOAPClient.la
## 
## CORBADllCalcService_cgi_SOURCES =
## CORBADllCalcService_cgi_LDADD = \
## 	calcSK.o \
## 	CORBADllCalcService.cgi.o \
## 	$(top_builddir)/src/api/libOpenSOAPService.la \
## 	$(top_builddir)/src/api/libOpenSOAPClient.la
## 
## CORBADllCalcService.o: CORBADllCalcService.cpp calc.hh
## CORBADllCalcService.cgi.o: CORBADllCalcService.cpp calc.hh
## 	$(COMPILE) -DCONNECT_TYPE_CGI -c -o $@ $^
## 
## calc.hh: calcSK.cc
## calcSK.cc: calc.idl
## 	omniidl -bcxx calc.idl
