# Pentax capture security review

Independent in-host reviewer `/root/pentax_capture_review`; did not implement production code or tests. Reviewed evolving task candidate based on local master64e965d7f, then reinspected the final partial K-m/K2000 capture/control/lifecycle implementation and production-backed fake tests. Final reviewed source blob: ab1b7f5a1994031233d604e600e0407dddea2dd0; fake-test blob: e0ad77c067a1c9e06cbfe4d5e938ad4e6f4c3a59. The task remains physically blocked and unfinished; this security verdict does not archive or complete it. This is a scoped security review, not whole-repository audit, complete camera acceptance or proof that no defects remain.

## Scope and trust boundaries

Camera status lengths, segment metadata, memory addresses, image lengths and native file bytes are untrusted. The SCSI boundary requires exact CBW/data/CSW transfers, valid CSW signature/tag/residue and finite retries. Response storage remains bounded to the protocol read width. New image transfer is capped at64MiB/four segments/ten descriptors, checks contiguous output offsets and address/length overflow before allocation, and downloads exact blocks no larger than65,536bytes. JPEG format fields are validated before arithmetic/narrowing; RAW fields apply only to RAW requests.

Capture snapshots the original RAM mask, accepts a single new bit and preserves pre-existing RAM buffers and SD-card images. Ambiguous new buffers are retained for manual recovery. Native image delivery uses the framework upload path; no authentication, network, firmware, dependency, secret or deployment mechanism changed. Reference sources and independent implementation approach are recorded in research.md; this review does not establish broader licensing provenance.

## Finding reinspection

- **FND-20261026212631-884a-complete-pentax-001 — queued connection mutex deadlock:** the new queue already holds the framework device mutex. The redundant explicit connection lock was removed; public connection scenarios now complete through the real queue. Fixed and dynamically reverified.
- **FND-20261026212631-884a-complete-pentax-002 — completed native BLOB ownership:** framework native delivery retains the supplied pointer. The driver now transfers published storage to `last_image`, frees only in-progress storage on failure/abort, preserves completed storage until replacement, and releases it after base detach. Tests read/re-enumerate the completed image after completion and after abort; independent sanitizer run passes.
- **FND-20261026212631-884a-complete-pentax-003 — abort starvation during image transfer:** abort uses urgent priority while continued finalizers use normal priority, allowing abort between bounded blocks. The gated abort-during-download case verifies suppression and recovery.
- **FND-20261026212631-884a-complete-pentax-004 — setting restoration/readback and format validation:** verified native mode aliases map to command values; unsupported/invalid snapshots are refused before changing camera settings. Setting and ISO commands require full-state readback, rather than reporting requested values based solely on transport success. JPEG fields are range-checked before transfer-type computation. Refused readback and invalid metadata cases pass.

Associated teardown issue is corrected: `ccd_detach` sets `detaching` under the private mutex and stops/deletes the device queue before freeing any DSLR/CCD property or private mutex. The finalizer checks this flag before work/rescheduling, preventing a successor task from being added while queue deletion waits for a running callback. The last published image is released only after base detach. Reinspected source; no remaining freed-property/callback path was found in the assigned teardown scope. Canonical record reconciliation and closure remain coordinator-owned.

## Final USB ownership and capture reinspection

The automatic detach path is replaced by explicit paired ownership. The driver disables automatic detach, detaches interface0 only if a kernel driver is active, records `kernel_detached`, records `interface_claimed` only after successful claim, and routes open rollback and normal close through release/reattach/close. Failed claim after a successful detach therefore attempts to return the interface to the system. A failed reattach is logged, sets `usb_cleanup_failed`, surfaces an ALERT and blocks a subsequent open; it is not silently treated as a successful system-driver restoration. Physical removal errors are distinguished from a live-device cleanup failure. This does not require forcing a new user client or patching the system.

The final fake cases verify claim failure after detach reattaches without a spurious release, normal close orders interface release before reattach/close, and detach failure prevents claim. Independent code inspection confirms each new rollback after claim/LUN/mode initialization reaches the paired cleanup. No further physical test was executed by this reviewer.

Capture mode/shutter are read back before trigger, with nonzero rational operands checked before ratio comparison, so an invalid0/0 reply cannot authorize a capture. Output completion checks only the upload destinations selected for the current exposure, avoiding a stale inactive output error. Restoration-command failure retains `settings_saved` and refuses a fresh exposure until cleanup is retried instead of overwriting the original snapshot. The shared countdown is explicitly resumed after each successful trigger and progress after abort/restart is asserted by the public fake sequence.

## Independent execution

CWD `/private/tmp/indigo-complete-pentax`, macOS arm64, local copied INDIGO dependencies.

- `./.factory/scripts/security`: exit0; configured local-boundary validator passed. This structural check is not USB/capture coverage.
- `make -C indigo_test build/integration/test_ccd_pentax_usb_sanitize` then `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/integration/test_ccd_pentax_usb_sanitize`: exit0, all31 registered cases passed on the final partial candidate. Independent output `/private/tmp/pentax-independent-final-sanitize.log`; build output `/private/tmp/pentax-independent-final-sanitize-build.log`. Earlier26-case results are superseded by this final run.

The fake suite compiles the actual production implementation and exercises real public property/queue handling for capture, abort, retained BLOB and recovery. LeakSanitizer is unavailable on this macOS host; no whole-process leak certification is claimed. Explicit ownership assertions and source analysis support the repaired paths.

## Limits and remaining acceptance work

No hardware was run by this reviewer. Coordinator hardware attempts discover K-m but fail USB claim(-99), including the reported UID0 attempt; no physical exposure, format compatibility, setting restoration or timing success can be inferred. UID0 failure must not be presented as a missing-root-permission diagnosis. The coordinator reports that the earlier system mass-storage binding was restored with an external recovery helper; current paired-cleanup behavior is source/fake verified only. Physical unplug/replug was excluded by the owner. Other models, Linux/Windows runtime, native RAW decoding and physical bulb timing remain unverified.

Synchronous trigger/status and transport transactions may delay a scheduled bulb stop; describe the measured interval/limits rather than claiming exact physical shutter duration. Zero-type segment readiness currently consumes the descriptor retry budget; compatibility under delayed descriptor availability remains a functional validation gap.

The previously reported countdown issue is fixed/retested. Security sign-off does not waive physical acceptance, broader driver capability coverage, final quality or committed-candidate gates. Camera-setting restoration is verified by command error handling and synthetic state, not a physical readback demonstration. No residual risk was accepted and no complete-driver/hardware readiness claim is made.

Verdict: pass
