# PTP (ISO15740:2000) DSLR driver

https://people.ece.cornell.edu/land/courses/ece4760/FinalProjects/f2012/jmv87/site/files/pima15740-2000.pdf

## Supported devices

DSLRs with PTP-over-USB support

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_ptp

## Status: Under development - very early phase

Driver is developed and tested with:
* Canon EOS 1100D
* Canon EOS 1000D
* Nikon D7000

## NOTE: If you have trouble connecting to your camera with Linux please make sure the following programs are not running:
* gvfs-gphoto2-volume-monitor
* gvfsd-gphoto2
