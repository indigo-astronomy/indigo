# Docker Linux attempt

Owner requested Linux in Docker. Started the existing Docker Desktop installation; engine28.5.1, linux/arm64, Linux6.10.14-linuxkit. Used the existing debian:trixie-slim image, no image downloads or package installation.

Executed from the task root:

```sh
docker run --rm --network none --read-only --privileged debian:trixie-slim sh -c 'uname -srm; if test -d /dev/bus/usb; then find /dev/bus/usb -maxdepth 2 -type c; else echo "No /dev/bus/usb in Docker Linux VM"; fi; for d in /sys/bus/usb/devices/*; do if test -f "$d/idVendor"; then printf "%s " "${d##*/}"; cat "$d/idVendor" "$d/idProduct" | tr "\n" " "; echo; fi; done'
```

Observed only virtual root hubs1d6b:0002 and1d6b:0003, not K-m0a17:00ce. Even the privileged Linux container cannot see the physical camera. Driver hardware tests were NOT run in Linux and cannot be called passing.

Checked Docker's installed USB/IP tools through its documented host mount namespace: usbip is present, usbip port lists no imported devices, usbip list-l lists no exportable physical devices. The Mac has no usbip/usbipd server installed. Temporary containers exited with--rm, no host filesystem mounted and no camera commands issued.

[Docker documentation](https://docs.docker.com/desktop/features/usbip/) requires a host USB/IP server; [FAQ](https://docs.docker.com/desktop/troubleshoot-and-support/faqs/general/) explicitly states direct USB passthrough is unsupported. The documented jiegec/usbip physical-device server itself acquires host USB interfaces through rusb/libusb or nusb. Source inspection and [nusb documentation](https://docs.rs/nusb/latest/nusb/) show that host access is still required; moving only the client into Linux does not remove the current macOS ownership problem. No unfiltered network USB service or new toolchain was installed.
