# Specification

Target available K-m (0x12d73; K-2000 same protocol), retain monitoring without unverified capture for other models. Incremental completion, no generator migration.

AC-1: Transactional checked connection, ALERT/disconnected rollback; safe cancellation/close/disconnect/reconnect.
AC-2: Functional supported program/aperture/shutter/ISO writes with readback and ALERT on failure. Restore capture-temporary program/shutter settings; preserve camera format and SD card.
AC-3: Short timed and fractional/long bulb exposures, monotonic timing/shared countdown, finite asynchronous polling/readout. Abort/disconnect serviceable. Timed abort suppresses image and drains only own capture; no undocumented mechanical stop claim.
AC-4: Capture only new RAM buffer bits, bounded JPEG/PEF/DNG segments/chunks; preserve pre-existing RAM and SD images. Delete only own successfully delivered image or aborted capture, finite cleanup; short/error transfers fail.
AC-5: Existing INDIGO DSLR image pipeline and upload modes; no unsupported streaming/liveview advertisement.
AC-6: Original baseline, production fake USB/bus regressions, ASan/UBSan/configured checks and short grouped non-interactive physical test if claim accessible. Snapshot/restore settings, no physical unplug, conserve batteries.

High complexity (shutter/readout/abort state, untrusted segments, USB ownership); medium risk (camera settings/images, memory safety). Use protocol facts from open-source sources, no copied copyleft source or new dependency. Hardware baseline discovery succeeds but claim fails(-99), no settings writes/exposures; blocked evidence is not a pass.

## Owner-approved delivery revision

On 2026-10-26 the owner explicitly requested commit, merge and push now and continuation later on Linux (delivery-decision.md). AC-1 through AC-5 are delivered with software/fake-USB evidence only. AC-6's physical acceptance gate is deferred, not passed. Hardware limitations and the failed 1/0 recorded run remain visible. This task closes the current software delivery; a later Linux hardware task must establish physical behavior.
