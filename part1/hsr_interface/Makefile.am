
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = test_create_hsr

test_create_hsr_SOURCES = test_create_hsr.c
test_create_hsr_LDADD = $(LIBNL_LIBS) \
                        $(LIBNL_CLI_LIBS)
test_create_hsr_CFLAGS = $(LIBNL_CFLAGS) \
                         $(LIBNL_CLI_CFLAGS) \
                         -I /home/root/applications/libnl-build/usr/local/include/libnl3
                         
                         

test_create_hsr_LDFLAGS = -L/home/root/applications/libnl-build/usr/local/lib    
                                                

AM_CFLAGS = -Wall
