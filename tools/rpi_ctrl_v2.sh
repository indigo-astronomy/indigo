#!/bin/bash

# Copyright (c) 2023-2024 Rumen G.Bogdanovski <rumenastro@gmail.com>
# All rights reserved.
#
# This scrpt is based on the original version by
# Thomas Stibor <thomas@stibor.net> & Rumen G.Bogdanovski <rumenastro@gmail.com>
#
# You can use this software under the terms of 'INDIGO Astronomy open-source license'
# (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This script is called by indigo_server and modifies config files of the Raspbian GNU/Linux distribution to:
# 1. Setup and WiFi server access point, or
# 2. Setup WiFi client to connect to an access point.
# 3. Upgrade indigo sidtribution.
#
# This version of the script uses network manager to setup WiFi AP and client.

VERSION=2.01

# Setup RPi as access point server.
DEFAULT_WIFI_AP_SSID="indigosky"
DEFAULT_WIFI_AP_PW="indigosky"

WIFI_AP_SSID=""
WIFI_AP_PW=""
OPT_WIFI_AP_SET=0
OPT_WIFI_AP_GET=0

WIFI_COUNTRY_CODE=""
OPT_WIFI_COUTNRY_CODE_GET=0
OPT_WIFI_COUTNRY_CODE_SET=0

WIFI_AP_CH=6
WIFI_HW_MODE="bg"
OPT_WIFI_CH_SET=0
OPT_WIFI_CH_GET=0

# Connect RPi to access point server.
WIFI_CN_SSID=""
WIFI_CN_PW=""
OPT_WIFI_CN_SET=0
OPT_WIFI_CN_GET=0

# List available indigo versions.
OPT_LIST_AVAIL_VERSIONS=0

# Install certain indigo version.
OPT_INSTALL_VERSION=""

# Get forwarding
OPT_GET_FORWARDING=0

# Enable forwarding
OPT_ENABLE_FORWARDING=0

# Disable forwarding
OPT_DISABLE_FORWARDING=0

# Get date.
OPT_GET_DATE=0

# Set date.
OPT_SET_DATE=""

# Get wifi mode.
OPT_GET_WIFI_MODE=0

# Poweroff.
OPT_POWEROFF=0

# Reboot.
OPT_REBOOT=0

# Reset RPi access point settings.
OPT_WIFI_AP_RESET=0

# Verbose ALERT messages.
OPT_VERBOSE=0

# Valid WIFI channels
WIFI_CHANNELS=('0' '1' '2' '3' '4' '5' '6' '7' '8' '9' '10' '11' '12' '13' '36' '40' '44' '48' '56' '60' '64' '100' '104' '108' '112' '116')

# Connection network manager connection name
CON_NAME="indigo-wifi"

# Required config files.
CONF_SYSCTL="/etc/sysctl.conf"
PROC_FORWARD="/proc/sys/net/ipv4/ip_forward"

# Required executable files.
CAT_EXE=$(which cat)
GREP_EXE=$(which grep)
SED_EXE=$(which sed)
NFT_EXE=$(which nft)
NM_CLI=$(which nmcli)
POWEROFF_EXE=$(which poweroff)
REBOOT_EXE=$(which reboot)
SYSTEMCTL_EXE=$(which systemctl)
APT_CACHE_EXE=$(which apt-cache)
APT_GET_EXE=$(which apt-get)
DATE_EXE=$(which date)
IW_EXE=$(which iw)
RPI_CFG_EXE=$(which raspi-config)
WIFI_CH_SELECT_EXE=$(which wifi_channel_selector.pl)

