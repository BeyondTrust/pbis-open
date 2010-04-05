librpc_srcdir = $(top_srcdir)
idl_srcdir = $(top_srcdir)/Idl

noinst_LTLIBRARIES = libmsrpc.la

libmsrpc_la_SOURCES =      \
	UnicodeString.c    \
	UnicodeStringEx.c  \
	md4.c              \
	md5.c              \
	hmac_md5.c         \
	rc4.c              \
	des.c              \
	smbcrypto.c        \
	random.c           \
	MemPtr.c           \
	errcheck.c         \
	LMcrypt.c          \
	sidhelper.c

noinst_PROGRAMS = md4test md5test cryptotest randomtest destest

md4test_SOURCES = md4test.c
md4test_CFLAGS =               \
    $(AM_CFLAGS)               \
    -I$(librpc_srcdir)/include

md4test_LDADD = libmsrpc.la

md5test_SOURCES = md5test.c
md5test_CFLAGS =               \
    $(AM_CFLAGS)               \
    -I$(librpc_srcdir)/include

md5test_LDADD = libmsrpc.la

cryptotest_SOURCES = cryptotest.c
cryptotest_CFLAGS =            \
    $(AM_CFLAGS)               \
    -I$(librpc_srcdir)/include

cryptotest_LDADD = libmsrpc.la

randomtest_SOURCES = randomtest.c
randomtest_CFLAGS =            \
    $(AM_CFLAGS)               \
    -I$(librpc_srcdir)/include

randomtest_LDADD = libmsrpc.la

destest_SOURCES = destest.c
destest_CFLAGS =               \
    $(AM_CFLAGS)               \
    -I$(librpc_srcdir)/include

destest_LDADD = libmsrpc.la

libmsrpc_la_CPPFLAGS =         \
    $(AM_CPPFLAGS)             \
    $(LWBASE_INCLUDES)         \
    $(DCERPC_INCLUDES)         \
    $(LIBUNISTR_INCLUDES)      \
    -I$(librpc_srcdir)/include

libmsrpc_la_CFLAGS =   \
    $(AM_CFLAGS)       \
    -DLIBRPC_BUILD

libmsrpc_la_LDFLAGS =    \
    $(AM_LDFLAGS)        \
    $(LWBASE_LDFLAGS)    \
    $(DCERPC_LDFLAGS)    \
    $(LIBUNISTR_LDFLAGS)

libmsrpc_la_LIBADD =  \
    $(LWBASE_LIBS)    \
    $(DCERPC_LIBS)    \
    $(LIBUNISTR_LIBS)
