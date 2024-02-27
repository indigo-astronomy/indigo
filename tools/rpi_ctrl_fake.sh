#!/bin/bash

# Copyright (c) 2024 CloudMakers, s. r. o.
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
# This is fake versiopn of rpi_ctrl/rpi_ctrl_v2 for debugging purposes on non-rpi systems

VERSION=2.01


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
# Get ipv4 forwarding => 0 disabled 1 enabled
###############################################
__get-forwarding() {
	echo `cat /tmp/rpi_ctrl_forwarding`; exit 0
}

###############################################
# Enable ipv4 forwarding
###############################################
__enable-forwarding() {
	echo 1 >/tmp/rpi_ctrl_forwarding; echo "OK"; exit 0
}

###############################################
# Disable ipv4 forwarding
###############################################
__disable-forwarding() {
	echo 0 >/tmp/rpi_ctrl_forwarding; echo "OK"; exit 0
}

###############################################
# get WiFi country code
###############################################
__get_wifi_country_code() {
		echo `cat /tmp/rpi_ctrl_country_code`; exit 0
}

###############################################
# set WiFi conutry code
###############################################
__set_wifi_country_code() {
	echo ${WIFI_COUNTRY_CODE} >/tmp/rpi_ctrl_country_code; echo "OK"; exit 0
}

###############################################
# Get active wifi-mode, returns "wifi-server"
# or "wifi-client".
###############################################
__get-wifi-mode() {
		echo `cat /tmp/rpi_ctrl_wifi_mode`; exit 0
}

###############################################
# Set SSID and PW
###############################################
__set-wifi-server() {
	echo "wifi-server" >/tmp/rpi_ctrl_wifi_mode	echo ${WIFI_AP_SSID} >/tmp/rpi_ctrl_wifi_server; echo "OK"; exit 0
}

###############################################
# Get SSID and PW
###############################################
__get-wifi-server() {
	echo `cat /tmp/rpi_ctrl_wifi_server`; exit 0
}

###############################################
# Get CHANNEL
###############################################
__get-wifi-channel() {
	echo `cat /tmp/rpi_ctrl_wifi_channel`; exit 0
}

###############################################
# set Wifi channel
###############################################
__set-wifi-channel() {
	echo ${WIFI_AP_CH} >/tmp/rpi_ctrl_wifi_channel; echo "OK"; exit 0
}

###############################################
# Set client SSID and PW
###############################################
__set-wifi-client() {
	echo "wifi-client" >/tmp/rpi_ctrl_wifi_mode; echo ${WIFI_CN_SSID} >/tmp/rpi_ctrl_wifi_client; echo "OK"; exit 0
}

###############################################
# Get client SSID and PW
###############################################
__get-wifi-client() {
	echo `cat /tmp/rpi_ctrl_wifi_client`; exit 0
}

###############################################
# Reset WiFi access point settings to defaults
###############################################
__reset-wifi-server() {
	 echo "OK"; exit 0
}

###############################################
# Output in a single line all available INDIGO
# versions separated by space, where first item
# denotes the currently installed version.
###############################################
__list-available-versions() {
	echo "2.0-270 2.0-272 2.0-268"; exit 0
}

###############################################
# Install INDIGO package version.
###############################################
__install-version() {
		sleep 5; echo "OK"; exit 0
}

###############################################
# Get date in format "+%Y-%m-%dT%H:%M:%S%z"
###############################################
__get-date() {
	echo $(date "+%Y-%m-%dT%H:%M:%S%z"); exit 0;
}

###############################################
# Set date in format "+%Y-%m-%dT%H:%M:%S%z"
###############################################
__set-date() {
	echo "OK"; exit 0
}

###############################################
# Parse arguments.
###############################################

[ ! -f /tmp/rpi_ctrl_forwarding ] && echo "0" >/tmp/rpi_ctrl_forwarding
[ ! -f /tmp/rpi_ctrl_country_code ] && echo "US" >/tmp/rpi_ctrl_country_code
[ ! -f /tmp/rpi_ctrl_wifi_mode ] && echo "wifi-server" >/tmp/rpi_ctrl_wifi_mode
[ ! -f /tmp/rpi_ctrl_wifi_server ] && echo "indigosky" >/tmp/rpi_ctrl_wifi_server
[ ! -f /tmp/rpi_ctrl_wifi_client ] && echo "" >/tmp/rpi_ctrl_wifi_client
[ ! -f /tmp/rpi_ctrl_wifi_channel ] && echo "0" >/tmp/rpi_ctrl_wifi_channel

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

