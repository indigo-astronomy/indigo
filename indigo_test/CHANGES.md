# INDIGO Test Suite Changes

This document records the automated test suite added under `indigo_test/` and the remaining follow-up work. The suite is intentionally hardware-free: it links against the built INDIGO library and simulator driver archives, then exercises public APIs through unit and in-process integration tests.

## Goals

- Provide repeatable tests that run without physical astronomy hardware.
- Keep the harness dependency-free and compatible with the existing make build.
- Separate fast unit tests from simulator and bus-level integration tests.
- Validate driver lifecycle, property enumeration, property changes, and protocol adapters through public APIs.
- Preserve manual hardware validation in `TESTING.md` for real devices and vendor SDK behavior.

## Current Layout

```text
indigo_test/
  CHANGES.md
  Makefile
  test_runner.h
  fixtures/
    protocol/
  unit/
  integration/
```

Unit tests live in `indigo_test/unit/`. Integration tests live in `indigo_test/integration/`. Protocol parser fixtures live in `indigo_test/fixtures/protocol/`.

## Build Targets

- `make -C indigo_test test` runs unit and integration tests.
- `make -C indigo_test test-unit` runs pure unit tests.
- `make -C indigo_test test-integration` runs bus and simulator-driver integration tests.
- `make -C indigo_test test-clean` removes generated test binaries and dSYM files.

Run `make all` from the repository root first if `build/lib/libindigo` or the required simulator driver archives are missing.

## Test Harness

`indigo_test/test_runner.h` provides a small dependency-free C test runner with local assertion macros. Each test executable returns `0` on success, returns non-zero on failure, and prints the failing test and assertion location.

## Unit Coverage

Implemented unit suites:

- `test_aux_math.c`: dewpoint and Bortle-scale helper behavior.
- `test_base64.c`: known vectors, binary round trips, padding, and newline-tolerant decoding.
- `test_bus_helpers.c`: numeric/string conversion, sexagesimal conversion, pixel scale, local service trimming, switch helpers, and property value/target copying.
- `test_bus_property.c`: text, number, switch, light, and BLOB property initialization, matching, copying, resizing, and release behavior, with every test case exercising all vector types.
- `test_dome_azimuth.c`: hour wrapping, azimuth distance, dome azimuth range checks, mirror symmetry of the pivot offsets across the equator, agreement with an independent vector model of the same geometry over a latitude, hour angle, declination and OTA offset sweep, and side of pier handling (the OTA staying on the reported side, no azimuth jump when a mount tracks past the meridian, agreement with counterweight down for a normal mount).
- `test_md5.c`: known MD5 vectors, partial MD5, and file-prefix MD5.
- `test_polynomial_fit.c`: polynomial value, derivative, extrema, minimum search, string output, and exact line fitting.
- `test_protocol_json.c`: JSON escaping, device/server adapter serialization, parser routing for number and switch changes, BLOB URL output, and malformed input handling.
- `test_protocol_xml.c`: XML escaping, device and client adapter serialization, parser routing for text changes and BLOB URL mode, remote property events, and malformed input handling.
- `test_raw_image.c`: RAW type constants, Bayer extension detection, Bayer channel equalization, saturation masks, and contrast.
- `test_timer.c`: timer delay conversion, callback execution, data callbacks, mutex-wrapped callbacks, cancel/reschedule behavior, device-wide timer cancellation, queue priority ordering, queue removal, and queue deletion.
- `test_token.c`: token parsing, device token add/update/remove, and master-token fallback.

## Integration Coverage

Implemented bus and simulator integration suites:

