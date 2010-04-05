## Process this file with automake to produce Makefile.in
include $(top_srcdir)/include.mk
include $(top_srcdir)/ncklib/ncklib.mk

NCK_IDL_FLAGS=-keep c_source -no_cpp -v -no_mepv -I$(IDL_INCLUDE_DIR)/.. -I$(top_builddir)/include @DCETHREADINCLUDES@ $(TARGET_OS)

SUBDIRS=include

AM_CFLAGS += -Wall -Werror -fno-strict-aliasing

# Initialize
lib_LTLIBRARIES=
noinst_LTLIBRARIES=
noinst_HEADERS=
CLEANFILES=

######################################################################
## COMPONENT: include

noinst_HEADERS+= \
	cn.h			\
	cnasgsm.h		\
	cnassm.h		\
	cnassoc.h		\
	cnbind.h		\
	cncall.h		\
	cnclsm.h		\
	cncthd.h		\
	cnfbuf.h		\
	cnid.h			\
	cnmgmt.h		\
	cnnet.h			\
	cnp.h			\
	cnpkt.h			\
	cnrcvr.h		\
	cnsm.h			\
	cnxfer.h		\
	com.h			\
	comauth.h		\
	comcthd.h		\
	comfwd.h		\
	comimage.h		\
	cominit.h		\
	cominitp.h		\
	commonp.h		\
	comnaf.h		\
	comnetp.h		\
	comp.h			\
	comprot.h		\
	comsoc.h		\
	comsoc_bsd.h		\
	comsoc_smb.h            \
	comtwr.h		\
	comtwrflr.h		\
	comtwrref.h		\
	cs_s.h			\
	dg.h			\
	dgcall.h		\
	dgccall.h		\
	dgccallt.h		\
	dgcct.h			\
	dgclsn.h		\
	dgexec.h		\
	dgfwd.h			\
	dgglob.h		\
	dghnd.h			\
	dgpkt.h			\
	dgrq.h			\
	dgscall.h		\
	dgsct.h			\
	dgslive.h		\
	dgslsn.h		\
	dgsoc.h			\
	dgutl.h			\
	dgxq.h			\
	ipnaf.h			\
	mgmtp.h			\
	ndrglob.h		\
	ndrp.h			\
	noauth.h		\
	noauthcn.h		\
	noauthdg.h		\
	gssauth.h		\
	gssauthcn.h		\
	schnauth.h		\
	schnauthcn.h		\
	npnaf.h			\
	rpcclock.h		\
	rpcdbg.h		\
	rpcfork.h		\
	rpclist.h		\
	rpclog.h		\
	rpcmem.h		\
	rpcmutex.h		\
	rpcrand.h		\
	rpcsvc.h		\
	rpctimer.h		\
	sec_id_pickle.h		\
	uxdnaf.h


######################################################################
## COMPONENT: com - comprises of libcom and libstubs convenience libs

noinst_LTLIBRARIES+=libcom.la libstubs.la

libcom_la_SOURCES=comauth.c combind.c comcall.c comcthd.c comep.c comif.c \
 cominit.c cominit_ux.c comnaf.c comnet.c comnlsn.c comobj.c comp.c \
 comtwr.c comtwrflr.c comtwrref.c comutil.c nsldap.c sec_id.c
libcom_la_CPPFLAGS=$(AM_CPPFLAGS) -DIMAGE_DIR=\"$(libdir)\"
libcom_la_LIBADD=libstubs.la

# Do not use NCK
libstubs_la_SOURCES=pickle_cstub.c ep_cstub.c
libstubs_la_CPPFLAGS=-I. $(BASE_CPPFLAGS) $(IDL_CPPFLAGS)
libstubs_la_CFLAGS=$(BASE_CFLAGS)

noinst_HEADERS+=ep.h

sec_id.c: pickle.h

pickle.h pickle_cstub.c: pickle.idl
	$(IDL) -keep c_source -no_cpp -v -no_mepv -I$(top_srcdir)/include -server none $<