###############################################
# Show usage and exit with code 1.
###############################################
__usage() {

    echo -e "usage: ${0}\n" \
	 "\t--get-wifi-country-code\n" \
	 "\t--set-wifi-country-code <code> (two letter country code like US, BG etc. NOTE: This will reset the WiFi to defaults)\n" \
	 "\t--get-wifi-server\n" \
	 "\t--set-wifi-server <ssid> <password>\n" \
	 "\t--get-wifi-channel\n" \
	 "\t--set-wifi-channel <channel> (0 = automatically select in 2.4GHz band, available channels dpend on country regulations)\n" \
	 "\t--get-wifi-client\n" \
	 "\t--set-wifi-client <ssid> <password>\n" \
	 "\t--reset-wifi-server\n" \
	 "\t--list-available-versions\n" \
	 "\t--install-version <package-version>\n" \
	 "\t--get-date\n" \
	 "\t--set-date <+%Y-%m-%dT%H:%M:%S%z>\n" \
	 "\t--get-wifi-mode\n" \
	 "\t--enable-forwarding\n" \
	 "\t--disable-forwarding\n" \
	 "\t--get-forwarding\n" \
	 "\t--poweroff\n" \
	 "\t--reboot\n" \
	 "\t--verbose"
    echo "version: ${VERSION}, written by Rumen G.Bogdanovski <rumen@skyarchive.oer>"
    echo ""
    echo "Based ot the original script by Thomas Stibor <thomas@stibor.net>"
    echo "and Rumen G.Bogdanovski <rumen@skyarchive.oer>"
    exit 1
}

###############################################
# Return 1 of channel is a valid WIFI channel
###############################################
__validate_channel() {

    local IFS=$'\n';
    if [ "$1" == "$(compgen -W "${WIFI_CHANNELS[*]}" "$1" | head -1)" ] ; then
        return 1
    else
        return 0
    fi
}


###############################################
# Output to stdout "OK" and exit with 0.
###############################################
__OK() {

    { echo "OK"; exit 0; }
}

###############################################
# Output to stdout "ALERT" and exit with 1
# when not verbose, otherwise
# "ALERT: {reason} and exit with 1.
###############################################
__ALERT() {

    [[ ! -z "${1}" ]] && { echo "ALERT: $@"; exit 1; }
    { echo "ALERT"; exit 1; }
}

###############################################
# Verify that file exists and exit
# with 1 when not.
###############################################
__check_file_exists() {

    [[ ! -f ${1} ]] && __ALERT "file ${1} does not exist"
}

###############################################
# Get value for key in conf file and return
# value when successful, otherwise "".
###############################################
__get() {

    local key=${1}
    local conf_file=${2}
    local delim=${3-=}		# Default value '='

    echo $(${GREP_EXE} -oPm1 "(?<=${key}${delim}).*" ${conf_file})
}

__nm_get() {

    local key=${1}

    echo $(${NM_CLI} -s con show --active ${CON_NAME} 2>/dev/null | grep -oPm1 "(?<=${key}:).*")
}

###############################################
# Set key value in conf file and return 0
# when successful, otherwise 1.
###############################################
__set() {

    local key=${1}
    local value=${2}
    local conf_file=${3}
    local delim=${4-=}		# Default value '='

    if ${GREP_EXE} -q "${key}${delim}" ${conf_file}; then
        ${SED_EXE} -i "s/^${key}${delim}.*/${key}${delim}${value}/" ${conf_file}
        return 0
    fi

    return 1
}

###############################################
# Get ipv4 forwarding => 0 disabled 1 enabled
###############################################
__get-forwarding() {

    local enabled=`${CAT_EXE} ${PROC_FORWARD}`
    echo $enabled
}

###############################################
# Enable ipv4 forwarding
###############################################
__enable-forwarding() {
    __set "net.ipv4.ip_forward" 1 ${CONF_SYSCTL} >/dev/null 2>&1
    echo 1 2>/dev/null >${PROC_FORWARD}
    [[ $? -ne 0 ]] && { __ALERT "cannot enable forwarding"; }

    __OK
}

###############################################
# Disable ipv4 forwarding
###############################################
__disable-forwarding() {
    __set "net.ipv4.ip_forward" 0 ${CONF_SYSCTL} >/dev/null 2>&1
    echo 0 2>/dev/null >${PROC_FORWARD}
    [[ $? -ne 0 ]] && { __ALERT "cannot disable forwarding"; }

    __OK
}


###############################################
# get WiFi country code
###############################################
__get_wifi_country_code() {

    local country_code=`${RPI_CFG_EXE} nonint get_wifi_country`
    echo $country_code
}


###############################################
# set WiFi conutry code
###############################################
__set_wifi_country_code() {

    ${RPI_CFG_EXE} nonint do_wifi_country ${WIFI_COUNTRY_CODE} >/dev/null 2>&1
    [[ $? -ne 0 ]] && { __ALERT "cannot set WiFi country code ${WIFI_COUNTRY_CODE}"; }

    WIFI_AP_SSID=${DEFAULT_WIFI_AP_SSID}
    WIFI_AP_PW=${DEFAULT_WIFI_AP_PW}
    __set-wifi-server
    __OK

}


