# This file will be installed into daemonconfdir (defined in configure.in)
daemonconf_DATA = eventfwd.conf
EXTRA_DIST = eventfwd.conf.in

sysconf_DATA = eventfwd.reg

# This reads from stdin and replaces all '\' with '\\' and all '/' with '\/'
sed_escape = sed -e 's/\\\\/\\\\\\\\/g' -e 's,/,\\\\/,g'

# @bindir@ must be escaped as [@]bindir[@] so that the configure script
# does not substitute the variable before this makefile is run
do_subst = sed \
	   -e 's/[@]bindir[@]/'"`echo "$(bindir)" | $(sed_escape)`"'/g' \
	   -e 's/[@]sbindir[@]/'"`echo "$(sbindir)" | $(sed_escape)`"'/g'

eventfwd.conf: eventfwd.conf.in Makefile
	$(do_subst) $< >$@
