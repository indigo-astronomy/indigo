# ESP32Go ESP32-S3 firmware image

`ESP32Go_bbf224a_esp32_s3.bin` is a prebuilt image of the ESP32Go go-to mount
controller, kept here so the LX200 driver can be tested against the firmware
without a checkout of the controller sources. It is a test asset, not part of
any INDIGO build.

## Upstream

* Project: ESP32Go, https://github.com/masutokw/esp32go
* Source commit: `bbf224ad8dd4112cfd09527a398f4e1c040d4027`, abbreviated
  `bbf224a` in the file name. Upstream itself versions by build date, which
  `:GVN#` reports as `06.9` for this image.
* Vendored dependency: Ephemeris, https://github.com/MarScaper/ephemeris,
  commit `cca21485e7a9b3e9a1a5e5fd2da619998c10d933`.
* No licence file is present in the source archive the image was built from,
  and the upstream licence was not established while preparing this asset.
  Check it before redistributing the binary or the sources it came from.

## ESP32-S3 adaptations

The upstream project targets the original ESP32. The image was built from an
adapted tree that disables Bluetooth Classic and the TMC UART probing and the
Nunchuck reader for a bare board, reassigns the pins, keeps the LX200 UART, the
Wi-Fi interface and the web interface, bounds the UART buffer writes and repairs
an upstream `versionFromCompileDate()` that wrote twelve bytes into a six-byte
buffer. Native USB CDC is disabled, so the LX200 protocol is only on UART0.

## Image

* SHA-256: `647e4261a0f76b545be1085d9e8db2caea9fe92598ad2edf30eddb02e5e413ac`
* Size: 1125488 bytes
* Merged image for offset `0x0`: second stage bootloader at `0x0`, partition
  table at `0x8000`, `boot_app0` at `0xe000` and the application at `0x10000`.
  Partition layout `default_8MB.csv`, logical flash size 8 MB, DIO at 80 MHz.
  It is not an application-only image and must not be flashed at another offset.
* Built with PlatformIO, platform `espressif32@7.1.3`, Arduino core 2.0.17,
  board `esp32-s3-devkitc-1`, `-D ARDUINO_USB_CDC_ON_BOOT=0`.

Flash it from this directory, with the board's port in `ESP32_PORT`:

```sh
python -m esptool --chip esp32s3 --port "$ESP32_PORT" \
  --baud 460800 write_flash 0x0 ESP32Go_bbf224a_esp32_s3.bin
```

## Pin configuration

Bench development board, step / direction / enable:

* AZ/RA: 4 / 2 / 6
* ALT/DEC: 5 / 18 / 7
* Focus: 12 / 13 / 14
* Aux: 15 / 16 / 17
* AZ_RES 11, ALT_RES 21, buzzer 10, optional I2C SDA 8 / SCL 9

## Interfaces

* Serial: UART0 at 115200 baud, 8N1, on GPIO43 (TX) and GPIO44 (RX), which on an
  ESP32-S3-DevKitC-1 is the on-board USB-to-UART bridge. The firmware answers
  LX200 there about twelve seconds after a reset, and a host that asserts DTR or
  RTS when it opens the port resets the board.
* Wi-Fi: access point `ESP32go`, LX200 over TCP on port 10001 and the web
  interface on port 80. The station credentials in the image are the upstream
  placeholders.
* Bluetooth is not available in this build.
