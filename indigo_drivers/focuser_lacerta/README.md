# LACERTA Motorfocus focuser driver

## Supported devices

LACERTA MFOC and FMC, serial connection at 9600 baud. One focuser is present at startup; additional independent instances can be configured at runtime. No hot-plug support.

The generated INDIGO 3.0 source is `indigo_focuser_lacerta.driver`. Protocol reference: bundled `Lacerta_Mfoc-Fmc_API_2024.xlsx`, sheet `Munka1`.

Absolute and relative moves, coordinate SYNC, abort, reversal, backlash and maximum position are supported. Backlash is 0–255 steps. Maximum position is 300–65535 for firmware v1 and 300–250000 for v2/v3; minimum position is zero. Position and relative-step metadata follow the configured maximum. Settings are checked against device readback and loaded again on connection.

Temperature is read-only; the documented 99.9 (NC) response reports ALERT and preserves the last valid value. Speed, automatic mode and temperature compensation remain hidden and unsupported.

Movement reports measured position separately from its target. A no-op completes immediately. Abort reads the stopped position; failed stop/readback reports ALERT. Motion with no observed progress for 100 polls (nominally about 10 seconds) attempts a stop and reports ALERT. This is a software stall policy, not a measurement of physical motor timing. If stop or stopped-position confirmation fails, retry abort or reconnect before another move. Disconnect cancels queued work and attempts to halt active motion; transport failure cannot confirm a physical stop.

## Supported platforms

Uses portable INDIGO serial I/O. Universal macOS arm64/x86_64 compilation is checked; simulator tests run on arm64. Windows project/solution integration is included, but Windows and Linux builds/runs and physical hardware acceptance remain unverified.

## License

INDIGO Astronomy open-source license.

## Use

```sh
indigo_server indigo_focuser_lacerta
```

Select the serial port before connecting. Migration decisions and validation results are recorded in `REFACTOR.md`; automated coverage is recorded in `../../indigo_test/CHANGES.md`.
