# - Try to find DC1394
# Once done, this will define
#
#  DC1394L_FOUND       - system has libdc1394 installed
#  DC1394_INCLUDE_PATH - the dc1394 include directory
#  DC1394_LIBS         - Link these to use DC1394
#
# Author: Peter Antoniac <pan1nx@linux.com>
# Copyright (c) 2014

SET(DC1394_HEADER "dc1394/control.h")
FIND_PATH(DC1394_INCLUDE_DIR ${DC1394_HEADER} PATHS
	"${CMAKE_INSTALL_PREFIX}/include"
	/usr/include
	/usr/local/include
	DOC "The path to DC1394 headers")
FIND_LIBRARY(DC1394_LIBRARY_PATH NAMES dc1394 HINTS
	/usr/lib
	/usr/local/lib)
SET(DC1394_FOUND "NO")
IF(DC1394_INCLUDE_DIR)
	IF(DC1394_LIBRARY_PATH)
		SET(DC1394_LIBS ${DC1394_LIBRARY_PATH})
		SET(DC1394_FOUND "YES")
		SET(DC1394_INCLUDE_PATH ${DC1394_INCLUDE_DIR})
		MESSAGE(STATUS "Found DC1394_INCLUDE_PATH=${DC1394_INCLUDE_PATH}")
		MESSAGE(STATUS "Found DC1394_LIBS=${DC1394_LIBS}")
	ELSE()
		MESSAGE(FATAL_ERROR "Includes dir is missing:
		${DC1394_INCLUDE_DIR}/${DC1394_HEADER}
	Sometimes is because some paths changed (from /usr to /usr/local).
	Make sure that you rm -rf CMakeCache.txt; make rebuild_cache;")
	ENDIF(DC1394_LIBRARY_PATH)
ENDIF(DC1394_INCLUDE_DIR)

