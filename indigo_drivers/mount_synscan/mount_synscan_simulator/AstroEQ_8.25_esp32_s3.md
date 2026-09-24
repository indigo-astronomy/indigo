# AstroEQ ESP32-S3 firmware image

`AstroEQ_8.25_esp32_s3.bin` is a prebuilt image of the AstroEQ mount controller,
kept here so the SynScan driver can be tested against the firmware without a
checkout of the controller sources. It is a test asset, not part of any INDIGO
build.

## Upstream

* Project: AstroEQ, https://github.com/TCWORLD/AstroEQ
* Source commit: `2a91570c42c6816b4123fa337542e7b1e3992892`, the upstream
  "Compile V8.25 Firmware Release". `8.25` in the file name is that version, and
  the firmware reports it over the protocol as `825` (`:e1` answers `=390300`,
  which is `0x000339`); the SynScan driver renders the same value as its
  conventional `57.03`.
* Licence: MIT, Copyright (c) 2012 Thomas Carpenter, `License.txt` in the
  upstream tree. Redistributing this binary is permitted and carries no
  source-distribution obligation; keep the copyright notice with it.

## ESP32-S3 adaptations

Ported from the AVR original; the protocol parser, command handling,
acceleration, GoTo and stepping logic are upstream's. `src/s3_hal.cpp` in the
port replaces AVR GPIO, UART, EEPROM and timers:

* Two hardware timers at 16 MHz (80 MHz / 5), matching AVR's unprescaled clock,
  keeping the original two-toggle step logic and fractional timing.
* AVR `unsigned int` storage replaced by `uint16_t`, including the EEPROM table.
* AVR register state held in explicit arrays; the AVR nibble swap replaced.
* AVR atomic sections become a FreeRTOS mux shared with the timer callbacks.
* EEPROM is a 512-byte NVS namespace `astroeq-s3`; pending writes are committed
  before the configuration protocol's restart, with motors stopped.
* The original infinite main loop yields to FreeRTOS.
* Absent peripherals use sentinel 255: ST4, SNAP, status LED, polar illuminator
  and hand controller. Hand-controller detection is forced to EQMOD mode.

Two changes were made on top of that port while validating it on hardware:

* **The protocol now runs on the native USB CDC** (`ARDUINO_USB_CDC_ON_BOOT=1`,
  `ARDUINO_USB_MODE=1`), with the UART0 path kept under the same `#if` so the
  source still builds for either wiring. UART0 reaches a host only through the
  DevKitC's second USB socket; with only the native socket cabled a UART0-only
  build runs perfectly and is unreachable. `ARDUINO_USB_MODE=1` keeps the
  USB-Serial/JTAG identity the board already enumerates with, so esptool and the
  device node are unchanged. The CDC write timeout is set to zero so a host that
  stops reading cannot block the motor loop.
* **Error replies now carry their terminator.** `synta_decodeCommand()`'s error
  path wrote the NUL at `dataPacket[3]` and the `\r` at `dataPacket[4]`, and the
  reply is sent with `Serial_writeStr()`, which is `Serial.print()` and stops at
  the NUL - so every rejected command answered `!0<code>` with no terminator and
  left the host waiting for `\r` until its own read timeout. The two are now in
  the order `synta_assembleResponse()` already uses for the same packet shape.
  **This defect is byte-identical in upstream `2a91570` and is worth reporting
  upstream**; it was not introduced by the ESP32-S3 port.

## Image

* SHA-256: `aeddf03a7ff2bc8d198603c862d90c882cdaee0495935370f28723cf84dba714`
* MD5: `ebee4827a7e3c3e273fddf5d58b88117`
* Size: 352944 bytes
* Merged image for offset `0x0`: second stage bootloader at `0x0`, partition
  table at `0x8000`, `boot_app0` at `0xe000` and the application at `0x10000`.
  It is not an application-only image and must not be flashed at another offset.
* Partition layout `default_8MB.csv`, logical flash size 8 MB, DIO at 80 MHz:

  | partition | type/subtype | offset | size |
  | --- | --- | --- | --- |
  | `nvs` | 01/02 | `0x009000` | `0x005000` |
  | `otadata` | 01/00 | `0x00e000` | `0x002000` |
  | `app0` | 00/10 | `0x010000` | `0x330000` |
  | `app1` | 00/11 | `0x340000` | `0x330000` |
  | `spiffs` | 01/82 | `0x670000` | `0x180000` |
  | `coredump` | 01/03 | `0x7f0000` | `0x010000` |

* Built with PlatformIO, platform `espressif32@7.1.3`, Arduino core 2.0.17,
  board `esp32-s3-devkitc-1`. The bootloader and partition table are bit-identical
  to the previous image; `firmware.bin` is not reproducible because the IDF embeds
  the compile time in the application description.

Flash it from this directory, with the board's port in `ESP32_PORT`:

```sh
python -m esptool --chip esp32s3 --port "$ESP32_PORT" \
  --baud 460800 write_flash 0x0 AstroEQ_8.25_esp32_s3.bin
```

## Pin configuration

Bench GPIO map, step / direction / enable / reset:

* RA: 4 / 5 / 6 / 8, microstep mode pins 13, 14, 15
* DEC: 10 / 11 / 12 / 9, microstep mode pins 16, 17, 18

