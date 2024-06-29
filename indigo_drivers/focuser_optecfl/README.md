# Optec FocusLynx focuser driver

https://optec.us/resources/documents/FocusLynx/FocusLynx_Command_Processing_rev3.pdf

For Optec TCF-S focusers use indigo_focuser_optec driver instead.

## Supported devices
* Optec FocusLynx focusers

Single controller instance with two devices is present on the first startup (no hot-plug support). There is no multiinstance support yet!

Only Serial-over-USB connection is supported.

Temperature compensation is not supported yet.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_focuser_optecfl

## Status: Untested