###############################################
# Get active wifi-mode, returns "wifi-server"
# or "wifi-client".
###############################################
__get-wifi-mode() {

    # RPi is in wifi-client mode.
    MODE=$(__nm_get "802-11-wireless.mode")

    if [[ "$MODE" == "ap" ]]; then
        echo "wifi-server"
    else
        echo "wifi-client"
    fi
}


###############################################
# Read SSID PW and CHANNEL
###############################################
__read-wifi-credentials() {

    WIFI_AP_SSID=$(__nm_get "802-11-wireless.ssid")
    WIFI_AP_PW=$(__nm_get "802-11-wireless-security.psk")
}

__read-wifi-channel() {

    local wifi_ch=$(__nm_get "802-11-wireless.channel")
    [[ -z ${WIFI_AP_CH} ]] || WIFI_AP_CH=$(__nm_get "802-11-wireless.channel")
}

###############################################
# Set SSID and PW
###############################################
__set-wifi-server-silent() {

    [[ -z ${WIFI_AP_SSID} ]] && __ALERT "WIFI_AP_SSID not set"
    [[ -z ${WIFI_AP_PW} ]] && __ALERT "WIFI_AP_PW not set"
    [[ -z ${WIFI_AP_CH} ]] && WIFI_AP_CH=0

    [[ ${#WIFI_AP_PW} -lt 8 ]] && __ALERT "WIFI_AP_PW length < 8"
    [[ ${#WIFI_AP_PW} -gt 63 ]] && __ALERT "WIFI_AP_PW length > 63"
    __validate_channel ${WIFI_AP_CH}
    local channel_valid=$?
    [[ ${channel_valid} -eq 0 ]] && __ALERT "WIFI_AP_CH not in list (${WIFI_CHANNELS[*]})"

    if [[ ${WIFI_AP_CH} -eq 0 ]]; then
        WIFI_AP_CH=$($WIFI_CH_SELECT_EXE 2>/dev/null);
        [[ $? -ne 0 ]] && { __ALERT "cannot auto select WiFi channel"; }
    fi

    if [[ ${WIFI_AP_CH} -gt 30 ]]; then
        WIFI_HW_MODE="a"
    fi

    [[ ${WIFI_AP_CH} -eq 0 ]] && __ALERT "Auto Channel Selection is not available"

    ${NM_CLI} con delete ${CON_NAME} >/dev/null 2>&1
    ${NM_CLI} con add type wifi ifname wlan0 mode ap con-name ${CON_NAME} ssid "${WIFI_AP_SSID}" >/dev/null 2>&1 && \
    ${NM_CLI} con modify ${CON_NAME} 802-11-wireless.band ${WIFI_HW_MODE} >/dev/null 2>&1 && \
    ${NM_CLI} con modify ${CON_NAME} 802-11-wireless.channel ${WIFI_AP_CH} >/dev/null 2>&1 && \
    ${NM_CLI} con modify ${CON_NAME} 802-11-wireless-security.key-mgmt wpa-psk >/dev/null 2>&1 && \
    ${NM_CLI} con modify ${CON_NAME} 802-11-wireless-security.psk "${WIFI_AP_PW}" >/dev/null 2>&1 && \
    ${NM_CLI} con modify ${CON_NAME} ipv4.method shared >/dev/null 2>&1 && \
    ${NM_CLI} con modify ${CON_NAME} ipv4.addr 192.168.235.1/24 >/dev/null 2>&1 && \
    ${NM_CLI} con modify ${CON_NAME} connection.autoconnect yes >/dev/null 2>&1 && \
    ${NM_CLI} con up ${CON_NAME} >/dev/null 2>&1
    [[ $? -ne 0 ]] && __ALERT "cannot create AP with ssid '${WIFI_AP_SSID}'"
}

__set-wifi-server() {
    __set-wifi-server-silent
    __OK
}

###############################################
# Get SSID and PW
###############################################
__get-wifi-server() {

    local mode=$(__get-wifi-mode)

    WIFI_AP_SSID=$(__nm_get "802-11-wireless.ssid")
    [[ -z ${WIFI_AP_SSID} ]] && __ALERT "'ssid' not found"

    WIFI_AP_PW=$(__nm_get "802-11-wireless-security.psk")
    [[ -z ${WIFI_AP_PW} ]] && __ALERT "'psk' not found"

    if [[ ${OPT_VERBOSE} -eq 1 ]]; then
        { echo -e "${WIFI_AP_SSID}\t${WIFI_AP_PW}"; exit 0; }
    elif [[ "${mode}" == "wifi-server" ]]; then
        { echo -e "${WIFI_AP_SSID}"; exit 0; }
    fi

    exit 1;
}

###############################################
# Get CHANNEL
###############################################
__get-wifi-channel() {

    local mode=$(__get-wifi-mode)

    __read-wifi-channel

    [[ -z ${WIFI_AP_CH} ]] && __ALERT "'channel' not found"

    if [[ "${mode}" == "wifi-server" ]]; then
        if [[ "${WIFI_AP_CH}" == "acs_survey" ]]; then
            { echo -e "0"; exit 0; }
        else
            { echo -e "${WIFI_AP_CH}"; exit 0; }
        fi
    fi

    exit 1;
}

###############################################
# set Wifi channel
###############################################
__set-wifi-channel() {

    local mode=$(__get-wifi-mode)

    #__validate_channel ${WIFI_AP_CH}
    #local channel_valid=$?
    #[[ ${channel_valid} -eq 0 ]] && __ALERT "WiFi channel ${WIFI_AP_CH} is not in list (${WIFI_CHANNELS[*]})"

    if [[ ${WIFI_AP_CH} -gt 30 ]]; then
        WIFI_HW_MODE="a"
    fi

    if [[ ${WIFI_AP_CH} -eq 0 ]]; then
        WIFI_AP_CH=$($WIFI_CH_SELECT_EXE 2>/dev/null);
        [[ $? -ne 0 ]] && { __ALERT "cannot auto select WiFi channel"; }
    fi

    if [[ "${mode}" == "wifi-server" ]]; then
        ${NM_CLI} con modify ${CON_NAME} 802-11-wireless.band ${WIFI_HW_MODE} 802-11-wireless.channel ${WIFI_AP_CH} >/dev/null 2>&1 && \
        ${NM_CLI} con up ${CON_NAME} >/dev/null 2>&1
        if [[ $? -ne 0 ]]; then
            ${NM_CLI} con modify ${CON_NAME} 802-11-wireless.band ${WIFI_HW_MODE} 802-11-wireless.channel 6 >/dev/null 2>&1 && \
            ${NM_CLI} con up ${CON_NAME} >/dev/null 2>&1
            __ALERT "cannot set WiFi channel ${WIFI_AP_CH}, channel reset to default"
        fi
        sleep 1
        __OK
    fi

    __ALERT "not in AP mode";
}

###############################################
# Set client SSID and PW
###############################################
__set-wifi-client() {

    [[ -z ${WIFI_CN_SSID} ]] && __ALERT "WIFI_CN_SSID not set"
    [[ -z ${WIFI_CN_PW} ]] && __ALERT "WIFI_CN_PW not set"
    [[ ${#WIFI_CN_PW} -lt 8 ]] && "WIFI_CN_PW length < 8"
    [[ ${#WIFI_CN_PW} -gt 63 ]] && "WIFI_CN_PW length > 63"

    ${NM_CLI} con down ${CON_NAME} >/dev/null 2>&1
    ${NM_CLI} con delete ${CON_NAME} >/dev/null 2>&1
    sleep 3
    ${NM_CLI} dev wifi connect "${WIFI_CN_SSID}" password "${WIFI_CN_PW}" >/dev/null 2>&1
    local res=$?

    ${NM_CLI} con modify "${WIFI_CN_SSID}" con-name ${CON_NAME} connection.autoconnect yes >/dev/null 2>&1
    ${NM_CLI} con up ${CON_NAME} >/dev/null 2>&1

    if [[ ${res} -ne 0 ]]; then
	WIFI_AP_SSID=${DEFAULT_WIFI_AP_SSID}
        WIFI_AP_PW=${DEFAULT_WIFI_AP_PW}
        __set-wifi-server-silent
        __ALERT "failed to connect to WiFi network '${WIFI_CN_SSID}', WiFi set to default AP"
    fi

    __OK
}

###############################################
# Get client SSID and PW
###############################################
__get-wifi-client() {

    local mode=$(__get-wifi-mode)

    WIFI_CN_SSID=$(__nm_get "802-11-wireless.ssid")
    [[ -z ${WIFI_CN_SSID} ]] && __ALERT "'ssid' not found"

    WIFI_CN_PW=$(__nm_get "802-11-wireless-security.psk")
    [[ -z ${WIFI_CN_PW} ]] && __ALERT "'psk' not found"


    if [[ ${OPT_VERBOSE} -eq 1 ]]; then
	{ echo -e "${WIFI_CN_SSID}\t${WIFI_CN_PW}"; exit 0; }
    elif [[ "${mode}" == "wifi-client" ]]; then
	{ echo "${WIFI_CN_SSID}"; exit 0; }
    fi

    exit 1;
}

###############################################
# Reset WiFi access point settings to defaults
###############################################
__reset-wifi-server() {

    local mode=$(__get-wifi-mode)

    # RPi is in wifi-client mode.
    if [[ "${mode}" == "wifi-client" ]]; then
        # Network device wlan0 is not connected to infrastructure AP.
        if ${IW_EXE} wlan0 link | ${GREP_EXE} -q "^Not"; then
            # Ethernet link has no carrier.
            if [[ $(${CAT_EXE} /sys/class/net/eth0/carrier 2>/dev/null) -eq 0 ]]; then
                WIFI_AP_SSID=${DEFAULT_WIFI_AP_SSID}
                WIFI_AP_PW=${DEFAULT_WIFI_AP_PW}
                __set-wifi-server
		__OK
            fi
        fi
    fi
}

###############################################
# Output in a single line all available INDIGO
# versions separated by space, where first item
# denotes the currently installed version.
###############################################
__list-available-versions() {

    # Make sure we have latest package indices (when Internet connectivity).
    ${APT_GET_EXE} update >/dev/null 2>&1
    sleep 1;
    # List current installed version and candidate version or older versions. In total 3 versions are listed.
    echo $(${APT_CACHE_EXE} policy indigo | ${GREP_EXE} -oPm1 "(?<=Installed:\s).*") \
	 $(${APT_CACHE_EXE} policy indigo | ${GREP_EXE} -E -oe '\s\s[0-9]+.[0-9]+\-[0-9]+(\-[0-9]+)?' | tr -d '[:blank:]' | head -n 2)
}

###############################################
# Install INDIGO package version.
###############################################
__install-version() {

    [[ "$#" -ne 1 ]] && { __ALERT "wrong number of arguments"; }
    ${APT_GET_EXE} install -y --allow-downgrades indigo=${1} >/dev/null 2>&1
    local res=$?
    [[ ${res} -eq 100 ]] && { __ALERT "the software updater is busy, please try again later"; }
    [[ ${res} -ne 0 ]] && { __ALERT "cannot install indigo version ${1}, please try again later" ; }

    { echo "OK"; sleep 2; ${REBOOT_EXE}; }
}

###############################################
# Get date in format "+%Y-%m-%dT%H:%M:%S%z"
###############################################
__get-date() {

    { echo $(date "+%Y-%m-%dT%H:%M:%S%z"); exit 0; }
}

###############################################
# Set date in format "+%Y-%m-%dT%H:%M:%S%z"
###############################################
__set-date() {

    [[ "$#" -ne 1 ]] && { __ALERT "wrong number of arguments"; }
    ${DATE_EXE} -s "${1}" >/dev/null 2>&1
    [[ $? -ne 0 ]] && { __ALERT "cannot set date ${1}"; }

    __OK
}

###############################################
# Parse arguments.
###############################################
[[ $# -eq 0 ]] && __usage

while [[ $# -gt 0 ]]
do
    arg="$1"
    case $arg in
        --get-wifi-country-code)
            OPT_WIFI_COUTNRY_CODE_GET=1
            ;;
        --set-wifi-country-code)
            WIFI_COUNTRY_CODE="${2}"
            shift
            OPT_WIFI_COUTNRY_CODE_SET=1
            ;;
	--get-wifi-server)
	    OPT_WIFI_AP_GET=1
	    ;;
	--set-wifi-server)
	    WIFI_AP_SSID="${2}"
	    shift
	    WIFI_AP_PW="${2}"
	    shift
	    OPT_WIFI_AP_SET=1
	    ;;
	--get-wifi-channel)
	    OPT_WIFI_CH_GET=1
	    ;;
	--set-wifi-channel)
	    WIFI_AP_CH="${2}"
	    shift
	    OPT_WIFI_CH_SET=1
	    ;;
	--get-wifi-client)
	    OPT_WIFI_CN_GET=1
	    ;;
	--set-wifi-client)
	    WIFI_CN_SSID="${2}"
	    shift
	    WIFI_CN_PW="${2}"
	    shift
	    OPT_WIFI_CN_SET=1
	    ;;
	--reset-wifi-server)
	    OPT_WIFI_AP_RESET=1
	    ;;
	--list-available-versions)
	    OPT_LIST_AVAIL_VERSIONS=1
	    ;;
	--install-version)
	    OPT_INSTALL_VERSION="${2}"
	    shift
	    ;;
	--get-date)
	    OPT_GET_DATE=1
	    ;;
	--set-date)
	    OPT_SET_DATE="${2}"
	    shift
	    ;;
	--get-wifi-mode)
	    OPT_GET_WIFI_MODE=1
	    ;;
	--get-forwarding)
	    OPT_GET_FORWARDING=1
	    ;;
	--enable-forwarding)
	    OPT_ENABLE_FORWARDING=1
	    ;;
	--disable-forwarding)
	    OPT_DISABLE_FORWARDING=1
	    ;;
	--poweroff)
	    OPT_POWEROFF=1
	    ;;
	--reboot)
	    OPT_REBOOT=1
	    ;;
	--verbose)
	    OPT_VERBOSE=1
	    ;;
	*)
	    echo "unknown argument ${2}"
	    __usage
	    ;;
    esac
    shift
