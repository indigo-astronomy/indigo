AC_INIT([qsiapi],[7.6.0],[support@qsimaging.com],[qsiapi], www.qsimaging.com)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_COPYRIGHT(qsicopyright.txt)
AC_PROG_CXX
AC_LANG([C++])
AM_PROG_AR

dnl check whether we should disable the cypress based devices
AC_ARG_ENABLE( [cyusb], 
	AS_HELP_STRING([--enable-cyusb],[Enable Cypress USB device support]) 

)

AC_MSG_CHECKING([for Cypress USB device support])
if test "x$enable_cyusb" = "xyes"; then
	AC_MSG_RESULT(enabled.)
else
	AC_MSG_RESULT(disabled.)
fi

dnl conditionall check for Cypress headers and libs

if test "x$enable_cyusb" = "xyes"; then
	AC_CHECK_HEADER([cyusb.h])
	CY_LIBS="-L/usr/local/lib -lcyusb"
	CY_CPPFLAGS=""
else
	CY_LIBS=""
	CY_CPPFLAGS="-DNO_CYUSB"
fi
AC_SUBST(CY_LIBS)
AC_SUBST(CY_CPPFLAGS)

dnl check whether the TIFF library is installed, this adds the -ltiff linker 
dnl flag. Furthermore, the presence of the tiffio.h header triggers
dnl inclusion of the tiff code in the programs
AC_CHECK_HEADERS([tiff.h tiffio.h])
AC_CHECK_LIB([tiff],[TIFFOpen])

dnl check whether the CFITSIO library is installed, this addes the -lfitsio
dnl linker flag and enables FITS output in the demo program
AC_CHECK_HEADERS([fitsio.h])
AC_CHECK_LIB([cfitsio], [fits_init_cfitsio])

dnl check whether we should build the demo program. If we do so, then we
dnl also have to check for the presence of the tiff headers and libraries
AC_ARG_ENABLE(demo,
AS_HELP_STRING([--enable-demo],[Compile demo program]),,)
AM_CONDITIONAL(ENABLE_DEMO, test x$enable_demo = xyes)

LT_PREREQ([2.2])
LT_INIT
LT_LANG([C++])

dnl New version of the Stack selection scheme. AC_ARG_ENABLE works nicely
dnl when there are only two options, one can create an option for each
dnl option. Hoever, we now have three USB stacks available, so AC_ARG_WITH
dnl is much more apropriate. This also means that we dont have to deal
dnl with the fact that we can compile in only one, exclusion is handled
dnl by the syntax of the command
dnl AC_ARG_WITH([ftd],
dnl   [AS_HELP_STRING([--with-ftd=stack], [select FT stack: ftd2xx, ftdi, ftdi1])],
dnl   [],
dnl   [AC_MSG_ERROR([must use option --with-ftd=stack to select an FT stack])]
dnl )
AC_ARG_WITH([ftd],
  [AS_HELP_STRING([--with-ftd=stack], [select FT stack: ftd2xx, ftdi, ftdi1])],
  [],
  [with_ftd=ftd2xx]
)

AC_MSG_CHECKING([for selected FT stack])
case x$with_ftd in
	xftd2*)	AC_MSG_RESULT(ftd2xx)
		;;
	xftdi)	AC_MSG_RESULT(libftdi version 0.x)
		;;
	xftdi1)	AC_MSG_RESULT(libftdi version 1.x)
		;;
	*)	AC_MSG_ERROR(Unknown FT stack: $with_ftd)
		;;
esac

dnl Set variables depending on the selected stack. This still is somewhat
dnl rudimentary, e.g. it is not checked whether the the libraries actually
dnl exist and can be linked. Currently, the compile will fail, and it cannot
dnl handle different library locations. E.g. in the case of libftdi1, the
dnl pgk-config script should be used to find the correct flags
LIBFTD2XX_REQUIRED=1.1.12
LIBFTDI_REQUIRED=0.19

case x$with_ftd in
	xftd2*)
		enable_ftd2xx="yes"
		FTD2XX_CFLAGS="-I/usr/local/include"
		FTD2XX_LIBS="-L/usr/local/lib -lftd2xx"
		AC_SUBST(FTD2XX_CFLAGS) 
		AC_SUBST(FTD2XX_LIBS)
		FTDI_CFLAGS="${FTD2XX_CFLAGS}"
		FTDI_LIBS="${FTD2XX_LIBS}"        
		;;
	xftdi)
		enable_ftdi="yes"
		FTDI_CFLAGS="-I/usr/local/include"
		FTDI_LIBS="-L/usr/local/lib -lftdi"
		;;
	xftdi1)
		enable_ftdi1="yes"
		FTDI_CFLAGS="`pkg-config libftdi1 --cflags`"
		FTDI_LIBS="`pkg-config libftdi1 --libs`"
		;;
esac
AC_SUBST(FTDI_CFLAGS) 
AC_SUBST(FTDI_LIBS)

AM_CONDITIONAL(ENABLE_LIBFTDI, [test x$enable_ftdi = xyes])
AM_CONDITIONAL(ENABLE_LIBFTDI1, [test x$enable_ftdi1 = xyes])
AM_CONDITIONAL(ENABLE_LIBFTD2XX, [test x$enable_ftd2xx = xyes])

dnl --------------------------------------------------------------------------------
 
AC_CONFIG_FILES([libqsiapi-config])
AC_CONFIG_COMMANDS([default],[chmod a+x libqsiapi-config],[])
AC_CONFIG_FILES(Makefile doc/Makefile lib/Makefile src/Makefile demo_src/Makefile)

AC_OUTPUT
