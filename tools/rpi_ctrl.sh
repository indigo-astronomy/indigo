#!/bin/bash

# Copyright (c) 2019-2021 Thomas Stibor <thomas@stibor.net> & Rumen G. Bogdanovski <rumenastro@gmail.com>
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license'
# (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This script is called in indigo_server and modifies config files
# within the Raspbian GNU/Linux distribution to:
# 1. Setup and WiFi server access point, or
# 2. Setup WiFi client to connect to an access point.
#
# For 1.) following entries must to be adjusted in
# /etc/hostapd/hostapd.conf
# ssid=indigosky
# wpa_passphrase=indigosky
#
# In addition the the following entries must be set in
# /etc/dhcpcd.conf
# interface wlan0
# static ip_address=192.168.235.1/24
# nohook wpa_supplicant
#
# For 2.) following entries must to be adjusted in
# /etc/wpa_supplicant/wpa_supplicant.conf
# ssid="indigosky"
# psk="indigosky"
#
# In addition make sure the following entries must be commented out
# /etc/dhcpcd.conf
# interface wlan0
# static ip_address=192.168.235.1/24
# nohook wpa_supplicant

VERSION=0.24

# Setup RPi as access point server.
WIFI_AP_SSID=""
WIFI_AP_PW=""
OPT_WIFI_AP_SET=0
OPT_WIFI_AP_GET=0

WIFI_AP_CH=6
WIFI_HW_MODE="g"
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
HT_CAPAB=0
REQUIRE_HT=1

HT_CAPAB_HT40='[HT40-][SHORT-GI-20][SHORT-GI-40]'

# Required config files.
CONF_HOSTAPD="/etc/hostapd/hostapd.conf"
CONF_WPA_SUPPLICANT="/etc/wpa_supplicant/wpa_supplicant.conf"
CONF_DHCPCD="/etc/dhcpcd.conf"
CONF_SYSCTL="/etc/sysctl.conf"
PROC_FORWARD="/proc/sys/net/ipv4/ip_forward"

# Required executable files.
SUDO_EXE=$(which sudo)
CP_EXE=$(which cp)
CAT_EXE=$(which cat)
GREP_EXE=$(which grep)
SED_EXE=$(which sed)
POWEROFF_EXE=$(which poweroff)
REBOOT_EXE=$(which reboot)
SYSTEMCTL_EXE=$(which systemctl)
HOSTAPD_EXE=$(which hostapd)
APT_CACHE_EXE=$(which apt-cache)
APT_GET_EXE=$(which apt-get)
DATE_EXE=$(which date)
IW_EXE=$(which iw)
WIFI_CH_SELECT_EXE=$(which wifi_channel_selector.pl)

