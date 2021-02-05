
bin_PROGRAMS = \
	example

common_libs = \
	$(CONFDB_LIBS) \
	$(JANSSON_LIBS) \
	$(LIBLOG_LIBS) \
	$(LIBEVENT_LIBS) \
	-lubox \
	$(NULL)
common_cflags = \
	$(AM_CFLAGS) \
	$(CONFDB_CFLAGS) \
	$(JANSSON_CFLAGS) \
	$(LIBLOG_CFLAGS) \
	$(LIBEVENT_CFLAGS) \
	$(NULL)

example_SOURCES = \
	main.c \
	log.c \
	utils.c \
	apply_conf.c \
	status.c \
	$(NULL)
example_LDADD = \
	$(common_libs) \
	$(LIBNL_LIBS) \
	$(LIBNL_CLI_LIBS) \
	$(NULL)
example_CFLAGS = \
	$(common_cflags) \
	$(LIBNL_CFLAGS) \
	$(LIBNL_CLI_CFLAGS) \
	-I /home/root/applications/libnl-build/usr/local/include/libnl3 \
	$(NULL)

example_LDFLAGS = -L/home/root/applications/libnl-build/usr/local/lib    

AM_CPPFLAGS = \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALSTATEDIR=\"$(localstatedir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DRUNSTATEDIR=\"$(localstatedir)/run\"

AM_CFLAGS = -Wall