- `test_bus_lifecycle.c`: `indigo_start()` / `indigo_stop()`, client and device attach/detach, enumeration, property definition/update/delete delivery, change routing, and invalid lifecycle calls.
- `test_ccd_simulator.c`: CCD simulator driver metadata, imager device lifecycle, full visible/hidden imager property sets, connection/disconnection, short exposure checks, and compliance checks for exposed imager, wheel, focuser, guider camera, guider, AO, Bahtinov camera, DSLR, and file-camera devices.
- `test_dome_baader_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Baader Classic Dome driver.
- `test_dome_beaver_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome Beaver driver, including azimuth motion, shutter, park/home, calibration, abort, and Beaver safety/failure properties.
- `test_dome_nexdome_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome driver, including shutter, azimuth motion, abort, reverse-direction, and NexDome status/control properties.
- `test_dome_nexdome3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome 3 firmware driver, including shutter, azimuth motion, abort, slaving threshold, and NexDome 3 status/settings properties.
- `test_dome_skyroof_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Interactive Astronomy SkyRoof driver, including shutter, abort, and heater control.
- `test_dome_talon6ror_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Talon6 ROR driver, including roof open/close/abort handling and Talon6 sensor/configuration properties.
- `test_dome_simulator.c`: dome simulator metadata, lifecycle, property enumeration, connection/disconnection, visible and hidden properties, dome property item/range checks, shutter/slaving/park commands, azimuth motion, and abort handling.
- `test_gps_simulator.c`: GPS simulator metadata, lifecycle, property enumeration, connection/disconnection, GPS property item/range checks, status lights, and advanced-status coverage.
- `test_gps_nmea_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free NMEA 0183 stream parsing for the Generic NMEA GPS driver, including selected positioning system, coordinates, UTC time, 3D fix status, satellite counts, and advanced DOP/status updates.
- `test_mount_ioptron_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, one configurable iOptron protocol simulator covering HC 8406, HC 8407, protocol 1.0, 2.0, 2.5, and 3.0 dialect connection paths, plus protocol 3.0 mount and guider property/action coverage.
- `test_mount_lx200_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, configurable LX200/OnStep protocol simulator coverage for the LX200 mount, guider, focuser, and OnStep AUX logical devices.
- `test_mount_nexstar_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, configurable NexStar protocol simulator coverage for both Celestron and Sky-Watcher dialect connection paths, plus Celestron mount and guider property/action coverage.
- `test_mount_nexstaraux_simulator.c`: external TCP simulator launch, ready-file discovery using a `nexstar://` loopback URL, and hardware-free NexStar AUX binary protocol coverage for mount and guider logical devices.
- `test_mount_pmc8_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free PMC-Eight serial protocol coverage for mount and guider logical devices.
- `test_mount_rainbow_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free RainbowAstro serial protocol coverage for mount lifecycle, identity, guide-rate, tracking, track-rate, coordinate, and abort paths.
- `test_mount_simulator.c`: mount simulator metadata, main mount and guider-device lifecycle, property enumeration, connection/disconnection, mount compliance checks, and guider compliance checks.
- `test_polaralign_simulator.c`: polar aligner simulator metadata, lifecycle, property enumeration, connection/disconnection, property item/range checks, direction commands, offset no-op handling, reset commands, and abort command handling.
- `test_rotator_simulator.c`: rotator simulator metadata, lifecycle, property enumeration, connection/disconnection, and rotator compliance checks.
- `test_focuser_astromechanics_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the ASTROMECHANICS focuser driver, including position/aperture property coverage and the aperture command path.
- `test_focuser_askar_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the Askar-WAF focuser driver.
- `test_focuser_efa_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Celestron / PlaneWave EFA focuser driver, including position sync/goto, relative steps, fan control, temperature, and abort.
- `test_focuser_dmfc_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the PegasusAstro DMFC focuser driver, including speed, backlash, reverse motion, motor type, encoder/LED controls, position sync/goto, relative steps, temperature, and abort.
- `test_focuser_dsd_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Deep Sky Dad AF focuser driver, including AF2 speed, backlash, reverse motion, step mode, coils mode, current/timing controls, temperature, position sync/goto, relative steps, and abort.
- `test_focuser_focusdreampro_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the AstroGadget FocusDreamPro focuser driver, including speed, duty cycle, position sync/goto, relative steps, temperature, and abort.
- `test_focuser_ioptron_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the iOptron iEAF focuser driver, including status polling, reverse motion, absolute and relative moves, abort, zero sync, and temperature.
- `test_rotator_falcon2_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the Falcon2 rotator driver.
- `test_wheel_xagyl_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Xagyl filter wheel driver.
- `test_wheel_quantum_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Brightstar Quantum filter wheel driver.
- `test_wheel_trutek_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Trutek filter wheel driver.
- `test_wheel_qhy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the QHY CFW1, CFW2, and CFW3 filter wheel driver modes.
- `test_wheel_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Optec filter wheel driver.
- `test_focuser_fc3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the FocusCube 3 focuser driver.
- `test_focuser_qhy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the QHY Q-Focuser driver, including mode/compensation settings, position sync/goto, relative moves, speed, reverse motion, temperature, limits, and abort.
- `test_focuser_optecfl_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for both Optec FocusLynx focuser channels, including focuser type, reverse motion, position sync/goto, relative moves, temperature, and abort.
- `test_focuser_lacerta_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the LACERTA Motorfocus focuser driver.
- `test_focuser_lakeside_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the LakesideAstro focuser driver, including temperature polling, relative moves, abort, backlash, mode, compensation settings, and active slope.
- `test_focuser_mjkzz_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and binary-frame serial compliance checks for the MJKZZ Rail focuser driver, including speed, absolute position, relative movement, and abort.
- `test_focuser_moonlite_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the MoonLite focuser driver, including speed, stepping mode, compensation, mode, temperature polling, absolute and relative moves, and abort.
- `test_focuser_nfocus_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Rigel Systems nFOCUS focuser driver, including speed, temperature, relative moves, and abort.
- `test_focuser_nstep_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Rigel Systems nSTEP focuser driver, including speed, stepping mode, phase wiring, backlash, compensation mode, temperature, relative moves with position polling, and abort.
- `test_focuser_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Optec TCF-S focuser driver, including manual/automatic mode changes, compensation, reverse-motion state, temperature, and relative moves with position polling.
- `test_focuser_primaluce_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free JSON serial driver connection, and per-device compliance for both PrimaLuceLab logical devices — the focuser device (configuration/state/WiFi/LED/preset/hold-current controls, backlash, speed, absolute and relative moves, temperature, and abort) and the connection-sharing rotator device (property coverage and abort).
- `test_focuser_prodigy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for both PegasusAstro Prodigy logical devices — the focuser device (speed, backlash, park, temperature, position sync/goto, relative moves, and abort) and the powerbox device (power outlets, USB ports, outlet naming, and reboot).
- `test_focuser_robofocus_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free fixed-frame serial driver connection, and serial compliance checks for the RoboFocus driver, including power channels, configuration/backlash, temperature, limits, absolute and relative moves, reverse motion, and abort.
- `test_focuser_steeldrive2_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free SteelDrive II serial driver connection with CRC responses, and per-device compliance for both logical devices — the focuser device (saved values, temperature-compensation controls, end-stop/zeroing controls, position sync/goto, relative moves, temperature, and abort) and the connection-sharing AUX heater device (heater output, auto-dew/PID controls, PID settings, and sensor selection).
- `test_focuser_usbv3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free compact-command serial driver connection, and serial compliance checks for the USB_Focus v3 driver, including step size, compensation, mode, speed, temperature, limits, absolute and relative moves, and abort.
- `test_focuser_wemacro_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free binary-frame serial driver connection, and serial compliance checks for the WeMacro Rail driver, including rail configuration, shutter fire, relative movement, batch execution, speed, reverse motion, and abort.
- `test_aux_svbpowerbox_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the SVBONY PowerBox AUX driver.
- `test_aux_wbplusv3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the WandererBox Plus V3 AUX driver.
- `test_aux_wbprov3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the WandererBox Pro V3 AUX driver.
- `test_aux_wcv4ec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, lightbox property enumeration completeness, and compliance checks for the WandererCover V4-EC AUX lightbox driver.
- `test_aux_arteskyflat_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, lightbox property enumeration completeness, and compliance checks for the Artesky Flat Box AUX lightbox driver.
- `test_aux_astromechanics_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, SQM weather property enumeration completeness, and timer-driven `V#` reading compliance for the ASTROMECHANICS LPM AUX driver.
- `test_aux_fbc_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `: I #`/`: P #`/`: V #` handshake, lightbox property enumeration completeness, and light-intensity change compliance for the Lacerta FBC AUX driver.
- `test_aux_flatmaster_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `#`/`V` handshake, lightbox property enumeration completeness, and light switch and intensity change compliance for the Pegasus Astro FlatMaster AUX driver.
- `test_aux_flipflat_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `>POOO`/`>VOOO` handshake, lightbox and cover property enumeration completeness, and light, intensity, and timed cover-open (BUSY-to-OK via `>SOOO` status polling) compliance for the Optec/Alnitak Flip-Flat AUX driver.
- `test_ao_sx_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the character-framed `X`/`V` handshake, and per-device compliance for both logical devices of the StarlightXpress AO driver — the AO device (tip/tilt pulse and reset) and the connection-sharing guider device (guider pulse).
- `test_mount_synscan_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for both logical devices of the SynScan EQ8 driver — the mount device (property completeness and representative changes) and the connection-sharing guider device (guide pulse).
- `test_aux_upb3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for both logical devices of the Ultimate Powerbox 3 driver — the AUX device (power, USB, heater, dew control) and the connection-sharing focuser device (position move).
- `test_aux_ppb_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-model compliance for all three PegasusAstro Pocket Powerbox variants — PPB (power outlets, heater outlets, dew control), PPBA (same plus DSLR power selection), and SPB (single power outlet, heater outlets, dew control) — using the `--model ppb|ppba|spb` flag.
- `test_aux_skyalert_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `\r`-terminated `send` command, and property enumeration completeness for the Interactive Astronomy SkyAlert AUX weather driver.
- `test_aux_sqm_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `x`-terminated `ix` handshake, timer-driven `rx` reading compliance, and property enumeration completeness for the Unihedron SQM AUX driver.
- `test_aux_uch_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `P#`/`PV`/`PL:1`/`PA` handshake, USB port toggle compliance, and property enumeration completeness for the PegasusAstro USB Control Hub AUX driver.
- `test_aux_usbdp_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `SWHOIS`/`SGETAL` handshake (6-byte fixed-length commands, `\n`-terminated responses), and per-version compliance for both USB Dewpoint variants — V2 (three controllable heater outlets, dew control mode, calibration, thresholds, channel linking, aggressivity, weather, and dual temperature sensors) and V1 (weather and single temperature sensor only) — selected via `--model v1|v2`.

`integration/simulator_test_common.h` provides the shared in-process client, property cache, lifecycle helpers, and compliance-style assertions used by simulator tests.

## Compliance Model

The simulator compliance checks are derived from the shell-based routines in `indigo_tests/`. They verify interface bitmasks, mandatory class properties and items, numeric ranges, connection/disconnection paths, representative state transitions, and restoration of changed values where practical.

Coverage currently includes:

- GPS compliance on the GPS simulator.
- Mount compliance on the mount simulator.
- Guider compliance on the mount simulator guider and CCD simulator guider.
- Rotator compliance on the rotator simulator.
- Wheel, focuser, guider, and AO compliance through devices exposed by the CCD simulator.
- CCD camera compliance through the CCD simulator's imager, guider camera, Bahtinov camera, DSLR, and file-camera devices.
- Dome compliance on the dome simulator.
- Polar-aligner compliance on the polar-aligner simulator.
- External serial simulator compliance on Baader Classic Dome, NexDome Beaver, NexDome, NexDome 3, Interactive Astronomy SkyRoof, Talon6 ROR, Generic NMEA 0183 GPS, iOptron mount/guider, ASTROMECHANICS focuser, Askar-WAF focuser, Celestron / PlaneWave EFA focuser, AstroGadget FocusDreamPro focuser, Falcon2 rotator, Xagyl filter wheel, Brightstar Quantum filter wheel, Trutek filter wheel, QHY CFW1/CFW2/CFW3 filter wheel modes, Optec filter wheel, FocusCube 3 focuser, QHY Q-Focuser, Optec FocusLynx focuser, Optec TCF-S focuser, PrimaLuceLab focuser/rotator, PegasusAstro Prodigy focuser/powerbox, RoboFocus focuser, SteelDrive II focuser/AUX, USB_Focus v3 focuser, WeMacro Rail focuser, LACERTA Motorfocus focuser, MoonLite focuser, myFocuserPro2 focuser, Rigel Systems nFOCUS focuser, Rigel Systems nSTEP focuser, SVBONY PowerBox AUX, WandererBox Plus V3 AUX, WandererBox Pro V3 AUX, WandererCover V4-EC AUX, Artesky Flat Box AUX, ASTROMECHANICS LPM AUX, Lacerta FBC AUX, Pegasus Astro FlatMaster AUX, Optec/Alnitak Flip-Flat AUX, StarlightXpress AO, SynScan EQ8 mount, Ultimate Powerbox 3 AUX, PegasusAstro Pocket Powerbox PPB/PPBA/SPB AUX, Interactive Astronomy SkyAlert AUX, Unihedron SQM AUX, PegasusAstro USB Control Hub AUX, and USB Dewpoint v1/v2 AUX drivers through pseudo terminals exported with `INDIGO_SIMULATOR_PORT`.

Standalone simulator archives are not present for every device class in this checkout, so some class coverage intentionally uses multi-device simulator drivers.

## Current Behavior

- The suite is hardware-free.
- Tests do not launch `indigo_server` or open network sockets.
- Protocol tests use fixed fixtures and temporary files through `indigo_uni_io`.
- Integration tests exercise public driver entry points and bus APIs; they do not include production `.c` files directly.

## Verification

Last verified locally on macOS:

- `make -C indigo_test test-unit`
- `make -C indigo_test test-integration`
- `make -C indigo_test test`
- `make -C indigo_test test-clean`

All tests passed at the time this document was cleaned up.

## Deferred Work

- Add binary fixtures for `indigo_dslr_raw.c`, `indigo_fits.c`, `indigo_tiff.c`, `indigo_avi.c`, and `indigo_ser.c`.
- Add carefully sourced expected values for `indigocat` coordinate, precession, and time transforms.
- Add slower live `indigo_server` socket tests with process management, port allocation, and strict timeouts.
- Add driver-specific fake I/O tests for hardware drivers with complex parsing or command sequencing.
- Wire a root-level `make test` target once the suite is accepted as part of the normal project workflow.
- Update `README.md` and `TESTING.md` with the finalized automated test commands and their relationship to manual hardware testing.
