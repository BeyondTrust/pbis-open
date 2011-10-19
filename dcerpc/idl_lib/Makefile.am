## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk

AM_CFLAGS+=-DMIA -D_POSIX_C_SOURCE -I$(top_srcdir)/ncklib/ -I$(srcdir) -Wall -Werror -DDCETHREAD_ENFORCE_API

noinst_LTLIBRARIES=libidl.la

libidl_la_SOURCES = alfrsupp.c allocate.c autohndl.c bindcall.c ctxeecli.c \
	ctxeectx.c ctxerrtl.c cvt_glob.c eebool.c eebyte.c eechar.c eedouble.c \
	eeenum.c eefloat.c eehyper.c eelong.c eenodtbl.c eeshort.c eesmall.c \
	eeuhyper.c eeulong.c eeushort.c eeusmall.c erbool.c erbyte.c erchar.c \
	erdouble.c erenum.c erfloat.c erhyper.c erlong.c ernodtbl.c ershort.c \
	ersmall.c eruhyper.c erulong.c erushort.c erusmall.c interpsh.c marbfman.c \
	nbaseool.c ndrcharp.c ndrchars.c ndrfloat.c ndrmi.c ndrmi2.c ndrmi3.c \
	ndrmi5.c ndrui.c ndrui2.c ndrui3.c ndrui5.c nidlalfr.c pickling.c pipesupp.c \
	sscmaset.c sscmasrv.c

noinst_HEADERS= ctxeertl.h cvt.h cvt_pvt.h lapollo.h lsysdep.h ndrmi.h \
	ndrui.h upkcray.c.h upkibml.c.h upkibms.c.h upkieees.c.h upkieeet.c.h upkvaxf.c.h \
	upkvaxg.c.h pkcray.c.h pkibml.c.h pkibms.c.h pkieees.c.h pkieeet.c.h pkvaxf.c.h \
	pkvaxg.c.h round.c.h
