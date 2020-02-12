#!/usr/bin/env bash

sudo udevadm trigger --action=change && \
/lib/systemd/systemd-udevd & \
indigo_server -vv
