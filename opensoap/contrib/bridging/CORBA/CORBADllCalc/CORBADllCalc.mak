# -----------------------------------------------------------------------------
#  $RCSfile: CORBADllCalc.mak,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
#
#
#

PROGRAMS = CORBADllCalcClient CORBADllCalcService CORBADllCalcService.cgi

IDLCXX = omniidl -bcxx
OPENSOAP_PREFIX=/usr/local
OPENSOAP_INCLUDE=$(OPENSOAP_PREFIX)/include
OPENSOAP_LIB=$(OPENSOAP_PREFIX)/lib

OMNILIBS=-lomniORB3 -ltcpwrapGK -lomnithread -lpthread

XML2_LIBS=`xml2-config --libs`

CFLAGS=-I$(OPENSOAP_INCLUDE)
CXXFLAGS=-D__x86__ -D__linux__ -D__OSVERSION__=2 -I/usr/local/include
CGI_CFLAGS=$(CFLAGS) $(CXXFLAGS) -DCONNECT_TYPE=\"cgi\"
COMMON_LDFLAGS=-L$(OPENSOAP_LIB) -Wl,-rpath -Wl,$(OPENSOAP_LIB)
#COMMON_LDFLAGS=-L$(OPENSOAP_LIB) -R$(OPENSOAP_LIB)
CLIENT_LDFLAGS=$(COMMON_LDFLAGS) -lOpenSOAPClient
SERVICE_LDFLAGS=$(CLIENT_LDFLAGS) -lOpenSOAPService

CSOURCES= CORBADllCalcClient.c
CXXSOURCES= CORBADllCalcService.cpp
IDL_SOURCES = calc.idl
IDL_OUTPUT_HEADERS = calc.hh
IDL_OUTPUT_CXXSOURCES = calcSK.cc
IDL_OUTPUT_FILES = $(IDL_OUTPUT_HEADERS) $(IDL_OUTPUT_CXXSOURCES)
OBJS = $(CSOURCES:.c=.o) \
	$(CXXSOURCES:.cpp=.o) \
	CORBADllCalcService.cgi.o \
	$(IDL_OUTPUT_CXXSOURCES:.cc=.o)

all: $(PROGRAMS)

calcSK.cc: calc.hh

calc.hh: calc.idl
	$(IDLCXX) $<

CORBADllCalcService.o: CORBADllCalcService.cpp calc.hh

CORBADllCalcService.cgi.o: CORBADllCalcService.cpp calc.hh
	$(CXX) $(CGI_CFLAGS) -c -o $@ $<

CORBADllCalcClient: CORBADllCalcClient.o
	$(CC)  -o $@ $^ $(CLIENT_LDFLAGS) $(XML2_LIBS)

CORBADllCalcService: CORBADllCalcService.o calcSK.o
	$(CXX)  -o $@ $^ $(SERVICE_LDFLAGS) $(XML2_LIBS) $(OMNILIBS)

CORBADllCalcService.cgi: CORBADllCalcService.cgi.o calcSK.o
	$(CXX)  -o $@ $^ $(SERVICE_LDFLAGS) $(XML2_LIBS) $(OMNILIBS)

clean: 
	rm -f $(OBJS) $(PROGRAMS) $(IDL_OUTPUT_FILES)
