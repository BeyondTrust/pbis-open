AM_CPPFLAGS = -I$(top_srcdir)/include -D_FILE_OFFSET_BITS=64

bin_PROGRAMS = lwio-fuse-mount

lwio_fuse_mount_SOURCES = \
	common.c          \
	init.c            \
	getattr.c         \
	statfs.c          \
	readdir.c         \
	open.c            \
	release.c         \
	read.c            \
	write.c           \
	truncate.c        \
	mknod.c           \
	create.c          \
	unlink.c          \
	mkdir.c           \
	rmdir.c           \
	rename.c          \
	chmod.c           \
	chown.c           \
	utimens.c         \
	entrypoint.c      \
	main.c

lwio_fuse_mount_LDADD =   \
	$(LWBASE_LIBS) \
	$(top_builddir)/client/liblwioclient.la \
	-lfuse

