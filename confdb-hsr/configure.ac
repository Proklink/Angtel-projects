
AC_PREREQ(2.57)
AC_INIT([confdb-hsr], [0.0.1])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

LT_INIT

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([LIBNL], [libnl-route-3.0], , )
AC_SUBST([LIBNL_CFLAGS])
AC_SUBST([LIBNL_LIBS])

PKG_CHECK_MODULES([LIBNL_CLI], [libnl-cli-3.0], , )
AC_SUBST([LIBNL_CLI_CFLAGS])
AC_SUBST([LIBNL_CLI_LIBS])

PKG_CHECK_MODULES([CONFDB], [confdb], , )
AC_SUBST([CONFDB_CFLAGS])
AC_SUBST([CONFDB_LIBS])

PKG_CHECK_MODULES([JANSSON], [jansson], , )
AC_SUBST([JANSSON_CFLAGS])
AC_SUBST([JANSSON_LIBS])

PKG_CHECK_MODULES([LIBLOG], [liblogging-stdlog], , )
AC_SUBST([LIBLOG_CFLAGS])
AC_SUBST([LIBLOG_LIBS])

PKG_CHECK_MODULES([LIBEVENT], [libevent_core], , )
AC_SUBST([LIBEVENT_CFLAGS])
AC_SUBST([LIBEVENT_LIBS])

AX_PROG_LUA

AC_CONFIG_FILES([Makefile src/Makefile lib/Makefile data/Makefile])
AC_OUTPUT
