cmake_minimum_required(VERSION 2.8)
PROJECT(libsbig C CXX)

set(SBIG_VERSION "2.84")
set(SBIG_SOVERSION "2")

set(FIRMWARE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/firmware")
set(UDEVRULES_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")

IF(UNIX AND NOT WIN32 AND NOT APPLE AND NOT SKIP_LIBS)
 IF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm_v6/libsbigudrv.so ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm_v6/libsbigudrv.a ${CMAKE_BINARY_DIR}/libsbigudrv.a)
 ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm_v7/libsbigudrv.so ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm_v7/libsbigudrv.a ${CMAKE_BINARY_DIR}/libsbigudrv.a)
 ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm_v8/libsbigudrv.so ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm_v8/libsbigudrv.a ${CMAKE_BINARY_DIR}/libsbigudrv.a)

 ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/x86_64/libsbigudrv.so ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/x86_64/libsbigudrv.a ${CMAKE_BINARY_DIR}/libsbigudrv.a)
 ELSE()
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/i386/libsbigudrv.so ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libs/i386/libsbigudrv.a ${CMAKE_BINARY_DIR}/libsbigudrv.a)
 ENDIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")

 # Make sure symbolic links are installed
 file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_sbig_symlink.cmake "
 exec_program(${CMAKE_COMMAND} ARGS -E create_symlink ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libsbigudrv.so.${SBIG_VERSION} ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libsbigudrv.so.${SBIG_SOVERSION})\n
 exec_program(${CMAKE_COMMAND} ARGS -E create_symlink ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libsbigudrv.so.${SBIG_SOVERSION} ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libsbigudrv.so)\n
 ")

 install( FILES ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION} DESTINATION ${LIB_INSTALL_DIR}${LIB_POSTFIX})
 install( FILES ${CMAKE_BINARY_DIR}/libsbigudrv.a DESTINATION ${LIB_INSTALL_DIR}${LIB_POSTFIX})
 install( FILES include/sbigudrv.h DESTINATION ${INCLUDE_INSTALL_DIR}/libsbig)
 install( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_sbig_symlink.cmake)
ENDIF(UNIX AND NOT WIN32 AND NOT APPLE AND NOT SKIP_LIBS)
IF(NOT SKIP_FIRMWARE_INSTALL MATCHES "True")
	install( FILES firmware/sbigucam.hex firmware/sbiglcam.hex firmware/sbigfcam.hex firmware/sbigpcam.hex firmware/stfga.bin DESTINATION ${FIRMWARE_INSTALL_DIR})
	install( FILES udev/51-sbig-debian.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
ENDIF(NOT SKIP_FIRMWARE_INSTALL MATCHES "True")
