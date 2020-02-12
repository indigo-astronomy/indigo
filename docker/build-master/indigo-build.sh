#!/usr/bin/env bash

BRANCH=$1
REPOSITORY=$2

BRANCH=${BRANCH:-master}
REPOSITORY=${REPOSITORY:-"https://github.com/indigo-astronomy/indigo"}

# udevadm hack
# from <https://github.com/linuxserver/docker-plex/blob/master/Dockerfile>
if [ -e /sbin/udevadm ]; then
    mv /sbin/udevadm /sbin/udevadm.bak
fi
echo "exit 0" > /sbin/udevadm
chmod +x /sbin/udevadm

git clone -b $BRANCH $REPOSITORY indigo && \
cd indigo && \
sudo make install && \
cd .. && \
rm -rf indigo

# cleanup udevadm hack
rm /sbin/udevadm
if [ -e /sbin/udevadm.bak ]; then
    mv /sbin/udevadm.bak /sbin/udevadm
fi
