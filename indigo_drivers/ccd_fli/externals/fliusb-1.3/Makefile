modname := fliusb
obj-m := $(modname).o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
PWD := "$$(pwd)"

ifdef DEBUG
CFLAGS_$(obj-m) := -DDEBUG
endif

EXTRA_CFLAGS		+= -DSGREAD	# enable scatter-gather reads

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) O=$(PWD) -C $(KDIR) M=$(PWD) clean

load:
	rmmod $(modname)
	insmod $(modname).ko

install:
	mkdir -p /lib/modules/$(KVERSION)/misc/$(modname)
	install -m 0755 -o root -g root $(modname).ko /lib/modules/$(KVERSION)/misc/$(modname)
	depmod -a

uninstall:
	rm /lib/modules/$(KVERSION)/misc/$(modname)/$(modname).ko
	rmdir /lib/modules/$(KVERSION)/misc/$(modname)
	depmod -a

INSTALL_PREFIX=/usr/local
PACKAGE_NAME="indigo-fliusb"
PACKAGE_VERSION="1.3"
BUILD_NO="7"
DEBIAN_ARCH=all

FULL_NAME=$(PACKAGE_NAME)-$(PACKAGE_VERSION)-$(BUILD_NO)-$(DEBIAN_ARCH)

REWRITE_DEBS="fliusb-dkms"
package: $(FULL_NAME).deb

deb-prepare:
	install -d /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/bin
	install -d /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/share/$(PACKAGE_NAME)
	install fliusb.c /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/share/$(PACKAGE_NAME)
	install fliusb.h /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/share/$(PACKAGE_NAME)
	install fliusb_ioctl.h /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/share/$(PACKAGE_NAME)
	install Makefile /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/share/$(PACKAGE_NAME)
	install README /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/share/$(PACKAGE_NAME)
	printf "#!/bin/bash\ncd $(INSTALL_PREFIX)/share/$(PACKAGE_NAME) && make clean && make && make install && modprobe fliusb\ngrep -q 'fliusb' /etc/modules || echo 'fliusb' >>/etc/modules" >/tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/bin/indigo-install-fliusb
	printf "#!/bin/bash\ncd $(INSTALL_PREFIX)/share/$(PACKAGE_NAME) && rmmod fliusb && make uninstall && make clean\nsed -i.bak '/^fliusb/d' /etc/modules" >/tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/bin/indigo-remove-fliusb
	chmod +x /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/bin/indigo-install-fliusb
	chmod +x /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/bin/indigo-remove-fliusb

$(FULL_NAME).deb: deb-prepare
	install -d /tmp/$(FULL_NAME)/DEBIAN
	printf "Package: $(PACKAGE_NAME)\nVersion: $(PACKAGE_VERSION)-$(BUILD_NO)\nInstalled-Size: $(shell echo $$((`du -s /tmp/$(FULL_NAME) | cut -f1`)))\nPriority: optional\nArchitecture: $(DEBIAN_ARCH)\nReplaces: $(REWRITE_DEBS)\nMaintainer: INDIGO Community\nDepends: build-essential, raspberrypi-kernel-headers | linux-headers\nDescription: Kernel module for FLI USB devices\n" > /tmp/$(FULL_NAME)/DEBIAN/control
	cp /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/bin/indigo-install-fliusb /tmp/$(FULL_NAME)/DEBIAN/postinst
	cp /tmp/$(FULL_NAME)/$(INSTALL_PREFIX)/bin/indigo-remove-fliusb /tmp/$(FULL_NAME)/DEBIAN/prerm
	sudo chown root /tmp/$(FULL_NAME)
	dpkg --build /tmp/$(FULL_NAME)
	mv /tmp/$(FULL_NAME).deb .
	sudo rm -rf /tmp/$(FULL_NAME)
