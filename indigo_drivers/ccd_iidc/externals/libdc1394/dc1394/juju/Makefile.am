pkgjujuincludedir = $(pkgincludedir)/juju

if HAVE_LINUX
noinst_LTLIBRARIES = libdc1394-juju.la

# headers to be installed
pkgjujuinclude_HEADERS = 
endif

AM_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/dc1394
libdc1394_juju_la_SOURCES =  \
	control.c \
	capture.c \
	juju.h \
	firewire-cdev.h \
	firewire-constants.h

