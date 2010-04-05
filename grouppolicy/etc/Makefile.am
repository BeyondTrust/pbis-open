grouppolicyconf_DATA = grouppolicy.reg gpagentd.reg

grouppolicy.reg:
	$(RM) $@
	/bin/cat @top_srcdir@/etc/grouppolicy-header.conf > $@
	for csedir in @CSES_SUBDIRS@; do \
	/bin/cat @top_builddir@/server/cses/$$csedir/$$csedir.reg >> $@; \
	done
	/bin/cat @top_srcdir@/etc/grouppolicy-footer.conf >> $@