ep.h ep_cstub.c: $(IDL_INCLUDE_DIR)/ep.idl
	$(IDL) -keep c_source -no_cpp -v -no_mepv -I$(top_srcdir)/include -cepv -server none $<

# Unused: id_base.h (the copy here came from some IDL w/MS PAC format info?)

######################################################################
## COMPONENT: rpc - comprises of librpc convenience lib

noinst_LTLIBRARIES+=librpc.la

librpc_la_SOURCES=dce_error.c rpcclock.c rpcdbg.c rpclist.c \
 rpclog.c rpcmem.c rpcmutex.c rpcrand.c rpctimer.c

######################################################################
## COMPONENT: transport - comprises of libtrans convenience lib

noinst_LTLIBRARIES+=libtrans.la

libtrans_la_CPPFLAGS = $(AM_CPPFLAGS)
libtrans_la_SOURCES = comsoc.c comsoc_bsd.c comsoc_smb.c
libtrans_la_LIBADD = -llwioclient -llwbase

######################################################################
## COMPONENT: auth - comprises of libauth_none shared lib

if AUTH_DUMMY
lib_LTLIBRARIES+=libauth_none.la

libauth_none_la_SOURCES = noauth.c
# Unused: noauthcn.c
# Unused: noauthdg.c
endif

######################################################################
## COMPONENT: . - comprises of libcodeset convenience lib
##
## Note: This appears to be unused

if BUILD_CODESET
noinst_LTLIBRARIES+=libcodeset.la libcodeset_stubs.la

libcodeset_la_SOURCES=cs_s_eval.c cs_s_reg.c cs_s_stub.c
libcodeset_la_LIBADD=libcodeset_stubs.la

libcodeset_stubs_la_SOURCES=codesets_cstub.c
libcodeset_stubs_la_CPPFLAGS=-I. $(BASE_CPPFLAGS) $(IDL_CPPFLAGS)
libcodeset_stubs_la_CFLAGS=$(BASE_CFLAGS)

codesets.h codesets_cstub.c: $(IDL_INCLUDE_DIR)/codesets.idl
	$(IDL) $(NCK_IDL_FLAGS) -server none $<
else
# TODO-2008/01/24-dalmeida - This appears to be completely unused,
# so I commented it out:
#EXTRA_FILES=$(codeset_files)
endif

######################################################################
## COMPONENT: . - comprises of libnck and libmgmt_stubs convenience libs

noinst_LTLIBRARIES+=libnck.la libmgmt_stubs.la
noinst_HEADERS+=mgmt.h

libnck_la_SOURCES=comfork.c ndrglob.c mgmt.c
libnck_la_LIBADD=libcom.la librpc.la libtrans.la libmgmt_stubs.la

### Build w/o NCK flags
libmgmt_stubs_la_SOURCES=mgmt_cstub.c mgmt_sstub.c
libmgmt_stubs_la_CPPFLAGS=-I. $(BASE_CPPFLAGS) $(IDL_CPPFLAGS)
libmgmt_stubs_la_CFLAGS=$(BASE_CFLAGS)

mgmt_cstub.c mgmt_sstub.c mgmt.h: $(IDL_INCLUDE_DIR)/mgmt.idl
	$(IDL) -keep c_source -no_cpp -v -no_mepv -I$(top_srcdir)/include -cepv $<

# Unused: comfork_cma.c
# Unused: dgauth.c ("obsolete")
# Unused: rpcsvc.c

######################################################################
## Dist stuff

######################################################################
## COMPONENT: transport/cn - comprises of libprot_ncacn shared lib

# code for connection orientated transports
if PROT_NCACN

noinst_LTLIBRARIES+=libprot_ncacn.la
libnck_la_LIBADD+=libprot_ncacn.la

