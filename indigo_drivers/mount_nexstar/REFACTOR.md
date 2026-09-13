# Mount NexStar Refactor

## Current Code Findings

- The driver was still hand-written INDIGO 2.x style code in `indigo_mount_nexstar.c`; the source of truth is now `indigo_mount_nexstar.driver` and generated `indigo_mount_nexstar.c`, `indigo_mount_nexstar.h` and `indigo_mount_nexstar_main.c` must stay synchronized.
- The old connection lifecycle kept a private `count_open` and unlocked `serial_mutex` from inside the failed `mount_open()` path even though the caller owned that lock. The generated lifecycle replaces this with the generator-owned shared connection counter and a transactional `nexstar_open()` / `nexstar_close()` pair.
- The mount and guider shared one serial `libnexstar` connection. The migration keeps that topology and lets the generator serialize connection open/close ownership across both logical devices.
- The old guider RA/DEC handlers slept for the whole guide duration in the property handler. The migration changes guide pulses to a start handler plus `guider_guide_ra_finalizer()` and `guider_guide_dec_finalizer()` so the device queue is not blocked.
- The old `MOUNT_SLEW_RATE` change path marked and updated `MOUNT_GUIDE_RATE_PROPERTY` instead of `MOUNT_SLEW_RATE_PROPERTY`. The generated property handler now updates the handled slew-rate property.
- The old GPS detach path returned `indigo_guider_detach(device)` from a GPS device. The migrated custom GPS device returns `indigo_gps_detach(device)`.
- GPS remains dynamically attached only for the Celestron dialect, matching the old driver behavior. The generator owns mount and guider boilerplate; custom GPS code remains in the `.driver` code block because the old driver exposes GPS conditionally after mount identification.
- Baseline validation before migration: `make -C indigo_test build/integration/test_mount_nexstar_simulator` succeeded, but running `indigo_test/build/integration/test_mount_nexstar_simulator` exited with signal 11 before any migration changes.

## Documentation Findings

- `README.md` describes serial/network NexStar protocol support, single startup instance plus runtime additional instances, `libnexstar` dependency, and two non-standard controls: `TRACKING_MODE` and `COMMAND_GUIDE_RATE`.
- `indigo_docs/PROPERTIES.md` already lists `TRACKING_MODE` and `COMMAND_GUIDE_RATE` as custom NexStar properties; after migration its source note needs to point to the `.driver` source and generated `.c` output.
- `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` listed the Arduino `.ino` simulator candidate even though a host-side C simulator exists. The migration moves the C simulator into the implemented simulator table and removes the stale candidate entry.
- The mount testing rules require simulator-backed coverage for connect/identity, GOTO and SYNC, manual motion and abort, tracking/rates, park/unpark, time/location/options, transport failure/recovery, and guider coverage for all guide directions.

## Migration Plan

1. Convert the driver to `indigo_generator` with `indigo_mount_nexstar.driver` as the source of truth.
2. Preserve serial protocol behavior through `libnexstar`, including Celestron and Sky-Watcher dialect detection, ST4 guide-rate handling, tracking-mode selection, mount time/location updates, side-of-pier probing and conditional Celestron GPS exposure.
3. Move long-running operations to generator queues and finalizers: mount park polling and guider pulse completion.
4. Expand the host-side serial simulator to cover elapsed goto/park motion, manual slew stop state, sync, tracking, time/location, guide-rate readback, Celestron GPS pass-through and failure injection needed by automated tests.
5. Replace the smoke-style NexStar test with full mount/guider simulator coverage following `indigo_test/DRIVER_TESTING_RULES.md`; keep Windows status unchanged because this migration explicitly does not solve `libnexstar` Windows support.
6. Run generator, build the driver/test target, run the full NexStar simulator test, clean test artifacts and update `MIGRATION_STATUS.md`, `indigo_test/CHANGES.md`, `indigo_docs/PROPERTIES.md` and `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` with the verified results.

## Validation Results

- `make -C indigo_drivers/mount_nexstar -f ../../Makefile.drv` passed after regenerating `indigo_mount_nexstar.c`, `indigo_mount_nexstar.h` and `indigo_mount_nexstar_main.c` from `indigo_mount_nexstar.driver`.
- `make -B -C indigo_test build/integration/test_mount_nexstar_simulator` passed and force-relinked the test binary against the regenerated driver archive.
- `cd indigo_test && build/integration/test_mount_nexstar_simulator` passed all ten simulator cases: Celestron and Sky-Watcher connection/identity, Celestron property/actions, NexStar HC time/location, SYNC/GOTO/abort, unaligned rejection, manual motion/park/unpark, queued park abort, Alt/Az `TRACKING_MODE`, dynamic Celestron GPS and guider guide-rate/all-direction pulse behavior.
- `make -C indigo_test test-clean` passed and removed integration build artifacts.
- Windows runtime remains unverified and unchanged; `MIGRATION_STATUS.md` keeps the existing `libnexstar` Windows TODO in the comment column.
