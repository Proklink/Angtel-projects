
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = hsr_nodes

hsr_nodes_SOURCES = hsr_nodes.c
hsr_nodes_LDADD = $(LIBNL_LIBS) \
                        $(LIBNL_CLI_LIBS)
hsr_nodes_CFLAGS = $(LIBNL_CFLAGS) \
                         $(LIBNL_CLI_CFLAGS) \
                         -I /home/root/applications/danil/newpart/libnl-build/include/libnl3
hsr_nodes_LDFLAGS = -L/home/root/applications/danil/newpart/libnl-build/lib                         

AM_CFLAGS = -Wall
