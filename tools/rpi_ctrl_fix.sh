#!/bin/sh

RPI_CTRL="/usr/bin/rpi_ctrl.sh"
DHCPCD_CONF="/etc/dhcpcd.conf"

[ ! `hostname` = "indigosky" ] && { echo "No patch is applied because this is not INDIGO Sky installation"; exit 0; }
[ ! -f ${RPI_CTRL} ] && { echo "No patch is applied because this is not INDIGO Sky installation"; exit 0; }
[ ! -f ${DHCPCD_CONF} ] && { echo "Can not apply patch because ${DHCPCD_CONF} is missing"; exit 0; }

if ! grep -Eq "^interface wlan0|^static ip_address=192.168.235.1/24|^nohook wpa_supplicant" /etc/dhcpcd.conf; then

    if ! systemctl is-active --quiet hostapd; then
	echo "INDIGO Sky is in WiFi infrastructure mode and hostapd daemon is not active"
	exit 0
    fi
    
    echo "INDIGO Sky is in WiFi infrastructure mode, try to stop and disable hostapd daemon"
    # Stop hostapd daemon.
    systemctl stop hostapd
    if [ $? -eq 0 ]; then
	echo "Daemon hostapd stopped"
    else
	echo "Cannot stop hostapd"
    fi
    # Disable hostapd daemon.
    systemctl disable hostapd
    if [ $? -eq 0 ]; then
	echo "Daemon hostapd disabled"
    else
	echo "Cannot disable hostapd"
    fi
else
    echo "INDIGO Sky is in WiFi access point mode, no software maintainance is required"
fi
