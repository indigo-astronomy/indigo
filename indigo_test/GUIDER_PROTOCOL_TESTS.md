# Guider protocol and SDK tests

Production driver sources remain unchanged. All three standalone guider modules now have a hardware-free boundary: a real PTY simulator for CG-USB-ST4, a vendor SDK fake for ASI USB-ST4, and the existing fake USB/SDK suite for GPUSB.

## Sources and limits

- ASI: the bundled vendor header `indigo_drivers/guider_asi/bin_externals/libusb2st4conv/include/USB2ST4_Conv.h` is the primary contract for stable device IDs, lifecycle errors and independent direction ON/OFF calls. The fake does not invent automatic opposite-direction clearing. Behavior of physical relays on SDK Close is unspecified and is not asserted. No vendor binary or real USB enumeration is used.
- CG-USB-ST4: the historical manufacturer page `http://www.astrogene1000.com/products/guide_output_web_1/guide_out.htm` could not be retrieved. [Upstream PHD2 at commit c406cf2b2de51cbb7e3ed76165c9accf33edad2d](https://github.com/OpenPHDGuiding/phd2/blob/c406cf2b2de51cbb7e3ed76165c9accf33edad2d/src/scope_GC_USBST4.cpp) is a supplementary implementation: handshake byte 0x06 expects `A`, guide commands use numeric directions 0–3 and a four-character millisecond field. INDIGO instead uses letters n/s/e/w. Both simulator profiles are explicit; neither resolves whether hardware supports both. An automatic same-axis replacement in the simulator is a model assumption, not independently verified firmware behavior.
- GPUSB: retained the existing SDK model and tests in `integration/test_usb_outputs.c`; no new serial protocol was invented for a USB-only device.

## Build and run

From `indigo_test/` after the root libraries/archives exist:

```sh
make build/integration/test_guider_asi_sdk build/integration/test_guider_cgusbst4_simulator build/integration/test_guider_gpusb_usb
./build/integration/test_guider_asi_sdk
./build/integration/test_guider_cgusbst4_simulator
./build/integration/test_guider_gpusb_usb
```

All are included in `test-integration`. On macOS, the ASI test builds the supported x86_64 implementation and therefore requires Rosetta on Apple Silicon. The production arm64 branch intentionally returns unsupported; its architecture guard is not modified or disabled. Linux uses its normal target architecture. SDK and hot-plug APIs are substituted only when compiling the test driver object; the actual bus, timers and property handlers are used.

ASI cases run in separate bounded child processes so a failed lifecycle cannot contaminate later scenarios. `ASI_TEST_FILTER` selects a case by name substring. CG-USB-ST4 uses a separately launched simulator process and the existing ready-file contract. Its parent removes the event log and stops the simulator after each case, including a crashing test child.

The standalone simulator is at `indigo_drivers/guider_cgusbst4/guider_cgusbst4_simulator/guider_cgusbst4_simulator.c`. Options: `--headless --ready-file PATH --profile phd2|indigo|wrong-identity|silent`. Default is `phd2`. Commands drive independently expiring pulse state; `PATH.events` records command acceptance/rejection and ON/OFF events using a monotonic clock. This file is test observation only and adds no invented commands to the device protocol.

## Recorded validation (2026-09-09)

| Driver | Results | Remaining scope |
| --- | --- | --- |
| `guider_asi` | 3/9 pass: four directions and cross-axis pulses, failed-open retry, removal. Six failing scenarios reproduce five findings DRV-090–DRV-094: ON/OFF errors, reversal, zero stop, failed INIT retry, pending arrival after SHUTDOWN. | Multi-device/capacity, failed ID enumeration/attach, detailed active-disconnect behavior and repeated timing benchmark. ON/OFF entry timestamps are collected by the SDK fake but no timing-precision claim is made. |
| `guider_cgusbst4` | 4/5 pass: all four directions/expiration/value reset with the explicit INDIGO dialect, wrong identity, silent identity, reconnect. Numeric PHD2 dialect rejects INDIGO's letter commands (DRV-095, needs manufacturer confirmation). | Resolve protocol dialect, overlapping/replaced pulses, transport loss, additional instances and full timing benchmark. |
| `guider_gpusb` | Existing four groups pass: lifecycle/discovery/duplicate/open rollback/removal, direction mapping/completion/errors, INIT/attach rollback, replacement and stop/error handling. | Previously recorded timing/capacity audit remains open. |

Known failures remain nonzero; no production fixes or expected-failure suppression were added. These results establish partial coverage, not complete standard coverage or hardware acceptance. Run `make test-clean` after validation.