libprot_ncacn_la_LDFLAGS = $(DCERPC_LDFLAGS)
libprot_ncacn_la_SOURCES = cnassoc.c cnbind.c cncall.c cncasgsm.c \
 cncassm.c cncclsm.c cncthd.c cnfbuf.c cnid.c cninit.c cnmgmt.c \
 cnnet.c cnp.c cnpkt.c cnrcvr.c cnsasgsm.c cnsassm.c cnsclsm.c \
 cnsm.c cnxfer.c

endif

######################################################################
## COMPONENT: transport/dg - comprises of libprot_ncadg shared lib

if PROT_NCADG
noinst_LTLIBRARIES+=libprot_ncadg.la
noinst_LTLIBRARIES+=libprot_ncadg_stubs.la
libnck_la_LIBADD+=libprot_ncadg.la

libprot_ncadg_stubs_la_SOURCES=conv_cstub.c convc_cstub.c convc_sstub.c
libprot_ncadg_stubs_la_CPPFLAGS=-I. $(BASE_CPPFLAGS) $(IDL_CPPFLAGS)
libprot_ncadg_stubs_la_CFLAGS=$(BASE_CFLAGS)

noinst_HEADERS+=conv.h convc.h

conv.h conv_cstub.c: $(IDL_INCLUDE_DIR)/conv.idl
	$(IDL) $(NCK_IDL_FLAGS) -cepv -server none $<

convc.h convc_cstub.c convc_sstub.c: $(IDL_INCLUDE_DIR)/convc.idl
	$(IDL) $(NCK_IDL_FLAGS) -cepv $<

libprot_ncadg_la_SOURCES=conv.c dg.c dgcall.c dgccall.c \
 dgccallt.c dgcct.c dgclive.c dgclsn.c dgexec.c dgfwd.c \
 dgglob.c dghnd.c dginit.c dglossy.c dglsn.c dgpkt.c dgrq.c \
 dgscall.c dgsct.c dgslive.c dgslsn.c dgsoc.c dgutl.c dgxq.c 

libprot_ncadg_la_LIBADD=libprot_ncadg_stubs.la

endif

######################################################################
## COMPONENT: naf - comprises of libnaf_{np,ip,uxd} shared libs

if NAF_IP
noinst_LTLIBRARIES+=libnaf_ip.la
libnck_la_LIBADD+=libnaf_ip.la
libnaf_ip_la_SOURCES=ipnaf.c ipnaf_linux.c twr_ip.c
endif

if NAF_NP
noinst_LTLIBRARIES+=libnaf_np.la
libnck_la_LIBADD+=libnaf_np.la
libnaf_np_la_SOURCES=npnaf.c npnaf_linux.c twr_np.c twr_uxd.c
endif

if NAF_UXD
noinst_LTLIBRARIES+=libnaf_uxd.la
libnck_la_LIBADD+=libnaf_uxd.la
libnaf_uxd_la_SOURCES=uxdnaf.c uxdnaf_linux.c twr_uxd.c
endif

# Unused: twr_dnet.c

if AUTH_GSS_NEGOTIATE
noinst_LTLIBRARIES += libauth_gss_negotiate.la
libnck_la_LIBADD+=libauth_gss_negotiate.la

libauth_gss_negotiate_la_SOURCES = gssauth.c gssauthcn.c
libauth_gss_negotiate_la_CFLAGS = @GSSAPI_INCLUDES@
libauth_gss_negotiate_la_LDFLAGS = @GSSAPI_LDFLAGS@
libauth_gss_negotiate_la_LIBADD = @GSSAPI_LIBS@ -llwbase
endif

if AUTH_SCHANNEL
noinst_LTLIBRARIES += libauth_schannel.la
libnck_la_LIBADD+=libauth_schannel.la

libauth_schannel_la_SOURCES = schnauth.c schnauthcn.c
libauth_schannel_la_CFLAGS = @SCHANNEL_INCLUDES@
libauth_schannel_la_LDFLAGS = @SCHANNEL_LDFLAGS@
libauth_schannel_la_LIBADD = @SCHANNEL_LIBS@
endif

EXTRA_DIST=ncklib.mk include
