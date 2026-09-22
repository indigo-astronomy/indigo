# indigo_ccd_qhy2 refactoring notes

## `reject_change` migration (2026-09-18)

The driver refused changes during an acquisition through a hand-written helper, `qhy2_busy()`, called from the `on_change_request` block of nine properties: `CCD_GAIN`, `CCD_OFFSET`, `CCD_GAMMA`, `CCD_FRAME`, `CCD_BIN`, `CCD_MODE`, `X_PIXEL_FORMAT`, `X_ADVANCED` and `X_READ_MODE`. All nine now use the generator's `reject_change` block with the same condition and message, and the helper is gone. Version increased from 31 to 32 (`0x0300001F` to `0x03000020`).

```
reject_change {
    condition = CCD_EXPOSURE_PROPERTY->state == INDIGO_BUSY_STATE || CCD_STREAMING_PROPERTY->state == INDIGO_BUSY_STATE;
    message = "Acquisition in progress";
}
```

The behavioral difference is the per-item `do_update` marking the generator emits and the helper did not. Without it the protocol adapter omits unchanged items, so a client behind an adapter keeps displaying the value it requested even though the driver refused it. Everything else is unchanged: both forms refuse before any requested value is copied, so `value` and `target` were already preserved.

Regeneration is deterministic; a second run produces byte-identical output. SHA-1 values are `aacb480135033d561c936a44ffcb61aaa4ac85f4`, `9d4945d6631440f55006fce0f02032bb2337a2c3` and `df42959fa39f402eea8335b85fb10e162eb0eaac` for `.cpp`, `.h` and `_main.c`; the `.h` and `_main.c` outputs are unchanged.

### Test coverage

`acquisition conflicts` in `indigo_test/integration/test_ccd_qhy_sdk.cpp` was extended: a refused `CCD_GAIN`, `CCD_OFFSET` or `CCD_GAMMA` change keeps `value`, `target` and the SDK parameter untouched during an exposure and during a stream, and all three are accepted again once the camera is idle. Both suites pass, 74 test bodies over `indigo_ccd_qhy` and `indigo_ccd_qhy2`:

```sh
make -C indigo_test test-ccd-qhy-sdk
```

This case does not discriminate between the two guard forms. The fake-SDK client is in process and receives the driver properties directly, so the per-item `do_update` marking is invisible to it; only a protocol adapter would show the difference.

### Hardware regression (QHY5III178M, no replug)

`indigo_test/hardware/test_ccd_qhy_hw.c` gained a `reject` scenario that asks every guarded property for a value it does not hold while a 5 s exposure runs.

```sh
INDIGO_TEST_DEVICE=QHY5III178 QHY_HW_CASE=reject indigo_test/build/hardware/test_ccd_qhy_hw --run build/drivers/indigo_ccd_qhy2.dylib indigo_ccd_qhy2
```

`CCD_GAIN`, `CCD_OFFSET`, `CCD_GAMMA`, `CCD_FRAME`, `CCD_BIN`, `CCD_MODE`, `X_PIXEL_FORMAT` and `X_ADVANCED` each returned ALERT with unchanged values and targets and the `Acquisition in progress` message, the exposure still delivered its image, `CCD_GAIN` and `CCD_FRAME` were refused the same way during an unbounded stream, and the guards were not sticky. `X_READ_MODE` exposes a single mode on this camera, so no alternative value can be requested and that guard is still unverified on hardware; it needs a camera with several read modes. `CCD_MODE` is only exercised on the refusal path, because restoring it would reprogram frame and binning together.

The full scenario set (everything except hotplug) passed after the migration: `QHY hardware: all tests passed`, covering exposures from 0.1 s to 16.5 s, repeated pixel-format and single/live switching, geometry and modes, settings, abort, streaming, guider pulses, reconnect and driver `dlclose`/`dlopen`.

### Unplug of a connected camera (fixed, 3.0.0.36)

`test-ccd-qhy2-hotplug-hw` failed its three connected cases and passed every idle one: a camera
switched off while connected stayed published, and the disconnect only completed once the camera
was plugged back in.

`unplug_match` answered the removal with `ScanQHYCCD()`, and the generator enters that block with
`unplug_result` already set from the libusb identity of the device that left, so the rescan
replaced the answer libusb had given instead of adding to it. `ScanQHYCCD()` keeps reporting a
camera whose handle is still open, so the removal was refused, and no second `LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT`
ever arrives for the same device.

The block now runs only when libusb has not already identified the device. All six cases pass on a
QHY5III178M on Linux arm64.

The same dead assignment is in the generator's SDK hot-plug template, so `ccd_asi`, `ccd_playerone`
and `ccd_svb` carry the same shape. `ccd_svb` passes the suite unchanged, because its SDK stops
reporting a camera that is gone; the other two are untested for it.

### Open

`indigo_ccd_qhy` still carries the identical hand-written `qhy_busy()` helper on the same nine properties and was not migrated here.
