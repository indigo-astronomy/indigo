#!/usr/bin/env bash

# hostapd
ip link set wlan0 down
ip link set wlan0 up
ip addr flush dev wlan0
ip addr add 192.168.235.1/24 dev wlan0
sudo hostapd -B /etc/hostapd/hostapd.conf

# dnsmasq
sudo service dnsmasq start

# indigo
sudo udevadm trigger --action=change && \
/lib/systemd/systemd-udevd & \
indigo_server -vv
