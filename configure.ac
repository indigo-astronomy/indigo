# process this file with autoconf to get a configure script
AC_PREREQ([2.6.0])
AC_INIT([libdc1394],[2.2.4],[libdc1394-devel@lists.sf.net])
AC_CONFIG_SRCDIR([dc1394/control.c])
AC_CONFIG_MACRO_DIR([m4])

m4_include([ax_check_framework.m4])
 
AC_CANONICAL_SYSTEM

case "$host" in
*-*-cygwin* | *-*-mingw*)
AC_LIBTOOL_WIN32_DLL
;;
esac

#
DX_HTML_FEATURE(OFF)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
#
DX_INIT_DOXYGEN([libdc1394], [Doxyfile.in])
#
AM_PROG_AR
AM_INIT_AUTOMAKE([--warnings=all 1.9.6])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

PKG_PROG_PKG_CONFIG

# check if the 'restrict' prefix is supported
AC_C_RESTRICT

AC_CHECK_LIB(m, pow, [ LIBS="-lm $LIBS" ], [])

PKG_CHECK_MODULES(LIBUSB, [libusb-1.0],
    [AC_DEFINE(HAVE_LIBUSB,[],[Defined if libusb is present])],
    [AC_MSG_WARN([libusb-1.0 not found])])

#MIN_SDL_VERSION=1.2.4
#AH_TEMPLATE(HAVE_LIBSDL, [SDL library])
#AM_PATH_SDL($MIN_SDL_VERSION, [AC_DEFINE(HAVE_LIBSDL) HAVE_LIBSDL="yes"])
#AC_SUBST(SDL_CFLAGS)
#AC_SUBST(SDL_LIBS)

case "$host" in
*-*-linux*)
    have_linux=true
    AC_DEFINE(HAVE_LINUX,[],[Defined if compiled for Linux])
    # Check for the new version of libraw1394 first since they might have both
    # installed and we want to use the newest version
    AC_CHECK_LIB(raw1394, raw1394_channel_modify,
       [AC_DEFINE(HAVE_LIBRAW1394,[],[Defined if libraw1394 is present]) libraw1394=true],
       [AC_MSG_WARN(libraw1394 not found or too old. Please upgrade to 1.2.0 or a more recent version.)])
    ;;
*-*-darwin*)
    AX_CHECK_FRAMEWORK([IOKit])
    if test x$my_cv_framework_IOKit = xyes; then
       AC_DEFINE(HAVE_MACOSX,[],[Defined if IOKit framework is present])
       have_macosx=true
    fi
    platform_LDFLAGS="-framework CoreServices -framework CoreFoundation -framework IOKit"
    AC_SUBST(platform_LDFLAGS)
    ;;
*-*-cygwin* | *-*-mingw*)
    have_windows=true
    AC_DEFINE(HAVE_WINDOWS,[],[Defined if compiled for Windows])
    AC_CHECK_LIB(1394camera, GetCmdrVersion,
      [AC_DEFINE(HAVE_LIB1394CAMERA,[],[Defined if lib1394camera present]) lib1394camera=true],
      [AC_MSG_ERROR(lib1394camera not found or too old. Please upgrade to 6.4.5 or a more recent version.)])
    platform_LDFLAGS="-Wl,--export-all -no-undefined -l1394camera -lsetupapi -lws2_32"
    AC_SUBST(platform_LDFLAGS)
    ;;
esac

AH_TEMPLATE(HAVE_OPENGL, [OpenGL library])
case "$host" in
*-*-linux* | *-*-darwin*)
    AC_CHECK_LIB(GL, main, [AC_DEFINE(HAVE_OPENGL) have_opengl="yes"])
    ;;
*-*-cygwin* | *-*-mingw*)
    AC_CHECK_LIB(opengl32, main, [AC_DEFINE(HAVE_OPENGL) have_opengl="yes"])
    ;;
esac

AM_CONDITIONAL(HAVE_LINUX, test x$have_linux = xtrue)
AM_CONDITIONAL(HAVE_MACOSX, test x$have_macosx = xtrue)
AM_CONDITIONAL(HAVE_WINDOWS, test x$have_windows = xtrue)
AM_CONDITIONAL(HAVE_LIBRAW1394, test x$libraw1394 = xtrue)
AM_CONDITIONAL(HAVE_LIBUSB, test "x$LIBUSB_LIBS" != "x")

AM_CONDITIONAL(HAVE_LIBSDL, test "x$SDL_LIBS" != "x")
AM_CONDITIONAL(HAVE_OPENGL, test "x$have_opengl" != "x")
AC_CHECK_HEADER([linux/videodev.h], [have_videodev=true])
AM_CONDITIONAL(HAVE_VIDEODEV, test x$have_videodev = xtrue)

