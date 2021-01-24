
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = vlan

vlan_SOURCES = vlan.c
vlan_LDADD = $(LIBNL_LIBS) \
                        $(LIBNL_CLI_LIBS)
vlan_CFLAGS = $(LIBNL_CFLAGS) \
                         $(LIBNL_CLI_CFLAGS)

AM_CFLAGS = -Wall
