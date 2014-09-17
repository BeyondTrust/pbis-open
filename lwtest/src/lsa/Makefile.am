MODE_NAMES = \
	authenticate-user \
	check-gid \
	check-user-info \
	enum-groups \
	enum-users \
	find-group-by-id \
	find-group-by-name \
	find-user-by-id \
	find-user-by-name \
	get-metrics \
	get-status \
	open-session \
	test-local-provider \
	validate-groupinfo-by-api \
	validate-groupinfo-by-id \
	validate-groupinfo-by-name \
	validate-user \
	verify-sid-info

bin_PROGRAMS = lwtest-lsa

lwtest_lsa_SOURCES = \
    lwt_lsa_authenticate_user.c \
    lwt_lsa_check_gid.c \
    lwt_lsa_check_user_info.c \
    lwt_lsa_enum_groups.c \
    lwt_lsa_enum_users.c \
    lwt_lsa_find_group_by_id.c \
    lwt_lsa_find_group_by_name.c \
    lwt_lsa_find_user_by_id.c \
    lwt_lsa_find_user_by_name.c \
    lwt_lsa_get_metrics.c \
    lwt_lsa_get_status.c \
    lwt_lsa_open_session.c \
    lwt_lsa_test_local_provider.c \
    lwt_lsa_validate_groupinfo_by_api.c \
    lwt_lsa_validate_groupinfo_by_id.c \
    lwt_lsa_validate_groupinfo_by_name.c \
    lwt_lsa_validate_groupinfo_common.c \
    lwt_lsa_validate_user.c \
    lwt_lsa_verify_sid_info.c \
    lwt_lsa_framework.c \
    main.c

lwtest_lsa_LDFLAGS = $(LSASS_LDFLAGS)
    
lwtest_lsa_LDADD = \
    $(top_builddir)/src/utility/libutility.la \
    $(LSASS_LIBS)

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	    for name in $(MODE_NAMES); \
	    do \
	        $(LN_S) -f lwtest-lsa$(EXEEXT) lwt-lsa-$${name}$(EXEEXT) || exit $$?; \
	    done
