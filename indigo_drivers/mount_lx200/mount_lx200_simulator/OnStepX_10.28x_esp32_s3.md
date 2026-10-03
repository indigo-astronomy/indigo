# OnStepX ESP32-S3 firmware image with focuser and auxiliary features

`OnStepX_10.28x_esp32_s3.bin` is a prebuilt image of the OnStepX go-to
controller with a mount, one focuser and four auxiliary features, kept here so
the LX200 driver (and OnStepX Alpaca servers) can be tested against the real
firmware without a checkout of the controller sources. It is a test asset, not
part of any INDIGO build. It replaces an earlier image of the same name that had
no focuser and no features and did not answer on the bench board.

## Upstream

* Project: OnStepX, https://github.com/hjd1964/OnStepX
* Source commit: `65a751825677a03a0b12b15546780b614a206435`. `:GVN#` reports
  `10.28x`.
* Licence: GNU GPL v3. The sources are unmodified apart from `Config.h`.
* Libraries: EspSoftwareSerial 8.1.0 and TMCStepper 0.7.3 (needed to compile,
  unused by the GENERIC drivers).

## Image

* SHA-256: `fc64c3ba14a9a4c4899f5a62457edf4520bfc07fa2a6ee905f19fc09cad61235`
* Size: 4194304 bytes
* Merged image for offset `0x0`: second stage bootloader, partition table,
  `boot_app0` and the application. Partition scheme `huge_app` (3 MB
  application, no OTA), logical flash size 4 MB, so it runs on any ESP32-S3
  with at least 4 MB of flash. PSRAM is not used.
  It is not an application-only image and must not be flashed at another offset.
  Flashing it erases the NVS region, so OnStepX starts from its defaults.
* Built with arduino-cli, Arduino core esp32 3.3.8, FQBN
  `esp32:esp32:esp32s3:FlashSize=4M,PartitionScheme=huge_app,USBMode=hwcdc,CDCOnBoot=cdc,PSRAM=disabled`.

Flash it from this directory, with the board's port in `ESP32_PORT`:

```sh
python -m esptool --chip esp32s3 --port "$ESP32_PORT" \
  --baud 460800 write_flash 0x0 OnStepX_10.28x_esp32_s3.bin
```

## Configuration

`PINMAP OFF` with the pins defined in `Config.h` for an ESP32-S3-DevKitC-1.
The pins avoid the strapping pins, native USB, UART0, the octal flash and PSRAM
pins and the RGB LED. Everything else is the upstream default.

* Mount: GEM, AXIS1 and AXIS2 `GENERIC` step/dir, 12800 steps per degree,
  slew rate 1 deg/s. Step / direction: AXIS1 4 / 5, AXIS2 6 / 7.
* Focuser 1: AXIS4 `GENERIC`, 0.5 steps per micron, 0 to 50 mm (Alpaca
  servers report 25000 steps of 2 microns). Step / direction 15 / 16.
* Shared enable pin 17 for all axes.
* Features:
  * 1 `Switch 1`, SWITCH, GPIO38
  * 2 `Switch 2`, SWITCH, GPIO39
  * 3 `Power 3`, ANALOG_OUT (0..255), GPIO40
  * 4 `Pulse 4`, MOMENTARY_SWITCH, GPIO41
* No time or location source: set the date, time and site before the first
  GoTo; the mount refuses to track until the date is set.
* Wi-Fi, Bluetooth and plugins are off, `DEBUG` is off.

## Interfaces

* Serial A: UART0 at 9600 baud, 8N1, on GPIO43 (TX) and GPIO44 (RX), which on
  an ESP32-S3-DevKitC-1 is the on-board USB-to-UART bridge. A host that pulses
  RTS on this port resets the board.
* Serial B: native USB Serial/JTAG (USB CDC on boot), any baud rate. The first
  reply after the port is opened arrives late, so a probe that reads only one
  reply per command sees it attached to the second command.

## Verified

On indigosky with both sockets cabled, 2026-10-03: LX200 on both ports, and
the juanjol OnStepX Alpaca server v0.2.0-beta.8 on `/dev/ttyACM1` (native USB)
driving the mount (tracking, GoTo to RA 18.867 h / Dec +45 landing within
0.0002 h / 0.0001 deg), the focuser (move to 2000 steps) and all four features
over Alpaca, with UDP discovery answering from the Mac.
