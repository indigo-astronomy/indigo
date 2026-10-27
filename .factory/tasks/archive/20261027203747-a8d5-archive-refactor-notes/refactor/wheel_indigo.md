# PegasusAstro Indigo wheel portability follow-up

Status: Windows compile fix in progress, 2026-09-15. Baseline driver version: `0x03000005`.

## Audit and scope

The generated serial wheel driver exposes one wheel with seven slots, uses portable `indigo_uni_io` transport helpers and completes a move by polling `WF` until the target is reached or a 30-second deadline expires. The `.driver` is the source of truth. Its private data stores a POSIX `struct timespec`, and both move start and deadline calculation call `clock_gettime(CLOCK_MONOTONIC)`. The Windows x64 build therefore fails before protocol behavior can run. INDIGO already provides `indigo_monotonic_time()` for this purpose. Properties, protocol commands, asynchronous handler ownership, simulator model, Windows project integration and supported wheel capabilities are otherwise unchanged.

The repository contains a deterministic fake-transport suite and a host serial simulator suite. Baseline universal macOS build passed; fake transport passed 3/3 and simulator integration passed 2/2. No physical wheel is available, so hardware testing will not be performed or claimed.

## Atomic plan

1. **Completed.** Stored the move start as portable seconds, used `indigo_monotonic_time()` for elapsed time, and incremented the driver version from `0x03000005` to `0x03000006`.
2. **Completed.** Regenerated C/H/main output and verified byte-identical second generation. The implementation emits version `0x03000006` and no longer contains direct POSIX clock use.
3. **Completed.** Universal macOS driver build passed; fake transport passed 3/3 and serial simulator integration passed 2/2. Xcode registration, XML and diff formatting validation passed. Native Windows compilation remains to be confirmed in the reporting environment.

## Found defects

Windows compilation fails because the driver-specific `.driver` block uses POSIX `CLOCK_MONOTONIC`. Replace it with the framework monotonic API; the existing timeout and fresh-move fake transport scenario plus elapsed-motion simulator scenario are the regression tests.

## Final test summary

Simulated tests run: **5**. Simulated tests passed: **5**. Hardware tests run: **0**. Hardware tests passed: **0**.

## Initialisation at connect (3.0.0.9, 2026-10-08)

Against the bundled serial command language (firmware >= 1.1): `WI` initialises the wheel to filter 1, as at
power-up, and is answered with `WI:1` before the wheel has turned there. 3.0.0.8 published `WHEEL_SLOT` OK on
filter 1 at once after `WI`. The connect now keeps `WHEEL_SLOT` BUSY and confirms filter 1 through `WF`, with the
same 30 s deadline as a move. The protocol checks were not tightened by the generator migration: the original
driver already sent `W#`, `WI` and `WV`, and `FW_OK` is now matched as a prefix.

Both simulators turn the wheel to filter 1 on `WI` instead of jumping there; the C simulator takes `--slot` for
its start slot. `connect_waits_for_the_wheel_to_reach_filter_1` fails against 3.0.0.8. The fake transport and
simulator cases wait for the initialisation before their first request, as a client has to.

The new hardware suite `indigo_test/hardware/test_wheel_indigo_hw.c` (`make test-wheel-indigo-hw`) ran against
`wheel_indigo_simulator.ino` on an ESP32-S3 (11/11); no physical wheel was available.
