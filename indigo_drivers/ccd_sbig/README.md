# SBIG CCD, Filter Wheel and Adaptive Optics driver

http://diffractionlimited.com/

## Supported devices

All SBIG CCD USB and Ethernet cameras along with connected to them filter wheels and adaptive optics.

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver depends on 3rd party library and is only partly supported on Linux (Intel 32/64 bit, ARM v6+ and ARM 64 bit) and MacOS, because Diffraction Limited/SBIG cancelled the suport of the closed source libraries for these platforms.

## License

INDIGO Astronomy open-source license (3rd party library is closed source).

## Use

indigo_server indigo_ccd_sbig

## Status: Stable

Driver is developed and tested with:
* SBIG ST-2000XCM
* SBIG ST-2000XM
* SBIG ST-7XE
* SBIG CFW-8A
* SBIG AO-7
* SBIG Camera Simulator

## NOTES
### Problem on ARM 64 bit architecture

If the driver fails to load on a 64-bit Raspberry PI with the error:

```
ELF load command address/offset not page-aligned
```

The solution is add the folowing line to /boot/firmware/config.txt
```
kernel=kernel8.img
```

### Legacy Filter wheels

CFW8 and CFW6A are legacy filter wheels and can not be auto detected. In order to use
them one should set environment variable SBIG_LEGACY_CFW to CFW8 or CFW6A like:

export SBIG_LEGACY_CFW=CFW8

This will expose the device even it is not really present.

### AO-7

AO-8 and later are auto detected, but AO-7 cannot be auto detected. In order to use it 
one should set environment variable SBIG_LEGACY_AO to AO7 like:

export SBIG_LEGACY_AO=AO7

This will expose the device even if it is not really present.
