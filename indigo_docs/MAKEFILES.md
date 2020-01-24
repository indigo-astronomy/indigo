# INDIGO makefiles

## Top level makefiles

There are three top level makefiles.

Makefile - builds platform specific bundle (framework + drivers)
Makefile.drvs - builds all platform drivers (should be used in indigo_drivers, indigo_mac_drivers or indigo_linux_drivers)
Makefile.drv - builds a particular driver (should be used in driver source code folder)

When top level Makefile is used, Makefile.inc file is generated (and included by all other makefiles).

## Second level makefiles

There are also second level makefiles (e.g. in indigo_libs, indigo_tools and indigo_server).

They are supposed to be used in enclosing folders only and top level Makefie.inc should be present.

## Third level makefiles

Makefile.drv detects all usuall driver components on an usuall places (driver source code, rules file, firmware files,
SDKs in externals or bin_externals), but if you want to customise a build process, either create your own Makefile
(e.g. like ccd_andor) or just Makefile.inc (e.g. ccd_apogee or ccd_fli) in driver folder.
