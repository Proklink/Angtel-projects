
bin_PROGRAMS = \
	cdbhsr

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

cdbhsr_SOURCES = \
	main.c \
	log.c \
	utils.c \
	apply_conf.c \
	status.c \
	hsr_module.c \
	wait_list.c \
	$(NULL)
cdbhsr_LDADD = \
	$(common_libs) \
	$(LIBNL_LIBS) \
	$(LIBNL_CLI_LIBS) \
	$(NULL)
cdbhsr_CFLAGS = \
	$(common_cflags) \
	$(LIBNL_CFLAGS) \
	$(LIBNL_CLI_CFLAGS) \
	-I /home/root/projects/danil/newpart/libnl-build/usr/local/include/libnl3 \
	$(NULL)

cdbhsr_LDFLAGS = -L/home/root/projects/danil/newpart/libnl-build/usr/local/lib    

AM_CPPFLAGS = \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALSTATEDIR=\"$(localstatedir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DRUNSTATEDIR=\"$(localstatedir)/run\"

AM_CFLAGS = -Wall
