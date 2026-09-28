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

Three changes were made on top of that port while validating it on hardware:

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
* **The seeded bench configuration is a usable mount.** The first image seeded
  460800 counts per revolution, gear change off, GoTo speed 12 and an acceleration
  table from speed 300 to 12 in steps of 5 with 4 repeats per entry. That capped
  every GoTo at 267 counts per second - 0.21 degrees per second - after a 20 to
  25 s ramp, so a 30 degree slew took over two and a half minutes. The coarse
  resolution also made every stop slow: AstroEQ stops an axis only once the step
  in progress and one more have completed, which at the 50 % guide rate on that
  axis was up to 0.56 s, longer than any guide pulse test tolerates. The seed now
  models five times the resolution (2304000 counts, sidereal IVal 120), enables
  gear change so a high-speed GoTo steps in the 1/2 microstep mode, uses GoTo
  speed 2, and fills the table geometrically from 60 (twice sidereal) to 2 over
  its 64 entries with no repeats. A GoTo now runs at about 10600 counts per
  second, 1.7 degrees per second, after a ramp of about 1 s, and a stop at the
  guide rate completes within about 0.11 s. Only the seed changed; stepping,
  timers and the acceleration logic are upstream's.

## Image

* SHA-256: `18d234390dd3b6d5c7d63966d9a451632b780890d97d2fa78433926b24aa5ff1`
* MD5: `a147faa6b85f0db7537772314c46acdb`
* Size: 357536 bytes
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

Flash it from this directory, with the board's port in `ESP32_PORT`. The bench
configuration is seeded only into erased storage, so erase the `nvs` partition
first when the board already ran an earlier image; otherwise the old
configuration survives the upload:

```sh
python -m esptool --chip esp32s3 --port "$ESP32_PORT" erase-region 0x9000 0x5000
python -m esptool --chip esp32s3 --port "$ESP32_PORT" \
  --baud 460800 write_flash 0x0 AstroEQ_8.25_esp32_s3.bin
```

## Pin configuration

Bench GPIO map, step / direction / enable / reset:

* RA: 4 / 5 / 6 / 8, microstep mode pins 13, 14, 15
* DEC: 10 / 11 / 12 / 9, microstep mode pins 16, 17, 18

Only erased storage is seeded, with a bench model that is **an example, not the
calibration of a real mount**: A4988, 16 microsteps, 200 motor steps through a
5:1 gearbox onto a 144-tooth worm = 2304000 counts per axis, bVal 3209, sidereal
IVal 120, 16000 steps per worm revolution, gear change to 1/2 microstep at high
speed (`:g` answers 8), GoTo speed 2, and an acceleration table falling
geometrically from 60 to 2 with no repeats. Use the original configuration
protocol to set real mount parameters before connecting motors.

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
* First upload: `write-flash 0x0` over `/dev/ttyACM0` at 460800 baud, the
  352944-byte image with the original bench model, **`Hash of data verified`**.
* Current image, the same evening: `erase-region 0x9000 0x5000` so the new bench
  model is seeded, then `write-flash 0x0` of this 357536-byte image at 460800
  baud, **`Hash of data verified`**, hard reset. The rest of the flash was not
  erased.
* Firmware left installed on the board: this image, with both axes stopped.

### Protocol validation

Read-only inquiries all answer correctly and consistently with the seeded bench
configuration: `:e1`/`:e2` `=390300` (825), `:a1` 2304000 counts per revolution,
`:b1` timer frequency 3222, `:s1` 16000 steps per worm revolution, `:g1` 8,
`:j1`/`:j2` `0x800000` at the home centre, `:f1`/`:f2` well-formed, and `:q1` -
which AstroEQ 8.25 does not implement - `!01\r`, with the terminator the fix
above restored. After `:F1` the axis reports `=101`, initialised.

Motion measured directly over the protocol, sampling `:j1`: a low-speed slew at
`:I` 600 moves 5.4 counts per second and at `:I` 12 moves 262; a high-speed slew
at `:I` 12 moves 2010 over an 8 s window that includes the ramp; a 100000-count
high-speed GoTo finishes within 10 s at 100008, 8 counts past the target, and
reports `=501`, stopped.

Three properties of the upstream firmware that a host sees and that are not port
defects:

* **Nothing moves before `:F`.** `:F` runs `motorEnable()`, which configures the
  step timers; before it an accepted `:J` reports the axis running (`=110`)
  while it never steps. The SynScan driver always sends `:F` at connect.
* **`:F` on one axis resets both step timers**, as upstream's `configureTimer()`
  does on AVR, so an `:F2` sent while RA is moving leaves RA reporting running
  without stepping. The driver only sends `:F` at connect, with both axes stopped.
* **A stop finishes the step in progress and one more.** `:K` at a slow rate
  therefore takes up to two step periods. That is what made the original,
  coarse bench model fail every guide timing check, and why the current one has
  five times the resolution.

### SynScan driver acceptance, `make -C indigo_test test-mount-synscan-hw`

`SYNSCAN_HW_URL=/dev/ttyACM0`, driver `indigo_mount_synscan` version 8, on the
current image: **16 of 16 cases passed, exit status 0**, run finished
2026-09-24 19:26 UTC. The controller identifies as `Sky-Watcher SynScan EQ6,
firmware 57.03`, with a polarscope and no encoders, PPEC, home indexer or snap
port.

* The slews to the test pointing at declination 60 from the pole, SYNC and a
  3 degree slew, abort and recovery, park, unpark and home all complete; the
  slew arrived within 0.0003 degrees in declination.
* Tracking for 12 s moved the reported RA by -0.00002 degrees; an idle axis
  moved 0.0458 against the sidereal 0.050.
* Guiding: eight 1000 ms pulses each way at the 50 % guide rate differ by
  0.0378 degrees against the expected 0.033. Pulse completion overruns by a
  constant +300 ms on RA and +150 ms on DEC across 100 to 2000 ms: the driver
  stops the axis and polls it stopped at 100 ms intervals before it reports the
  pulse done, twice on RA where tracking is resumed. The overlapping-pulse cases
  finish in 1251, 1301 and 1151 ms.
* Autohome and the snap port are not applicable to this controller. The driver
  refuses the aux device because there is no snap port, and the reconnect cases
  confirm that refusal is clean and leaves the shared connection working.

The earlier runs on this board are superseded. The first image stood at 7 of 16
with a crash and then 5 of 16: the crash was a driver defect, fixed in driver
version 8 (SYNSCAN-D01) - `guider_guide_dec_finalizer` passed the guider device
to the mount-coordinate helpers, which read `MOUNT_GEOGRAPHIC_COORDINATES_PROPERTY`
through the wrong device context. Every GoTo then timed out. That was reported as
an unexplained 33 counts per second and a missing initialised bit; both readings
were wrong. The firmware stepped exactly as configured, the 267 counts per
second ceiling and the slow ramp of the original bench model above made every
slew outlast the test's 120 s motion timeout, and `:f` does report the axis
initialised after `:F`. With gear change enabled the image reached 10 of 16;
the remaining failures were the guide timings, fixed by the resolution of the
current model, and two limitations of the test, fixed in the test: it expected a
`synscan://` port where a serial device was given, and it expected the aux
device to connect on a controller without a snap port.

No motors or sensors were attached, so nothing here validates physical motion.