done

# Executable files.
__check_file_exists ${CAT_EXE}
__check_file_exists ${GREP_EXE}
__check_file_exists ${SED_EXE}
__check_file_exists ${POWEROFF_EXE}
__check_file_exists ${REBOOT_EXE}
__check_file_exists ${SYSTEMCTL_EXE}
__check_file_exists ${APT_CACHE_EXE}
__check_file_exists ${APT_GET_EXE}
__check_file_exists ${DATE_EXE}
__check_file_exists ${IW_EXE}
__check_file_exists ${RPI_CFG_EXE}

[[ ${OPT_WIFI_COUTNRY_CODE_GET} -eq 1 ]] && { __get_wifi_country_code; }
[[ ${OPT_WIFI_COUTNRY_CODE_SET} -eq 1 ]] && { __set_wifi_country_code; }
[[ ${OPT_GET_FORWARDING} -eq 1 ]] && { __get-forwarding; }
[[ ${OPT_ENABLE_FORWARDING} -eq 1 ]] && { __enable-forwarding; }
[[ ${OPT_DISABLE_FORWARDING} -eq 1 ]] && { __disable-forwarding; }
[[ ${OPT_WIFI_AP_GET} -eq 1 ]] && { __get-wifi-server; }
[[ ${OPT_WIFI_AP_SET} -eq 1 ]] && { __read-wifi-channel; __set-wifi-server; }
[[ ${OPT_WIFI_CH_GET} -eq 1 ]] && { __get-wifi-channel; }
[[ ${OPT_WIFI_CH_SET} -eq 1 ]] && { __set-wifi-channel; }
[[ ${OPT_WIFI_CN_GET} -eq 1 ]] && { __get-wifi-client; }
[[ ${OPT_WIFI_CN_SET} -eq 1 ]] && { __set-wifi-client ${WIFI_CN_SSID} ${WIFI_CN_PW}; }
[[ ${OPT_WIFI_AP_RESET} -eq 1 ]] && { __reset-wifi-server; }
[[ ${OPT_LIST_AVAIL_VERSIONS} -eq 1 ]] && { __list-available-versions; }
[[ ! -z ${OPT_INSTALL_VERSION} ]] && { __install-version ${OPT_INSTALL_VERSION}; }
[[ ${OPT_GET_DATE} -eq 1 ]] && { __get-date; }
[[ ! -z ${OPT_SET_DATE} ]] && { __set-date ${OPT_SET_DATE}; }
[[ ${OPT_GET_WIFI_MODE} -eq 1 ]] && { __get-wifi-mode; }
[[ ${OPT_POWEROFF} -eq 1 ]] && { ${POWEROFF_EXE}; }
[[ ${OPT_REBOOT} -eq 1 ]] && { ${REBOOT_EXE}; }

