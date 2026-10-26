# Independent security review

Reviewed base: local master `aa8faa422`, task branch working tree. Final reviewed source blob: `a71dd8ab38a3afc1a8ad2bd84523e00e0fe65018`; fake USB test blob: `dd38ad6c4253b852dfa07b15ad216d080111a50e`. Reviewer is a separate in-host subagent and did not implement production code/tests. Actual model/effort metadata is unavailable; inherited host settings retained. No third-party service or hardware was contacted. `docs/security.md` does not exist.

## Boundary and assigned repairs

The relevant trust boundary is USB camera data entering native C code. Reviewed immediate open failure, SCSI CBW/data/CSW completion, readiness bytes, response-length narrowing, model-dependent state offsets, unsigned 32-bit decoding and unplug ownership.

- bc8d-020: immediate failed-open return precedes handle consumers and leaves NULL state; fake retry checks no invalid follow-on call/close.
- bc8d-021: both loops use a monotonic deadline. USB operations/retries remain individually bounded; five seconds is a polling budget with possible in-flight transaction overrun. Documentation avoids a hard five-second disconnect guarantee.
- 3bd2-024: all four response paths use private 65,535-byte storage, validating lengths before ushort conversion/read. Identity/firmware minima and every model-dependent field extent precede decode/update. Exact transfers and valid CSW signature/tag/residue/receive are required. Failed data still drains a bounded CSW before rejection. Internal fixed-size CDB callers supply trusted memcpy lengths. No remaining out-of-bounds decode path was identified in the changed helpers.
- 3bd2-025: capture-before-detach and free-after-detach/unref fixes ownership. Repeated removal cannot find a retained slot; fake detach clears the pointer, proving capture is used.

No authentication, authorization, secret, cryptography, network, filesystem-path, dependency or deployment policy changes occur. Existing valid command ordering and endian model behavior remain covered. No security risk was accepted.

## Independent execution

CWD: `/private/tmp/indigo-pentax-fix`, macOS arm64, Apple clang 21.0.0 (clang-2100.3.34.2), Python 3.9.6.

- `./.factory/scripts/security`: pass, configured local boundary validator; not a camera protocol scanner.
- `ASAN_OPTIONS=detect_leaks=0 indigo_test/build/integration/test_ccd_pentax_usb_sanitize`: pass initially 16/16 and after the descriptor repair 17/17. Coordinator built this binary from the reviewed source; independent output: `/private/tmp/pentax-independent-security.log`. Canonical build/run evidence is in verification.md.

LeakSanitizer is unavailable on this host; explicit free accounting verifies the assigned leak repair without claiming full leak certification. USB/time/detach boundaries are replaced; real asynchronous concurrency, physical hotplug, camera behavior and other-platform runtime are not certified. The overdue repository audit remains due under the owner-approved deferral.

## Additional finding and repair reinspection

FND-20261026210137-00fb-fix-pentax-findings-001 — Pentax hotplug consumes uninitialized USB descriptor strings. Severity: medium; confidence: high. Source: `process_plug_event`. The former uninitialized vendor/product arrays were consumed when USB string indices were zero or reads failed; `%s` logging/name formatting could read uninitialized/unterminated stack data. This was source-confirmed during review, with no hardware reproduction.

Coordinator repaired the finding within the all-Pentax-findings scope. Reinspection confirms both arrays start with valid fallback strings, successful reads reserve the final byte and explicitly terminate at the returned count, and empty/error/invalid counts restore the fallback. The added hotplug-descriptor case executes production plug/unplug with missing descriptors, read errors and maximum-length deliberately unterminated fake replies. It checks bounded fallback/full names plus balanced reference, config, allocation and detach/free ownership.

Independent rerun after the repair: configured security group passes; the rebuilt ASan/UBSan suite passes 17/17, including the hotplug case. No unresolved security blocker remains in this scoped final diff. Coordinator must preserve canonical closure and final process evidence; this review neither integrates nor publishes the change.

Verdict: pass
