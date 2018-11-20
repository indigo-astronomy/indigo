bin_PROGRAMS = qsiapitest qsifilter qsiimage

qsiapitest_CPPFLAGS = $(all_includes) -I$(top_srcdir)/lib -I/usr/local/include
qsiapitest_CXXFLAGS = -g -Wall

qsiapitest_LDFLAGS = $(all_libraries) -L$(top_srcdir)/lib -L/usr/local/lib
qsiapitest_LDADD = $(top_builddir)/lib/libqsiapi.la 

qsiapitest_SOURCES = qsiapitest.cpp

qsifilter_CPPFLAGS = $(all_includes) -I$(top_srcdir)/lib -I/usr/local/include
qsifilter_CXXFLAGS = -g -Wall

qsifilter_LDFLAGS = $(all_libraries) -L$(top_srcdir)/lib -L/usr/local/lib
qsifilter_LDADD = $(top_builddir)/lib/libqsiapi.la 
qsifilter_SOURCES =	qsifilter.cpp

qsiimage_CPPFLAGS = $(all_includes) -I$(top_srcdir)/lib -I/usr/local/include
qsiimage_CXXFLAGS = -g -Wall

qsiimage_LDFLAGS = $(all_libraries) -L$(top_srcdir)/lib -L/usr/local/lib
qsiimage_LDADD = $(top_builddir)/lib/libqsiapi.la 
qsiimage_SOURCES = qsiimage.cpp
 
