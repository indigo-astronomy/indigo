# Task brief

- Task ID: 20261112202203-24f6-fix-qhy2-readout-timeout
- Owner: Peter Polakovic
- Created: 2026-11-12

## Problem and desired outcome
GitHub issue indigo-astronomy/indigo#783: a QHY168C (firmware 22-9-5, USB 3, macOS 26.6.2 on an M4 Max) connects to `indigo_ccd_qhy2` 3.0.0.46 but every exposure has min, max, average and median 0 in all channels. QHY's EZCAP_Qt build 15121 captures normal images on the same Mac, cable and port. The reporter tried 0.1–1 s exposures, every capture mode and USB traffic 50 and 60.

The driver sets `SetQHYCCDSingleFrameTimeOut(handle, 1000)` for every single frame. The SDK header says the timeout is in ms and has to cover the readout as well, and that the SDK default is 60000 ms. A QHY168C full frame is about 33 MB, and its readout does not fit into one second together with the exposure. The owner asked to fix this.

Desired outcome: the single-frame timeout covers the exposure plus a readout allowance, and a slow readout no longer produces an empty frame.

## Users and use cases
Users of QHY cameras with long readout (large sensors, older USB 3 models such as the QHY168C) taking single exposures with `indigo_ccd_qhy2`.

## Scope and non-goals
In scope:
- the single-frame timeout in `indigo_ccd_qhy2.driver` and the regenerated `.cpp`;
- the driver version bump;
- a fake-SDK model of the frame timeout and a regression test in `indigo_test/integration/test_ccd_qhy_sdk.cpp`.

Non-goals:
- the `CONTROL_USBTRAFFIC` readback warning ("Control 12 reads back 0");
- the effective-area ROI offset on the QHY168C;
- the legacy `ccd_qhy` firmware load failure (`OSXInitQHYCCDFirmware = 0xffffffff`);
- an SDK update;
- streaming, which does not use this timeout.

## Constraints and references
- `.driver` is the source of truth; regenerate the `.cpp`.
- No README change without approval.
- No push.
- SDK header: `indigo_drivers/ccd_qhy2/bin_externals/qhyccd/include/qhyccd.h`, `SetQHYCCDSingleFrameTimeOut`.
- The 1000 ms value first appeared in `3768cd34f` ("ccd_qhy/ccd_qhy2: migrated to generator", 2026-09-12). The driver before it never called `SetQHYCCDSingleFrameTimeOut()` and used the SDK default of 60000 ms, so this is a regression of the generator migration.

## Initial questions
None blocking. The real SDK's behaviour on timeout (success with an unfilled buffer) is inferred from the report: the driver logged no read error, so `GetQHYCCDSingleFrame()` returned success with the requested size. It is not confirmed from the SDK binary or on hardware.
