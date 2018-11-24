MAINTAINERCLEANFILES = Makefile.in
dist_man_MANS = dc1394_multiview.1  dc1394_reset_bus.1  dc1394_vloopback.1  grab_color_image.1  grab_gray_image.1  grab_partial_image.1
AM_CPPFLAGS = -I$(top_srcdir)

A = grab_gray_image grab_partial_image grab_color_image \
	grab_color_image2 helloworld ladybug grab_partial_pvn \
	basler_sff_info basler_sff_extended_data ladybug5capture
B = dc1394_reset_bus

if HAVE_LIBSDL
if HAVE_OPENGL
A += dc1394_iso
endif
endif

if HAVE_LINUX
if HAVE_VIDEODEV
B += dc1394_vloopback
endif
if HAVE_XV
A += dc1394_multiview
endif
if HAVE_OPENGL
GL_LIBS = -lGL
endif
endif

if HAVE_WINDOWS
if HAVE_OPENGL
GL_LIBS = -lopengl32
endif
endif


if HAVE_MACOSX
AM_LDFLAGS = -framework CoreFoundation $(FRAMEWORK_IOKIT)
endif

noinst_PROGRAMS = $(A)
bin_PROGRAMS = $(B)

LDADD = ../dc1394/libdc1394.la

helloworld_SOURCES = helloworld.c

ladybug_SOURCES = ladybug.c

ladybug5capture_SOURCES = ladybug5capture.c

grab_gray_image_SOURCES = grab_gray_image.c

grab_color_image_SOURCES = grab_color_image.c

grab_color_image2_SOURCES = grab_color_image2.c

grab_partial_image_SOURCES = grab_partial_image.c

grab_partial_pvn_SOURCES = grab_partial_pvn.c

dc1394_vloopback_SOURCES = dc1394_vloopback.c affine.c affine.h
dc1394_vloopback_LDADD = $(LDADD) -lm

dc1394_reset_bus_SOURCES = dc1394_reset_bus.c

basler_sff_info_SOURCES = basler_sff_info.c

basler_sff_extended_data_SOURCES = basler_sff_extended_data.c

dc1394_multiview_CFLAGS = $(X_CFLAGS) $(XV_CFLAGS)
dc1394_multiview_SOURCES = dc1394_multiview.c
dc1394_multiview_LDADD = $(LDADD) $(X_LIBS) $(X_PRE_LIBS) $(XV_LIBS) -lX11 $(X_EXTRA_LIBS)

dc1394_iso_CFLAGS = $(SDL_CFLAGS)
dc1394_iso_SOURCES = dc1394_iso.c
dc1394_iso_LDADD = $(LDADD) $(SDL_LIBS) $(GL_LIBS)

