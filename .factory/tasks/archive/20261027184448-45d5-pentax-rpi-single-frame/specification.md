# Specification

Deliver the existing Pentax K-m capture path with correct nonlegacy protocol argument framing, selected ISO state, settled control readback, restoration from physical Bulb, and a monotonic bulb deadline measured from trigger command dispatch. Preserve legacy framing, USB acquisition ownership, error bounds, public properties and abort/recovery behavior. Do not add guessed hardware timing calibration or claim physical timing accuracy from simulated tests.

Maintain opt-in connection, single-frame and bulb hardware profiles with distinct recorded labels so partial acceptance never replaces the full-camera verdict. Single-frame may select a finite 1/4000–5s duration before USB initialization; default remains0.1s. Save only the first validated native artifact, exclusively at an explicit absolute path. Native image bytes remain outside Git.

Actual hardware scope: K-m firmware1.11.1.8 on Linux arm64, controls and timed JPEG capture/abort/recovery in physical M; bulb JPEG capture/abort/recovery in physical B; DNG and PEF capture/transfer/unpack in physical B. Original pkTriggerCord also failed to switch M→B with user-mode flag0. The owner physically selected B with USB briefly disconnected; this setup is not a hotplug acceptance test. Restore ISO800, f/8, physical B, JPEG with latent DNG after tests.

Final owner instruction: fix bulb timing without accessing the camera, then commit, merge into master and push. Physical EXIF accuracy of that final repair remains a later check. No full-class/multiple-model/macOS hardware certification is implied. The prior Pentax audit deferral remains scoped to this continuation.
