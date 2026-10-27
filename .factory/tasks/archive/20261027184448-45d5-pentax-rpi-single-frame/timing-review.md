# Independent bulb timing analysis

Scope: read-only analysis of the uncommitted task tree on 2026-10-27, production `scsi_command`, `pentax_arguments`, `pentax_button`, exposure handler/finalizer, fake USB timing events, and the local INDI bundled pkTriggerCord sources. No camera, remote Pi access, production edits or timing calibration was performed. This is a focused design review, not the final whole-diff quality or security sign-off.

## Confirmed defect and test-model correction

`pentax_button` returns after parameter staging, command transport, and `pentax_status` readiness polling. The current bulb handler resets its deadline only after this entire sequence. Consequently, camera readiness latency extends the requested hold rather than counting toward its absolute deadline. The real 1.2-second requests produced 1.40/1.42-second EXIF durations after this change; the prior deadline placement produced 1.09 seconds. Those observations do not identify physical curtain-opening and closing times separately.

The fake USB test delays the trigger status response by 200 ms and checks only a lower bound on the interval after that response. That assertion positively enforces the defective extra delay and has no upper bound, so it cannot reject overlong exposures.

The reference `pktriggercord-cli.c:318` (`bulb_old`) subtracts elapsed time from an earlier timestamp after `pslr_bulb(true)` and `pslr_shutter()` return. It does not blindly wait the entire requested interval after trigger readiness. `pslr.c:953` confirms that the old bulb start/stop controls share the existing argument, command, status sequence. No fixed per-exposure transport sleep exists in this driver's `scsi_command`; readiness polling itself may sleep in 50 ms increments.

## Recommended software contract

Retain the existing prepare-bulb, trigger, readiness, close ordering. Obtain a monotonic timestamp at trigger command dispatch, after its parameter upload and before readiness polling. Base the absolute close deadline on that event plus the requested duration, without resetting it when readiness completes. Keep the later readout timeout separate.

The fake should record the trigger command and close command events and bound their interval on both sides, including injected trigger-readiness latency. Include a delayed argument upload if practical to prove preparation does not consume the requested interval. Abort and failure paths must continue closing an owned bulb and preserving capture cleanup.

The software contract cannot prove physical sensor integration time. Mechanical shutter latency and end-command delivery have not been measured independently; do not hardcode calibration from the 1.09/1.42 observations or claim exact hardware timing after an offline-only repair. A future physical timing check is still needed.

Verdict: design correction required; no production write by this reviewer.
