AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

libexec_PROGRAMS	= ticketcopy
ticketcopy_SOURCES	= ticketcopy.c
ticketcopy_LDADD	= \
			  @top_builddir@/client/liblsaclient.la \
			  @top_builddir@/logging_r/liblsalog_r.la \
			  @top_builddir@/common/liblsacommon.la \
			  -llwadvapi \
			  -llwbase_nothr \
			  -lpopt
ticketcopy_LDFLAGS	= -framework Kerberos

launchagent_DATA = com.likewisesoftware.ticketcopy.plist
launchagentdir = $(sysconfdir)/LaunchAgents
