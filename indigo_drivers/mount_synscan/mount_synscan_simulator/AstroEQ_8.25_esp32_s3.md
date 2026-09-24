# AstroEQ ESP32-S3 firmware image

`AstroEQ_8.25_esp32_s3.bin` is a prebuilt image of the AstroEQ mount controller,
kept here so the SynScan driver can be tested against the firmware without a
checkout of the controller sources. It is a test asset, not part of any INDIGO
build.

## Upstream

* Project: AstroEQ. The upstream repository and the source commit this image was
  built from were not recorded with the binary and cannot be recovered from it.
  Supply them before relying on this asset. `8.25` in the file name is the
  AstroEQ firmware version.
* The image carries the string `astroeq-s3`, which is the name the build uses for
  its own network interface.
* No licence file accompanies the binary and the upstream licence was not
  established while preparing this note. Check the source-distribution obligation
  before redistributing the binary; omitting the sources does not remove it.

## ESP32-S3 adaptations

Not recorded. What the image itself shows: an Arduino-ESP32 build for ESP32-S3
with the Wi-Fi stack, a DHCP server and the IDF USB-Serial/JTAG VFS linked in.
The application does not use native USB CDC - see Interfaces - so the SynScan
protocol is only on UART0, as in the LX200 ESP32-S3 images.

## Image

* SHA-256: `60b8f2be343d9f3da86b2a2b0c8a95b035fea41a9cf07acee0ea1d80c29d8a49`
* MD5: `796da93f8165e5a3535b5be4f312c738`
* Size: 368000 bytes
* Merged image for offset `0x0`: second stage bootloader at `0x0` (`e9 03 02 3f`),
  partition table at `0x8000` (`aa 50`) and the application at `0x10000`
  (`e9 05 02 3f`). It is not an application-only image and must not be flashed at
  another offset.
* Partition table, read from the image, logical flash size 8 MB:

  | partition | type/subtype | offset | size |
  | --- | --- | --- | --- |
  | `nvs` | 01/02 | `0x009000` | `0x005000` |
  | `otadata` | 01/00 | `0x00e000` | `0x002000` |
  | `app0` | 00/10 | `0x010000` | `0x330000` |
  | `app1` | 00/11 | `0x340000` | `0x330000` |
  | `spiffs` | 01/82 | `0x670000` | `0x180000` |
  | `coredump` | 01/03 | `0x7f0000` | `0x010000` |

Flash it from this directory, with the board's port in `ESP32_PORT`:

```sh
python -m esptool --chip esp32s3 --port "$ESP32_PORT" \
  --baud 460800 write_flash 0x0 AstroEQ_8.25_esp32_s3.bin
```

## Pin configuration

Not recorded, and not recoverable from the image. Supply the step / direction /
enable assignments and the UART0 pins before using this asset for anything beyond
a communication check.

## Interfaces

* Serial: UART0. The bench board the LX200 ESP32-S3 images are used on is an
  ESP32-S3-DevKitC-1, whose UART0 is wired to a second, on-board USB-to-UART
  bridge on its other USB socket; that socket has to be connected to the host for
  the protocol to be reachable. The baud rate of this image is not recorded. The
  SynScan driver opens the port with the framework default `9600-8N1`, and 9600 is
  the Skywatcher/EQMOD convention, so that is the rate to try first. It is
  unverified against this image. Expect the documented DTR/RTS hazard of these
  boards: a host that asserts either line when it opens the port resets the board.
* Native USB (USB-Serial/JTAG, `303a:1001`): the ROM prints its boot log here and
  esptool reaches the board here, but the application never opens it. Verified on
  2026-09-24: after a reset the port yields
  `rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)` through
  `entry 0x403c98d0` and then nothing at all, and a write to it times out because
  the peer never drains the CDC endpoint. Open this port with DTR and RTS
  deasserted: on the ESP32-S3 USB-Serial/JTAG those lines drive EN and BOOT, so
  the default open holds the chip in reset and a firmware that is running looks
  identical to one that is not.
* Wi-Fi: the Wi-Fi stack and a DHCP server are linked in and the image carries the
  name `astroeq-s3`, but no access point of that name was seen while this firmware
  was running on the bench board, so how the network interface is meant to be
  brought up is not established.

## Deployment record

2026-09-24, `indigosky` (Raspberry Pi 5, Debian 12 bookworm, aarch64), esptool
5.4.0 and pyserial 3.5.

* Board, read with `esptool flash-id` before flashing: ESP32-S3 (QFN56) revision
  v0.2, embedded PSRAM 8 MB, 40 MHz crystal, USB mode USB-Serial/JTAG, MAC
  `ac:a7:04:27:b2:58`, flash manufacturer `68` device `4018`, detected size
  **16 MB**, eFuse quad I/O at 3.3 V. The 8 MB the partition table needs therefore
  fits with room to spare.
* Upload: `write-flash 0x0 AstroEQ_8.25_esp32_s3.bin` over `/dev/ttyACM0` at
  460800 baud. Erased `0x0`-`0x59fff`, wrote 368000 bytes (183779 compressed) in
  2.3 s, **`Hash of data verified`**, hard reset via RTS. The whole flash was not
  erased.
* State after the upload: the board boots and reaches the application
  (`SPI_FAST_FLASH_BOOT`, segments loaded, `entry 0x403c98d0`).
* Firmware left installed on the board: this image.
* **Not established: any communication with the firmware, and therefore no driver
  test.** Only the board's native USB port was connected to the host, and the
  application does not serve UART0 there. Connect the DevKitC's USB-to-UART socket,
  or wire an adapter to the UART0 pins, and the SynScan acceptance run can proceed
  from `DEVICE_PORT`. No motors or sensors were attached, so only firmware state
  was validated, not protocol behaviour.
