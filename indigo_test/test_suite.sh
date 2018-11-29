#!/bin/bash

# Version history
# 0.1 by Thomas Stibor <thomas@stibor.net>

#---------------- Helper functions -----------------#
__realpath() {
    path=`eval echo "$1"`
    folder=$(dirname "$path")
    echo $(cd "$folder"; pwd)/$(basename "$path");
}

__log() {
    echo "`date "+%Y-%m-%d %H:%M:%S"` $@"
}

__bin_exists() {
    [[ ! -f ${1} ]] && { __log "ERROR: Cannot find binary '${1}'" ; exit 1; }
}

INDIGO_TEST_PATH=$(dirname $(__realpath $0))
INDIGO_PATH="${INDIGO_TEST_PATH}/.."
INDIGO_DRIVERS_PATH="${INDIGO_PATH}/build/drivers"
INDIGO_SERVER="${INDIGO_PATH}/build/bin/indigo_server"
INDIGO_PROP_TOOL="${INDIGO_PATH}/build/bin/indigo_prop_tool"
INDIGO_SERVER_PID=0
LD_LIBRARY_PATH="${INDIGO_PATH}/indigo_drivers/ccd_iidc/externals/libdc1394/build/lib"

#---------------- INDIGO functions -----------------#
__start_indigo_server() {
    [[ `ps aux | grep "[i]ndigo_server"` ]] && { __log "ERROR: Indigo server is already running" ; exit 1; }

    eval "${INDIGO_SERVER}" &>/dev/null &disown;
    INDIGO_SERVER_PID=`ps aux | grep "[i]ndigo_server" | awk '{print $2}'`
    __log "INFO: Started indigo_server with PID: ${INDIGO_SERVER_PID}"
    sleep 3
}

__stop_indigo_server() {
    [[ ! `ps aux | grep "[i]ndigo_server"` ]] && { __log "ERROR: Indigo server is not running" ; exit 1; }

    kill -9 ${INDIGO_SERVER_PID}
    if [[ $? -ne 0 ]]; then
	__log "ERROR: Cannot kill indigo_server with PID: ${INDIGO_SERVER_PID}"
	exit 1
    else
	__log "INFO: Successfully killed indigo_server with PID: ${INDIGO_SERVER_PID}"
    fi
}

#-------------- INDIGO test functions --------------#
__test_load_drivers() {
    local N=1

    __log "INFO: Starting test_load_drivers"
    # Skip SBIG due to external driver (in form of DMG file) for MacOS is needed.
    for n in $(find ${INDIGO_DRIVERS_PATH} -type f \
		    -not -name "*.so" \
    	     	    -not -name "*.a" \
		    -not -name "*.dylib" \
		    -not -name "*indigo_ccd_sbig*" \
	      );
    do
	DRIVER=`basename ${n}`
	${INDIGO_PROP_TOOL} "Server.LOAD.DRIVER=${DRIVER}"
	if [[ ! `${INDIGO_PROP_TOOL} list "Server.DRIVERS" | wc -l` -eq ${N} ]]; then
	    __log "ERROR: Failed to load driver '${DRIVER}'"
	    exit 1
	fi
	__log "INFO: Successfully loaded driver '${DRIVER}'"
	N=$((N + 1))
    done
}

#------------ Tests required files exists ----------#
__bin_exists ${INDIGO_SERVER}
__bin_exists ${INDIGO_PROP_TOOL}

#------------------- Tests INDIGO ------------------#
__start_indigo_server
__test_load_drivers
__stop_indigo_server

exit 0
