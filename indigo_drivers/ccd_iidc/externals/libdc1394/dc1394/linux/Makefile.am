pkglinuxincludedir = $(pkgincludedir)/linux

if HAVE_LIBRAW1394
noinst_LTLIBRARIES = libdc1394-linux.la

# headers to be installed
pkglinuxinclude_HEADERS = \
	capture.h \
	control.h
endif


AM_CFLAGS =-I$(top_srcdir) -I$(top_srcdir)/dc1394
libdc1394_linux_la_LIBADD = -lraw1394
libdc1394_linux_la_SOURCES =  \
	control.c \
	capture.c \
	capture.h \
	kernel-video1394.h \
	linux.h

