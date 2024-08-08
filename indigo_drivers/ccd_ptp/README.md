# PTP (ISO15740:2000) DSLR driver

PTP driver can handle multiple DSLR or mirrorless cameras using PTP-over-USB protocol. It supports generic PTP protocol and its Nikon, Canon, Sony and Fuji extensions.

It is a multiplatform replacement for platform dependent gPhoto2 and ICA drivers.

https://people.ece.cornell.edu/land/courses/ece4760/FinalProjects/f2012/jmv87/site/files/pima15740-2000.pdf

## Supported devices

DSLRs and mirrorless cameras with PTP-over-USB support

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_ptp

## Status: Stable

Driver is developed and tested with:
* Canon EOS 1000D
* Canon EOS 1100D
* Canon EOS 600D
* Nikon D70
* Nikon D3000
* Nikon D5100
* Nikon D5300
* Nikon D5600
* Nikon D7000
* Nikon D750
* Sony 7RII
* Sony 7RIV
* Fujifilm X-T1

### NOTE: If you have trouble connecting to your camera with Linux please make sure the following programs are not running:
* gvfs-gphoto2-volume-monitor
* gvfsd-gphoto2

### NOTE: If you have trouble getting a frame, please make sure you have a memory card inserted into the camera. This driver will not work with cameras without a memory card.
