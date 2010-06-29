noinst_PROGRAMS = lwkrb5str

lwkrb5str_SOURCES = \
    main.c

lwkrb5str_CPPFLAGS =  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/include/public \
    $(KRB5_INCLUDES)

lwkrb5str_LDADD = \
    @top_builddir@/threaded/liblwadvapi_threaded.la \
    @top_builddir@/unthreaded/liblwadvapi_nothr.la \
    $(KRB5_LIBS)


lwkrb5str_LDFLAGS = \
    $(KRB5_LDFLAGS)

