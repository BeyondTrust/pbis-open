AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include

MODE_NAMES = \
	ad-cache \
	add-group \
	add-user \
	del-group \
	del-user \
	enum-groups \
	enum-users \
	find-by-sid \
	find-group-by-id \
	find-group-by-name \
	find-user-by-id \
	find-user-by-name \
	get-log-info \
	get-metrics \
	get-status \
	list-groups-for-user \
	ypcat \
	ypmatch \
	mod-group \
	mod-user \
	refresh-configuration \
	set-default-domain \
	set-log-level \
	set-machine-sid \
	set-machine-name \
	find-objects \
	enum-objects \
	enum-members \
	authenticate-user \
	query-member-of

bin_PROGRAMS = lw-lsa

lw_lsa_SOURCES = \
	ad_cache_main.c \
	ad_get_machine.c \
	add_group_add_group.c \
	add_group_main.c \
	add_user_add_user.c \
	add_user_main.c \
	del_group_del_group.c \
	del_group_main.c \
	del_user_main.c \
	enum_groups_main.c \
	enum_users_main.c \
	find_by_sid_main.c \
	find_group_by_id_main.c \
	find_group_by_name_main.c \
	find_user_by_id_main.c \
	find_user_by_name_main.c \
	get_log_info_main.c \
	get_metrics_main.c \
	get_status_main.c \
	list_groups_for_user_main.c \
	lw_ypcat_main.c \
	lw_ypmatch_main.c \
	mod_group_main.c \
	mod_user_main.c \
	refresh_configuration_main.c \
	set_default_domain_main.c \
	set_log_level_main.c \
	set_machine_sid_main.c \
	set_machine_name_main.c \
	trace_info_main.c \
	find_objects.c \
	enum_objects.c \
	enum_members.c \
	query_member_of.c \
	authenticate_user.c \
	common.c \
	main.c

lw_lsa_LDADD = \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/common/liblsacommon.la \
	-llwbase_nothr \
	-lpopt \
	-llwadvapi_nothr

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	    for name in $(MODE_NAMES); \
	    do \
	        $(LN_S) -f lw-lsa$(EXEEXT) lw-$${name}$(EXEEXT) || exit $$?; \
	    done