Only erased storage is seeded, with a bench model that is **an example, not the
calibration of a real mount**: A4988, 16 microsteps, 200 motor steps * 144
reduction = 460800 counts per axis, bVal 3209, sidereal IVal 600, 3200 steps per
worm revolution, no gear change. Use the original configuration protocol to set
real mount parameters before connecting motors.

## Interfaces

* Serial: the native USB Serial/JTAG CDC, `303a:1001`, `/dev/ttyACM0` on Linux.
  It carries no line settings; the SynScan driver's default `9600-8N1` is
  accepted and ignored. **Open it with DTR and RTS deasserted**: on the ESP32-S3
  those lines drive EN and BOOT, so the default open holds the chip in reset and
  a running firmware looks identical to a dead one. A host that resets the board
  will find the ROM boot log in the input buffer before the first reply.
* UART0 at 9600 baud 8N1 on GPIO43 (TX) / GPIO44 (RX) is still supported by the
  source, selected by building with `ARDUINO_USB_CDC_ON_BOOT=0`. It needs the
  DevKitC's second USB socket connected.
* Wi-Fi: the stack is linked in and the image carries the name `astroeq-s3`, but
  no access point of that name appears while the firmware runs, so the network
  interface is not brought up in this configuration.
* No ST4, SNAP, status LED, polar illuminator or hand controller in this build.

## Deployment record

2026-09-24, `indigosky` (Raspberry Pi 5, Debian 12 bookworm, aarch64), esptool
5.4.0, pyserial 3.5, PlatformIO 6.2.0.

* Board, from `esptool flash-id` before flashing: ESP32-S3 (QFN56) revision v0.2,
  embedded PSRAM 8 MB, 40 MHz crystal, USB mode USB-Serial/JTAG, MAC
  `ac:a7:04:27:b2:58`, flash manufacturer `68` device `4018`, detected size
  **16 MB**. The 8 MB the partition table needs fits with room to spare.
* Upload: `write-flash 0x0` over `/dev/ttyACM0` at 460800 baud, 352944 bytes,
  **`Hash of data verified`**, hard reset. The whole flash was not erased.
* Firmware left installed on the board: this image, with both axes commanded to
  stop (`:K1`, `:K2`, both axes then reporting `=100`).

### Protocol validation

Read-only inquiries all answer correctly and consistently with the seeded bench
configuration: `:e1`/`:e2` `=390300` (825), `:a1` 460800 counts per revolution,
`:s1` 3200 steps per worm revolution, `:j1`/`:j2` `0x800000` at the home centre,
`:f1`/`:f2` well-formed, and `:q1` - which AstroEQ 8.25 does not implement -
`!01\r`, with the terminator the fix above restored.

### SynScan driver acceptance, `make -C indigo_test test-mount-synscan-hw`

`SYNSCAN_HW_URL=/dev/ttyACM0`, driver `indigo_mount_synscan`, 7 of 16 cases
passed and the run ended in a crash, so **this is not a passing hardware run**:

* Passed: identity and capabilities (reported as `Sky-Watcher SynScan EQ6,
  firmware 57.03`), coordinate and state readback, tracking and rates, park and
  unpark, home, and the two cases that are not applicable to this controller
  (autohome, snap port).
* `synscan_discovers_and_connects` fails on an assertion that `DEVICE_PORT` was
  rewritten to a `synscan://` address. That assertion only holds for the UDP
  transport the suite was written for; it is a test limitation, not a defect of
  the firmware or the driver.
* **Driver defect, not firmware, fixed in driver version 8 (SYNSCAN-D01): the process died with
  SIGSEGV** in
  `guider_guide_dec_finalizer` ->  `synscan_stop_axis_and_wait` ->
  `synscan_wait_axis_stopped` -> `synscan_update_mount_coordinates` ->
  `synscan_read_mount_coordinates`, at
  `indigo_mount_synscan.c:978`, which reads `MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY`.
  That macro resolves through `MOUNT_CONTEXT`, i.e. `device->device_context`, but
  the finalizer passes the **guider** logical device, whose context is an
  `indigo_guider_context`. The read is type-confused. The mount-coordinate helpers
  need the master device. The wire trace shows the firmware answering normally up
  to the moment of the crash.
* **Open, unattributed: a GoTo never completes.** The firmware accepts the whole
  sequence (`:K`, `:f`, `:j`, `:G`, `:H`, `:M`, `:J`) and does step - RA moved
  from `0x800051` to `0x7F90DB`, about 3958 counts, in the two minutes before the
  timeout - but that is roughly 33 counts per second, so a 96076-count slew would
  need about 48 minutes. Three cases fail on it, and after the crash was fixed the later cases cascade
  from the same timeout, so the rerun on driver version 8 stands at 5 of 16 with no crash. The axes also never set the
  "initialised" bit in `:f`, although `:F1` and `:F2` were accepted at connect.
  The step rate wants investigating in `s3_hal.cpp`'s timer path and in the
  `:I`-driven speed handling before this image is used for motion; the bench
  parameters above are examples and are not a calibration.

No motors or sensors were attached, so nothing here validates physical motion.
