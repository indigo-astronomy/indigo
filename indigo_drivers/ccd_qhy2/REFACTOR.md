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

The dead assignment was in the generator's SDK hot-plug template, which 17 drivers share, so the
guard was moved there rather than kept in this driver's hook; this driver's `unplug_match` is back
to its plain form. `ccd_svb` passes the suite unchanged either way, because its SDK stops reporting
a camera that is gone.

### Open: the process can abort after an unplug during streaming (QHY2-001)

With the withdrawal fixed, the full suite aborts with SIGSEGV in two runs out of four, always after
the streaming case has had its port switched off and back on, and never with a message beyond
`Acquisition failed`. The three runs of that case on its own all passed, so it needs the state the
earlier cases leave behind rather than the streaming teardown alone. A run that aborts leaves the
camera re-enumerating, so the next run can fail its own start-up for want of a camera.

The crash is only reachable now: before the fix the driver never detached the device of a camera
that was switched off while connected, so the teardown path was never taken. The cause is not
established and needs a run under a debugger; `ccd_uvc` records a similar abort from its own unplug
handler as UVC-014.

### Open

`indigo_ccd_qhy` still carries the identical hand-written `qhy_busy()` helper on the same nine properties and was not migrated here.

## QHY5-M hardware run (2026-09-22)

Non-interactive hardware run on macOS arm64 against a QHY5 (Orion StarShoot Autoguider variant) on a Pegasus Ultimate Powerbox hub, through the new `make -C indigo_test test-ccd-qhy2-hw` target. Two defects found, both reproducible against the fake SDK once it was taught to model this camera.

### Discovery depends on who loaded the firmware first

The camera enumerates as `1618:0901` with no firmware. `ccd_ssag` loads its own and the camera becomes `1856:0012 StarShoot Autoguider`, which `ScanQHYCCD()` does not recognise, so this driver then reports no camera at all. After a power cycle the SDK loads `QHY5.HEX` itself, the camera becomes `QHY5-CMOS` and `ScanQHYCCD()` returns it as `QHY5-M`. The two drivers therefore cannot share the camera within one power cycle, which is expected for a firmware-loading device but is worth knowing before concluding the SDK dropped support for it. The run needs `INDIGO_FIRMWARE_BASE` pointing at `indigo_drivers/ccd_qhy/bin_externals/qhyccd/firmware` on a host with no installed INDIGO, as the driver's `README.md` already documents.

### Defect 1: streaming hung the device queue (version 36 to 37)

`BeginQHYCCDLive()` never returns on this camera. The device queue thread stayed in it:

```
Queue QHY5-M
  acquisition_start(indigo_device*, bool)  (in indigo_ccd_qhy2.dylib)
    BeginQHYCCDLive  (in libqhyccd.dylib)
      QHYBASE::BeginLiveExposure(void*)
        QHYCAM::vendTXD(void*, unsigned char, unsigned char*, unsigned short)
          libusb_control_transfer  (in libusb-1.0.0.dylib)  sync.c:139
```

Nothing else could run on that queue afterwards, so `CCD_ABORT_EXPOSURE` and `CONNECTION` timed out as well and the camera was unusable until the process was killed. The driver's own deadline never fired, because the block is inside the vendor call.

`IsQHYCCDControlAvailable(handle, CAM_LIVEVIDEOMODE)` answers `QHYCCD_ERROR` for this camera while `CAM_SINGLEFRAMEMODE` answers `QHYCCD_SUCCESS`, so the SDK knows. `CCD_STREAMING` and `CCD_STREAMING_SETTINGS` are now hidden when the camera has no live video mode.

### Defect 2: the exposure was taken twice (version 37 to 38)

Every exposure took twice its duration plus readout - 0.1 s took 0.95 s, 2.5 s took 5.84 s, 16.5 s took 33.85 s. A standalone probe outside INDIGO shows why:

```
QHYCCD_READ_DIRECTLY = 8193
0.5 s: ExpQHYCCDSingleFrame -> 8193 after 0.243 s, remaining = 0, GetQHYCCDSingleFrame -> 0 total 0.818 s
4.0 s: ExpQHYCCDSingleFrame -> 8193 after 0.326 s, remaining = 100, GetQHYCCDSingleFrame -> 0 total 4.398 s
```

`QHYCCD_READ_DIRECTLY` means the camera does not time the exposure itself: the start call returns at once and `GetQHYCCDSingleFrame()` blocks for the exposure instead. The driver accepted that return value but still waited the full duration before reading, so the exposure elapsed twice. It now starts the read immediately for that return value and keeps the deadline covering the exposure that happens inside the read. Measured afterwards: 16.5 s takes 17.19 s. Cameras that answer `QHYCCD_SUCCESS` are untouched.

### Coverage

The shared fake SDK of `indigo_test/integration/test_ccd_qhy_sdk.cpp` gained a live-video capability, separate from the live-mode state it already tracked, and a read-directly mode in which `ExpQHYCCDSingleFrame()` returns `QHYCCD_READ_DIRECTLY` and the read blocks for the exposure.

- `missing live video` requires `CCD_STREAMING` to be absent and a single exposure to still work. It is `QHY2` only, because the legacy SDK enum has no `CAM_LIVEVIDEOMODE` and the legacy driver cannot ask.
- `read directly exposure` requires a one second exposure to finish in under 1.6 s. It failed at 2.02 s before the fix and passes at 1.01 s after, for both drivers.

`discovery capacity identity` was already failing before this session: commit `0057e7302` made `sdk.unplug_match` confirm a removal libusb could not identify rather than veto one it reported, so a failing `ScanQHYCCD()` no longer keeps a device whose own libusb device left. The case now sends its inconclusive events with a libusb token no logical device was created from, which is the path the hook still serves.

### Results

| run | result |
| --- | --- |
| `build/integration/test_ccd_qhy2_sdk` | 39/39 |
| `INDIGO_TEST_DEVICE="QHY5-M" make -C indigo_test test-ccd-qhy2-hw` | 1/1 |

The run covered exposures from 0.1 to 16.5 s, frame types, ROI, bins and read modes, the busy refusal guards for both exposure and streaming, abort and restart, the guider on all four axes including during an exposure, disconnect/reconnect, driver reload and a fresh exposure. Live video is not applicable to this camera, and the test now says so instead of assuming it.

Hot-plug was not established: the camera hangs on the Powerbox hub, whose port switch is invisible to the host's hub driver on macOS (see `indigo_drivers/ccd_atik/REFACTOR.md`).
