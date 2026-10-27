# Specification

Use existing single-frame harness twice, requested1.2s and3s, native JPEG, no control exercise or abort sequence. Preserve current camera settings and disconnect after each frame. Compare JPEG EXIF ExposureTime with request, report signed/absolute error and pass if each is within50ms. Two samples are a short metadata sanity check, not physical curtain metrology or statistical certification. Stop after first capture failure.90s total watchdog. Do not change camera format, dial, USB binding or firmware. Images stay outside Git. Camera must already be physically B with JPEG, as last restored.

## Owner scope updates

Owner authorized the awake camera launch, requested the complete registered hardware suite, then explicitly authorized README documentation of driver limitations. Run canonical full profile and preserve failures; run distinct supported bulb/RAW profiles, restore JPEG/latent DNG, and document actual timing and physical mode limitations. No speculative timing calibration or production source change.
