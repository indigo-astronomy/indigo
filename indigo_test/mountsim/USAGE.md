# MountSim acceptance on macOS

These opt-in tests require macOS, a logged-in GUI session and a built MountSim 2.3 app. They are separate from the portable simulator suite and from physical-hardware tests. Neither `test` nor `test-integration` depends on this directory. No Linux/Windows CI runner needs Python, Cocoa or MountSim for these cases.

From the INDIGO repository root:

```sh
make -C indigo_drivers/mount_temma -f ../../Makefile.drv all
make -C indigo_test test-mount-temma-mountsim

make -C indigo_drivers/mount_rainbow -f ../../Makefile.drv all
make -C indigo_test test-mount-rainbow-mountsim
```

The default app is the sibling MountSim checkout's `build/Build/Products/Debug/MountSim.app`. Override `MOUNTSIM_APP` with an absolute app path if needed. Build MountSim using its own README first; INDIGO does not build or install it automatically.

## Shared launcher contract

`run_mountsim.py` is model-independent. Supply `--app`, the exact control-server `--mount` name and `--test` binary. The binary implements `--list` (one case name per line) and accepts one exact case name. Each case runs serially in its own process and its own MountSim instance with fresh isolated preferences and a private control port. The C entry point uses `indigo_test_use_private_home()` before starting the bus.

The launcher gives the test `MOUNTSIM_PTY`, `MOUNTSIM_TRACE` and `MOUNTSIM_TRANSPORT_CONTROL`. `mountsim_test_common.h` supplies PTY attachment, trace-path discovery and explicit disconnect/reconnect of the relay transport. A reconnect returns a fresh slave name, which the test must apply to DEVICE_PORT. The app's simulated mount session survives the relay interruption, as real hardware survives an unplugged client cable.

A transparent relay forwards all bytes without synthesizing replies or translating protocols. Raw captures contain both directions. `--terminator 0d0a` groups Temma TX trace records into CRLF frames; the terminator affects only logging. Omit it for binary protocols or select a different delimiter for another mount. A new model gets its own `test_<exact_driver_name>_mountsim.c`, case registry and Darwin-only Makefile rule; shared lifecycle and transport code belongs in the launcher/header.

For one case, from `indigo_test`:

```sh
python3 mountsim/run_mountsim.py --app ../../MountSim/build/Build/Products/Debug/MountSim.app --mount Temma --terminator 0d0a --test build/mountsim/test_mount_temma_mountsim --case temma_park_arrives_before_standby
```

Set `MOUNTSIM_DEBUG=1` for driver communication diagnostics. `--timeout` sets the per-case watchdog (120 seconds by default). `--output` selects an artifact directory; by default each named case retains `session.json`, `test.log`, `app.log`, `serial.raw` and `commands.events` under `build/mountsim-results`. Each run replaces that case's previous logs. The launcher stops only processes it started and removes temporary preferences and PTYs on completion or failure. `make test-clean` removes build outputs and default logs; copy desired evidence elsewhere before cleaning.

## Evidence and limits

Record Temma runs as `MountSim 2.3 (Temma)`, not `simulator`. The app version and selected model are captured from its control response. This is software acceptance through an independent implementation, not physical Temma validation. PTYs do not exercise electrical baud/parity errors, real firmware variants or mechanical pointing accuracy.

Guiding statistics measure the interval between direction ON/OFF command frames forwarded to MountSim, including host/relay scheduling. They do not measure physical relay edges or the GUI motor loop's exact application time. Idle means tracking disabled; the second workload enables tracking and coordinate polling. All directions use 20/100/500 ms, three repetitions each, with discarded warmups. No universal precision threshold is asserted.

Malformed replies, injected protocol failures and fake-device edge cases remain in the separate portable `test_mount_temma_simulator` suite. The MountSim suite removes/recreates the relay PTY for real client-side transport-loss coverage and reconnect, without injecting synthetic mount replies.

### RainbowAstro RST135

Use model `RST135`, trace terminator `23` (`#`) and result type `MountSim 2.3 (RainbowAstro RST135)`. Its firmware identity is 190402, so the driver exercises the legacy global-stop and read-only clock branch. Avalon StarGO is a different model/protocol.

The 13 cases cover inventory/lifecycle, SYNC including fractional signed coordinates, reachable and rejected GOTO, already-at-target completion, BUSY conflicts, abort/recovery, all manual directions/rates plus measured movement and stop, tracking/rate readback, guide-rate and location reconnect roundtrips, park position/tracking and abort/disconnect, and real idle/active PTY loss. There is no guider interface in this driver; guide-rate configuration is distinct from pulse guiding. Modern firmware, malformed responses and injected protocol errors remain in the 15-case portable Rainbow suite.

## iOptron model matrix

Build the production `mount_ioptron` driver and the sibling MountSim checkout, then run:

```sh
make -C indigo_test test-mount-ioptron-mountsim
```

The target runs CEM25, CEM40, GEM45, CEM60, SmartEQPro, SmartEQ, ZEQ25, CEM70
and CEM120 sequentially. Override `MOUNTSIM_IOPTRON_MODELS` for a focused rerun.
Each model has eight named cases covering identity/reconnect, SYNC/GOTO/abort,
manual axes/rates, tracking/location/time, park/home/options, idle and active
relay loss, and guider timing/shared lifecycle. Unsupported capabilities are
skipped inside their case; these counts are not portable integration counts.
CEM60 advertises firmware 190716 and exercises the driver's protocol-3 branch;
the portable suite separately covers firmware 161101/protocol 2.5.

The shared launcher's trace timestamps use system `CLOCK_MONOTONIC` so that
Python 3.9 on macOS and C callbacks share an epoch. Watchdog deadlines can use
process-relative `time.monotonic()`. For duration-based guide commands, the
reported latency runs from relay forwarding to public-property completion;
there is no fabricated OFF edge. Keep this separate from physical pulse timing.