AC_ARG_ENABLE([examples], [AS_HELP_STRING([--disable-examples], [don't build example programs])], [build_examples=$enableval], [build_examples=true])
AM_CONDITIONAL(MAKE_EXAMPLES, test x$build_examples = xtrue)

# check for Xv extensions (necessary for examples/dc1394_multiview)
# imported from Coriander
AC_DEFUN([AC_CHECK_XV],[
    AC_SUBST(XV_CFLAGS)
    AC_SUBST(XV_LIBS)
    AC_MSG_CHECKING(for Xv extensions)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <X11/Xlib.h>
        #include <X11/extensions/Xvlib.h>]], [[
        int main(void) { (void) XvGetPortAttribute(0, 0, 0, 0); return 0; }
    ]])],[xv=yes],[xv=no])
    AC_MSG_RESULT($xv)
    if test x$xv = xyes; then
        XV_LIBS="-lXv -lXext"
        XV_CFLAGS=""
    fi
    AM_CONDITIONAL(HAVE_XV, test x$xv = xyes)
])
AC_CHECK_XV

AC_HEADER_STDC
AC_CHECK_HEADERS(stdint.h fcntl.h sys/ioctl.h unistd.h sys/mman.h netinet/in.h)
AC_PATH_XTRA

AC_TYPE_SIZE_T

AC_FUNC_MMAP

dnl ---------------------------------------------------------------------------
dnl When making a release:
dnl  1. If the library source code has changed at all since the last release,
dnl     then increment REVISION.
dnl  2. If the interface (ABI) has been modified since the last release,
dnl     increment CURRENT, and set REVISION to 0.
dnl  3. If the interface changes consist solely of additions, increment AGE.
dnl  4. If the interface has removed or changed elements, set AGE to 0.
dnl ---------------------------------------------------------------------------
lt_current=24
lt_revision=0
lt_age=2

AC_SUBST(lt_current)
AC_SUBST(lt_revision)
AC_SUBST(lt_age)

pkgincludedir='$(includedir)/dc1394'
AC_SUBST(pkgincludedir)

dnl AX_CFLAGS_WARN_ALL(CFLAGS)
CFLAGS="-Wall -Wunused -std=gnu99 $CFLAGS"
dnl for pedantic compilation, try to add "-pedantic" to the above string

AC_CONFIG_FILES([ \
    Makefile \
    libdc1394-2.pc \
    dc1394/Makefile \
    dc1394/linux/Makefile \
    dc1394/juju/Makefile \
    dc1394/macosx/Makefile \
    dc1394/windows/Makefile \
    dc1394/usb/Makefile \
    dc1394/vendor/Makefile \
    examples/Makefile \
])
AC_OUTPUT

LINUXMSG="Disabled (Linux not detected)"
RAW1394MSG="Disabled (libraw1394 not found)"
MACOSXMSG="Disabled (Mac OS not detected)"
WINDOWSMSG="Disabled (Windows not detected)"
USBMSG="Disabled (libusb-1.0 not found)"

if test x$have_linux = xtrue; then
  if test x$libraw1394 = xtrue; then
    RAW1394MSG="Enabled"
  fi
  LINUXMSG="Enabled"
fi

if test x$have_macosx = xtrue; then
  MACOSXMSG="Enabled"
fi

if test x$have_windows = xtrue; then
  WINDOWSMSG="Enabled"
fi

if test "x$LIBUSB_LIBS" != "x"; then
  USBMSG="Enabled"
fi

EXAMPLESMSG="No"
SDLEXAMPLESMSG="No"
XVEXAMPLESMSG="No"
VIDEXAMPLESMSG="No"

if test x$build_examples = xtrue; then
  EXAMPLESMSG="Yes"
  if test x$have_opengl = xyes; then
    if test "x$SDL_LIBS" != "x"; then
      SDLEXAMPLESMSG="Yes"
    fi
  fi
  if test x$xv = xyes; then
    XVEXAMPLESMSG="Yes"
  fi
  if test x$have_videodev = xtrue; then
    VIDEXAMPLESMSG="Yes"
  fi
fi

echo "
Configuration (libdc1394):

    Linux support:                      ${LINUXMSG}"
if test "x$LINUXMSG" = "xEnabled"; then
echo "      Legacy drivers (libraw1394):      ${RAW1394MSG}"
fi
echo "    Mac OS X support:                   ${MACOSXMSG}
    Windows support:                    ${WINDOWSMSG}
    IIDC-over-USB support:              ${USBMSG}
    Build examples:                     ${EXAMPLESMSG}"
if test "x$EXAMPLESMSG" = xYes; then
   echo "      Build SDL/OpenGL examples:        ${SDLEXAMPLESMSG}
      Build XV examples:                ${XVEXAMPLESMSG}
      Build V4L examples:               ${VIDEXAMPLESMSG}"
fi
dnl extra newline:
echo ""

