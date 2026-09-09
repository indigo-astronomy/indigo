# Celestron / PlaneWave EFA focuser driver

## Supported devices

- Celestron Focus Motor for SCT and EdgeHD
- PlaneWave EFA control unit

One `EFA Focuser` is present at startup; additional independent serial instances can be configured. INFO identifies the actual model. Connect through the PC/USB serial port at 19200 baud. AUX-port connection is not supported. Generated port-selection defaults replace the old platform-specific default path.

## Controls

Both models support absolute/relative integer movement and abort. Measured position is separate from target; no-op completes immediately. PlaneWave moves farther than 50000 encoder counts use coarse slew, stop, then fine GOTO. Handler/finalizer scheduling leaves abort/disconnect available during movement and calibration. Failed replies/ACKs and invalid completion state report ALERT; 100 unchanged motion polls trigger a stop attempt. After uncertain motion, use abort to confirm the stopped position before another move.

PlaneWave exposes coordinate SYNC, `X_FOCUSER_FANS` and temperature. `FOCUSER_LIMITS` sets both local software endpoints (default 0–3799422); these limits constrain driver movement and update property ranges. No undocumented hardware minimum-limit command is sent. Temperature is signed sixteenths of Celsius; no-sensor 7F7F or invalid data reports ALERT while preserving the last valid reading. Both documented three-byte and legacy two-byte temperature replies are supported (see REFACTOR.md).

Celestron exposes `X_FOCUSER_CALIBRATION.CALIBRATE`, with measured progress and read-only device limits. Calibration has a 180-second deadline and can be aborted. Limits/readback failure is reported as a failed calibration. SYNC, fans and temperature are hidden for this model. Speed, reversal, backlash and automatic compensation are not implemented.

Disconnect cancels queued finalizers and attempts a hardware stop. Communication failure cannot confirm a physical halt. PlaneWave RTS/CTS is retained; hardware echo releases RTS before the response, and all failure paths release it. Serial endpoints without modem-line support retain the legacy fallback, enabling PTYs; tests do not validate electrical handshake behavior.

## Protocol and validation

Authoritative source: `indigo_focuser_efa.driver`. Primary PlaneWave reference: bundled `PlaneWave EFA Communication Protocols.pdf`. Celestron-specific extensions follow the existing driver and require hardware verification. The parser checks frame size, checksum, source/destination/command, exact payload shape and command-specific acknowledgement.

Xcode and Windows project integration are included. Migration/test results and platform/hardware limitations are recorded in `REFACTOR.md`; simulator coverage is mapped in `../../indigo_test/CHANGES.md`. The API generator itself was not changed.

## Use

```sh
indigo_server indigo_focuser_efa
```

Select the serial port before connecting. Keep actual hardware movement within its usable calibrated travel.

## License

INDIGO Astronomy open-source license.
