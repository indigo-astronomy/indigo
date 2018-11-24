lib_LTLIBRARIES = lib/libnexstar.la 

lib_libnexstar_la_SOURCES = src/nexstar.c src/nexstar.h \
			 src/deg2str.c src/deg2str.h \
			 src/nexstar_pec.c src/nexstar_pec.h \
			 src/nex_open.c src/nex_open.h

lib_libnexstar_la_LDFLAGS = -version-info 0:1:0 

libnexstar_includedir = $(includedir)/
libnexstar_include_HEADERS = src/nexstar.h \
			     src/deg2str.h src/nexstar_pec.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnexstar.pc

man7_MANS = man/libnexstar.man
	 
EXTRA_DIST = $(man7_MANS) $(sysconf_DATA) testcode/*.c testcode/Makefile examples/*.c examples/Makefile