###############################################
# Show usage and exit with code 1.
###############################################
__usage() {
    echo -e "usage: ${0}\n" \
	 "\t--get-wifi-server\n" \
	 "\t--set-wifi-server <ssid> <password>\n" \
	 "\t--get-wifi-channel\n" \
	 "\t--set-wifi-channel <channel> (${WIFI_CHANNELS[*]}, 0 = auto)\n" \
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
    echo "version: ${VERSION}, written by Thomas Stibor <thomas@stibor.net>"
    echo "and Rumen G. Bogdanovski <rumen@skyarchive.oer>"
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
# Make a copy of *.conf as *.conf.orig
###############################################
__create_reset_files() {
    [[ ! -f ${CONF_HOSTAPD}.orig ]] && { ${CP_EXE} ${CONF_HOSTAPD} ${CONF_HOSTAPD}.orig; }
    [[ ! -f ${CONF_WPA_SUPPLICANT}.orig ]] && { ${CP_EXE} ${CONF_WPA_SUPPLICANT} ${CONF_WPA_SUPPLICANT}.orig; }
    [[ ! -f ${CONF_DHCPCD}.orig ]] && { ${CP_EXE} ${CONF_DHCPCD} ${CONF_DHCPCD}.orig; }
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
# Get active wifi-mode, returns "wifi-server"
# or "wifi-client".
###############################################
__get-wifi-mode() {
    # RPi is in wifi-client mode.
    if ! ${GREP_EXE} -Eq "^interface wlan0|^static ip_address=192.168.235.1/24|^nohook wpa_supplicant" ${CONF_DHCPCD}; then
        echo "wifi-client"
    else
        echo "wifi-server"
    fi
}


###############################################
# Read SSID PW and CHANNEL from hostapd.conf.
###############################################
__read-wifi-credentials() {
    WIFI_AP_SSID=$(__get "ssid" ${CONF_HOSTAPD})
    WIFI_AP_PW=$(__get "wpa_passphrase" ${CONF_HOSTAPD})
}

__read-wifi-channel() {
    WIFI_AP_CH=$(__get "channel" ${CONF_HOSTAPD})
}

###############################################
# Set SSID and PW in conf file hostapd.conf.
###############################################
__set-wifi-server() {
    [[ -z ${WIFI_AP_SSID} ]] && __ALERT "WIFI_AP_SSID not set"
    [[ -z ${WIFI_AP_PW} ]] && __ALERT "WIFI_AP_PW not set"
    [[ -z ${WIFI_AP_CH} ]] && __ALERT "WIFI_AP_CH not set"

    [[ ${#WIFI_AP_PW} -lt 8 ]] && __ALERT "WIFI_AP_PW length < 8"
    [[ ${#WIFI_AP_PW} -gt 63 ]] && __ALERT "WIFI_AP_PW length > 63"
    __validate_channel ${WIFI_AP_CH}
    local channel_valid=$?
    [[ ${channel_valid} -eq 0 ]] && __ALERT "WIFI_AP_CH not in list (${WIFI_CHANNELS[*]})"

    if [[ ${WIFI_AP_CH} -gt 30 ]]; then
        WIFI_HW_MODE="a"
        HT_CAPAB=${HT_CAPAB_HT40}
    fi

    [[ ${WIFI_AP_CH} -eq 0 ]] && __ALERT "Auto Channel Selection is not available"

    ${CP_EXE} "${CONF_HOSTAPD}" "${CONF_HOSTAPD}.backup"
    [[ $? -ne 0 ]] && __ALERT "cannot copy ${CONF_HOSTAPD} to ${CONF_HOSTAPD}.backup"

    ${CP_EXE} "${CONF_DHCPCD}" "${CONF_DHCPCD}.backup"
    [[ $? -ne 0 ]] && __ALERT "cannot copy ${CONF_DHCPCD} to ${CONF_DHCPCD}.backup"

    ${CAT_EXE} > "${CONF_HOSTAPD}" <<EOL
interface=wlan0
driver=nl80211
ssid=${WIFI_AP_SSID}
hw_mode=${WIFI_HW_MODE}
channel=${WIFI_AP_CH}
country_code=UK
ieee80211d=1
ieee80211n=1
ieee80211ac=1
wmm_enabled=1
require_ht=${REQUIRE_HT}
require_vht=${REQUIRE_HT}
ht_capab=${HT_CAPAB}
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=${WIFI_AP_PW}
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
EOL
    [[ $? -ne 0 ]] && { ${CP_EXE} "${CONF_HOSTAPD}.backup" "${CONF_HOSTAPD}"; \
			__ALERT "cannot insert lines to ${CONF_HOSTAPD}"; }

    # Lines do not exist in ${CONF_DHCPCD}, thus append them.
    if ! ${GREP_EXE} -Eq "^interface wlan0|^static ip_address=192.168.235.1/24|^nohook wpa_supplicant" ${CONF_DHCPCD}; then
	${CAT_EXE} >> "${CONF_DHCPCD}" <<EOL
interface wlan0
static ip_address=192.168.235.1/24
nohook wpa_supplicant
EOL
	[[ $? -ne 0 ]] && { ${CP_EXE} "${CONF_DHCPCD}.backup" "${CONF_DHCPCD}"; \
			    __ALERT "cannot append lines to ${CONF_DHCPCD}"; }
    fi

    { echo "OK"; sleep 1; ${SYSTEMCTL_EXE} enable hostapd; sleep 1; ${REBOOT_EXE}; }
}

###############################################
# Get SSID and PW from conf file hostapd.conf.
###############################################
__get-wifi-server() {

    local mode=$(__get-wifi-mode)

    WIFI_AP_SSID=$(__get "ssid" ${CONF_HOSTAPD})
    [[ -z ${WIFI_AP_SSID} ]] && __ALERT "'ssid' not found in ${CONF_HOSTAPD}"

    WIFI_AP_PW=$(__get "wpa_passphrase" ${CONF_HOSTAPD})
    [[ -z ${WIFI_AP_PW} ]] && __ALERT "'wpa_passphrase' not found in ${CONF_HOSTAPD}"

    if [[ ${OPT_VERBOSE} -eq 1 ]]; then
        { echo -e "${WIFI_AP_SSID}\t${WIFI_AP_PW}"; exit 0; }
    elif [[ "${mode}" == "wifi-server" ]]; then
        { echo -e "${WIFI_AP_SSID}"; exit 0; }
    fi

    exit 1;
}

###############################################
# Get CHANNEL from conf file hostapd.conf.
###############################################
__get-wifi-channel() {

    local mode=$(__get-wifi-mode)

    WIFI_AP_CH=$(__get "channel" ${CONF_HOSTAPD})
    [[ -z ${WIFI_AP_CH} ]] && __ALERT "'channel' not found in ${CONF_HOSTAPD}"

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

    __validate_channel ${WIFI_AP_CH}
    local channel_valid=$?
    [[ ${channel_valid} -eq 0 ]] && __ALERT "WiFi channel ${WIFI_AP_CH} is not in list (${WIFI_CHANNELS[*]})"

    if [[ ${WIFI_AP_CH} -gt 30 ]]; then
        WIFI_HW_MODE="a"
        HT_CAPAB=${HT_CAPAB_HT40}
    fi

    if [[ ${WIFI_AP_CH} -eq 0 ]]; then
        WIFI_AP_CH=$($WIFI_CH_SELECT_EXE);
        [[ $? -ne 0 ]] && { __ALERT "cannot auto select WiFi channel"; }
    fi

    __set "channel" ${WIFI_AP_CH} ${CONF_HOSTAPD} >/dev/null 2>&1
    __set "hw_mode" ${WIFI_HW_MODE} ${CONF_HOSTAPD} >/dev/null 2>&1
    __set "require_ht" ${REQUIRE_HT} ${CONF_HOSTAPD} >/dev/null 2>&1
    __set "ht_capab" ${HT_CAPAB} ${CONF_HOSTAPD} >/dev/null 2>&1
    echo 1 2>/dev/null >${PROC_FORWARD}
    [[ $? -ne 0 ]] && { __ALERT "cannot change WiFi channel"; }

    if [[ "${mode}" == "wifi-server" ]]; then
	{ ${SYSTEMCTL_EXE} restart hostapd; sleep 1; __OK; }
    fi

    __ALERT "not in AP mode, change will take effect when switched to AP mode";
}

###############################################
# Set SSID and PW in conf
# file /etc/wpa_supplicant/wpa_supplicant.conf.
###############################################
__set-wifi-client() {

    [[ -z ${WIFI_CN_SSID} ]] && __ALERT "WIFI_CN_SSID not set"
    [[ -z ${WIFI_CN_PW} ]] && __ALERT "WIFI_CN_PW not set"
    [[ ${#WIFI_CN_PW} -lt 8 ]] && "WIFI_CN_PW length < 8"
    [[ ${#WIFI_CN_PW} -gt 63 ]] && "WIFI_CN_PW length > 63"

    ${CP_EXE} "${CONF_WPA_SUPPLICANT}" "${CONF_WPA_SUPPLICANT}.backup"
    [[ $? -ne 0 ]] && __ALERT "cannot copy ${CONF_WPA_SUPPLICANT} to ${CONF_WPA_SUPPLICANT}.backup"

    ${CP_EXE} "${CONF_DHCPCD}" "${CONF_DHCPCD}.backup"
    [[ $? -ne 0 ]] && __ALERT "cannot copy ${CONF_DHCPCD} to ${CONF_DHCPCD}.backup"

    ${CAT_EXE} > "${CONF_WPA_SUPPLICANT}" <<EOL
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=GB
network={
	ssid="${WIFI_CN_SSID}"
	psk="${WIFI_CN_PW}"
}
EOL
    [[ $? -ne 0 ]] && { ${CP_EXE} "${CONF_WPA_SUPPLICANT}.backup" "${CONF_WPA_SUPPLICANT}"; \
			__ALERT "cannot insert lines to ${CONF_WPA_SUPPLICANT}"; }

    # Make sure proper lines exists and remove lines in ${CONF_DHCPCD}.
    if ${GREP_EXE} -Eq "^interface wlan0|^static ip_address=192.168.235.1/24|^nohook wpa_supplicant" ${CONF_DHCPCD}; then
	${SED_EXE} -i "s/^interface wlan0//" ${CONF_DHCPCD}
	[[ $? -ne 0 ]] && { ${CP_EXE} "${CONF_DHCPCD}.backup" "${CONF_DHCPCD}"; \
			    __ALERT "cannot remove 'interface wlan0' in ${CONF_DHCPCD}"; }
	${SED_EXE} -i "s/^static ip_address=192\.168\.235\.1\/24//" ${CONF_DHCPCD}
	[[ $? -ne 0 ]] && { ${CP_EXE} "${CONF_DHCPCD}.backup" "${CONF_DHCPCD}"; \
			    __ALERT "cannot remove 'static ip_address=192.168.235.1/24' in ${CONF_DHCPCD}"; }
	${SED_EXE} -i "s/^nohook wpa_supplicant//" ${CONF_DHCPCD}
	[[ $? -ne 0 ]] && { ${CP_EXE} "${CONF_DHCPCD}.backup" "${CONF_DHCPCD}"; \
			    __ALERT "cannot remove 'nohook wpa_supplicant' in ${CONF_DHCPCD}"; }
    fi

    { echo "OK"; sleep 1; ${SYSTEMCTL_EXE} disable hostapd; sleep 1; ${REBOOT_EXE}; }
}

###############################################
# Set SSID and PW in conf
# file /etc/wpa_supplicant/wpa_supplicant.conf.
###############################################
__get-wifi-client() {

    local mode=$(__get-wifi-mode)

    WIFI_CN_SSID=$(__get "ssid" ${CONF_WPA_SUPPLICANT})
    [[ -z ${WIFI_CN_SSID} ]] && __ALERT "'ssid' not found in ${CONF_WPA_SUPPLICANT}"
    WIFI_CN_SSID=$(echo ${WIFI_CN_SSID} | tr -d '"')

    WIFI_CN_PW=$(__get "psk" ${CONF_WPA_SUPPLICANT})
    [[ -z ${WIFI_CN_PW} ]] && __ALERT "'psk' not found in ${CONF_WPA_SUPPLICANT}"
    WIFI_CN_PW=$(echo ${WIFI_CN_PW} | tr -d '"')

    if [[ ${OPT_VERBOSE} -eq 1 ]]; then
	{ echo -e "${WIFI_CN_SSID}\t${WIFI_CN_PW}"; exit 0; }
    elif [[ "${mode}" == "wifi-client" ]]; then
	{ echo "${WIFI_CN_SSID}"; exit 0; }
    fi

    exit 1;
}

###############################################
# Reset WiFi access point settings, by copying
# all *.conf.orig files to *.conf and restart.
###############################################
__reset-wifi-server() {

    local mode=$(__get-wifi-mode)

    # RPi is in wifi-client mode.
    if [[ "${mode}" == "wifi-client" ]]; then
        # Network device wlan0 is not connected to infrastructure AP.
        if ${IW_EXE} wlan0 link | ${GREP_EXE} -q "^Not"; then
            # Ethernet link has no carrier.
            if [[ $(cat /sys/class/net/eth0/carrier 2>/dev/null) -eq 0 ]]; then
                # Copy back all orig files and restart in wifi-server mode.
                ${CP_EXE} "${CONF_HOSTAPD}.orig" "${CONF_HOSTAPD}"
                ${CP_EXE} "${CONF_WPA_SUPPLICANT}.orig" "${CONF_WPA_SUPPLICANT}"
                ${CP_EXE} "${CONF_DHCPCD}.orig" "${CONF_DHCPCD}"

                ${REBOOT_EXE}
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
    [[ $? -ne 0 ]] && { __ALERT "cannot install indigo version ${1}"; }

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
__check_file_exists ${SUDO_EXE}
__check_file_exists ${CP_EXE}
__check_file_exists ${CAT_EXE}
__check_file_exists ${GREP_EXE}
__check_file_exists ${SED_EXE}
__check_file_exists ${POWEROFF_EXE}
__check_file_exists ${REBOOT_EXE}
__check_file_exists ${SYSTEMCTL_EXE}
__check_file_exists ${HOSTAPD_EXE}
__check_file_exists ${APT_CACHE_EXE}
__check_file_exists ${APT_GET_EXE}
__check_file_exists ${DATE_EXE}
__check_file_exists ${IW_EXE}
# Config files.
__check_file_exists ${CONF_HOSTAPD}
__check_file_exists ${CONF_WPA_SUPPLICANT}
__check_file_exists ${CONF_DHCPCD}
__check_file_exists ${CONF_SYSCTL}
__check_file_exists ${PROC_FORWARD}

__create_reset_files


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
