# -----------------------------------------------------------------------------
#  $RCSfile: CORBAExeCalc.mak,v $
# 
#  See Copyright for the status of this software.
# 
#  The OpenSOAP Project
#  http://opensoap.jp/
# -----------------------------------------------------------------------------
#
#
#

PROGRAMS = CORBAExeCalcClient CORBAExeCalcService CORBAExeCalcService.cgi

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

CSOURCES= CORBAExeCalcClient.c
CXXSOURCES= CORBAExeCalcService.cpp
IDL_SOURCES = calc.idl
IDL_OUTPUT_HEADERS = calc.hh
IDL_OUTPUT_CXXSOURCES = calcSK.cc
IDL_OUTPUT_FILES = $(IDL_OUTPUT_HEADERS) $(IDL_OUTPUT_CXXSOURCES)
OBJS = $(CSOURCES:.c=.o) \
	$(CXXSOURCES:.cpp=.o) \
	CORBAExeCalcService.cgi.o \
	$(IDL_OUTPUT_CXXSOURCES:.cc=.o)

all: $(PROGRAMS)

calcSK.cc: calc.hh

calc.hh: calc.idl
	$(IDLCXX) $<

CORBAExeCalcService.o: CORBAExeCalcService.cpp calc.hh
CORBAExeCalcClient.o: CORBAExeCalcClient.c calc.hh

CORBAExeCalcService.cgi.o: CORBAExeCalcService.cpp
	$(CXX) $(CGI_CFLAGS) -c -o $@ $<

CORBAExeCalcClient: CORBAExeCalcClient.o
	$(CC)  -o $@ $^ $(CLIENT_LDFLAGS) $(XML2_LIBS)

CORBAExeCalcService: CORBAExeCalcService.o calcSK.o
	$(CXX)  -o $@ $^ $(SERVICE_LDFLAGS) $(XML2_LIBS) $(OMNILIBS)

CORBAExeCalcService.cgi: CORBAExeCalcService.cgi.o calcSK.o
	$(CXX)  -o $@ $^ $(SERVICE_LDFLAGS) $(XML2_LIBS) $(OMNILIBS)

clean: 
	rm -f $(OBJS) $(PROGRAMS) $(IDL_OUTPUT_FILES)
