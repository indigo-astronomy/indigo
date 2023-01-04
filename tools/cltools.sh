#!/bin/sh

##
# Install autoconf, automake and libtool smoothly on Mac OS X.
# Newer versions of these libraries are available and may work better on OS X
#
# This script is originally from http://jsdelfino.blogspot.com.au/2012/08/autoconf-and-automake-on-mac-os-x.html
#

export autoconf_ver=2.71
export automake_ver=1.16.5
export libtool_ver=2.4.7
export pkgconfig_ver=0.29.2

export build=/tmp/devtools # or wherever you'd like to build
mkdir -p $build

##
# Autoconf
# http://ftpmirror.gnu.org/autoconf

cd $build
curl -OL http://ftpmirror.gnu.org/autoconf/autoconf-$autoconf_ver.tar.gz
tar xzf autoconf-$autoconf_ver.tar.gz
cd autoconf-$autoconf_ver
./configure --prefix=/usr/local
make
sudo make install

##
# Automake
# http://ftpmirror.gnu.org/automake

cd $build
curl -OL http://ftpmirror.gnu.org/automake/automake-$automake_ver.tar.gz
tar xzf automake-$automake_ver.tar.gz
cd automake-$automake_ver
./configure --prefix=/usr/local
make
sudo make install

##
# Libtool
# http://ftpmirror.gnu.org/libtool

cd $build
curl -OL http://ftpmirror.gnu.org/libtool/libtool-$libtool_ver.tar.gz
tar xzf libtool-$libtool_ver.tar.gz
cd libtool-$libtool_ver
./configure --prefix=/usr/local
make
sudo make install

cd $build
curl -OL https://pkgconfig.freedesktop.org/releases/pkg-config-$pkgconfig_ver.tar.gz
tar xzf pkg-config-$pkgconfig_ver.tar.gz
cd pkg-config-$pkgconfig_ver
LDFLAGS="-framework CoreFoundation -framework Carbon" ./configure --prefix=/usr/local --with-internal-glib
make
sudo make install

echo "Installation complete."
