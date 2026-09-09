# Astromi.ch MGBox driver

https://astromi.ch

## Supported devices

Astromi.ch MBox, MGBox v1/v2, MGPBox and PBox devices

`MGBox Weather` (combined Weather/Powerbox AUX) and `MGBox GPS` are present on startup, with optional additional instances and no hot-plug support.

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_aux_mgbox

## Status: Stable

Driver is developed and tested with:
* MGPBox

## Connection and controls

Configure the port and baud on `MGBox Weather`, even when connecting only `MGBox GPS`. Both logical devices share the physical connection; either can disconnect while the other remains active. The serial default is 38400 baud. Use `mgbox://host:port` for TCP (default port 9999); a bridge such as Nexbridge can export the serial device.

Capabilities follow the reported device type: `M` enables weather, `G` permits GPS connection and `P` permits the pulse outlet. MBox and PBox reject GPS. The previous README also excluded MGBox from GPS, contradicting the existing driver predicate; the generated driver retains that predicate. Exact MGBox v1/v2 firmware capabilities still need hardware confirmation.

Powerbox/PBox uses the `Switch Control` group of `MGBox Weather`: outlet names, the GPIO pulse switch and pulse lengths belong to Powerbox. Lengths are milliseconds. A pulse reports BUSY until its duration expires and then resets the switch. Disconnect cancels the local completion callback, but cannot abort a pulse already running in hardware. Calibration and mount forwarding wait for matching readback and report ALERT on timeout. Reboot controls reset after a two-second settling delay; the protocol does not independently acknowledge completed reboot.

10Micron firmware is deprecated and unsupported; GPS, weather and pulse control remain available where the reported model supports them.

## Generated source and validation

Edit `indigo_aux_mgbox.driver` and regenerate the C, header and main with `indigo_generator`. The migration, compatibility decisions and validation results are recorded in [REFACTOR.md](REFACTOR.md). The host PTY simulator covers weather, GPS, Powerbox, failure recovery and shared/additional-instance lifecycles. These tests do not validate physical relay timing, hardware reboot/reset behavior, TCP bridges or all model firmware variants.
