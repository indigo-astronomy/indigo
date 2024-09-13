# PENTAX Camera driver

https://en.wikipedia.org/wiki/Template:Pentax_digital_interchangeable_lens_cameras

## Supported devices

The following cameras are, in theory, supported by this driver. Nevertheless the most of them were never tested.

* Pentax K100D - APS-C 6.1 MP
* Pentax K100D Super - APS-C 6.1 MP
* Pentax K110D - APS-C 6.1 MP
* Pentax K10D - APS-C 10.2 MP
* Pentax K20D - APS-C 14.6 MP
* Pentax K200D - APS-C 10.2 MP
* Pentax K-m - APS-C 10.2 MP
* Pentax K-x - APS-C 12.4 MP
* Pentax K-r - APS-C 12.4 MP
* Pentax K-7 - APS-C 14.6 MP
* Pentax K2000 - APS-C 10.2 MP
* Pentax GX10 - APS-C 10.2 MP (Samsung rebranded)
* Pentax GX20 - APS-C 14.6 MP (Samsung rebranded)
* Pentax K-01 - APS-C 16.3 MP (Mirrorless)
* Pentax K-30 - APS-C 16.3 MP
* Pentax K-50 - APS-C 16.3 MP
* Pentax K-500 - APS-C 16.3 MP
* Pentax K-5 - APS-C 16.3 MP
* Pentax K-5 II - APS-C 16.3 MP
* Pentax K-5 IIs - APS-C 16.3 MP (No AA filter)
* Pentax K-3 - APS-C 24.1 MP
* Pentax K-3 II - APS-C 24.1 MP
* Pentax K-S1 - APS-C 20.12 MP
* Pentax K-S2 - APS-C 20.12 MP
* Pentax K-1 - Full-frame 36.4 MP
* Pentax K-1 Mark II - Full-frame 36.4 MP
* Pentax KP - APS-C 24.3 MP
* Pentax K-70 - APS-C 24 MP
* Pentax KF - APS-C 24 MP
* Pentax K-3 Mark III - APS-C 25.7 MP
* Pentax 645D - Medium Format 40 MP
* Pentax 645Z - Medium Format 51.4 MP

This driver supports hot-plug (multiple devices).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_ccd_pentax

## Comments

The following VIDs are recognised by this driver: 0x0A17 (Pentax) a 0x04E8 (rebranded Samsung).

On macOS there is a conflict between the driver and USB Mass Storage Device driver, so the driver must be executed with elevated privileges to detach from the system driver. On linux the situation is uknown yet.

## Status: Under development

Driver is developed and tested with:
* Pentax K-m
