# Pentax completion quality review

Reviewer: independent in-host agent `/root/pentax_protocol_sources`. The reviewer researched protocol facts earlier but did not implement production code or tests. Reviewed base 64e965d7f and the software candidate retained at checkpoint 2c5744636 on task/20261026212631-884a-complete-pentax, followed by its owner-authorized delivery documentation revision. No production/test change occurs after that checkpoint. This is a scoped software-delivery quality pass, not physical camera acceptance or the overdue repository audit.

## Scope and method

Inspected the actual Pentax driver diff and surrounding lifecycle/property/image-pipeline code, extended fake USB suite, untracked opt-in hardware suite, Makefile/Xcode registration, property documentation and task specification/research. The implementation uses existing INDIGO queue, countdown and DSLR processing APIs. K-m/K2000 capture gating avoids asserting compatibility for unverified model layouts. Segment count/address/image-size bounds, new-buffer ownership and finite callback work are explicit. No new dependency, generator change, unrelated driver edits or copied copyleft implementation was found.

The synthetic suite compiles actual production code and exercises public property dispatch, real queues, native image publication, failure/recovery and preserved BLOB ownership, while retaining the original transport regressions. The hardware suite is opt-in and validates decoded JPEG/RAW payloads; it does not turn discovery or claim failure into a passing capture result.

## Repair reinspection

Two functional defects found during this pass were returned to the coordinator and repaired:

- Capture acknowledged program/shutter setters without verifying temporary readback before trigger. The revised exposure_handler now reads full state, verifies intended manual/bulb mode and positive timed shutter numerator/denominator plus rational equality before starting capture. The new public refused-exposure-mode case asserts ALERT with no shutter trigger, bulb start or image.
- Delivery checked both upload output states irrespective of the active upload mode, allowing a stale disabled-output ALERT to falsely fail a valid capture. The revised phase4 predicate checks only enabled client/local outputs. The public cross-recovery case exercises client download failure followed by local success, then local file failure followed by client success.

The final positive-rational guard also rejects zero/zero readback instead of allowing cross-multiplication to match zero. Coordinator owns canonical finding reconciliation and exact retest evidence.

## USB ownership reinspection

Reinspected the final explicit USB resource pairing. Auto-detach is disabled; a kernel driver is detached only when reported active, and ownership is recorded only after successful detach/claim. A shared cleanup helper releases a claimed interface, reattaches only an explicitly detached kernel driver and closes/reset the handle on failure and normal close. Reattach failure is reported and latched to block another open attempt instead of hiding unsuccessful system-driver restoration. Fake cases cover claim failure after detach, successful release/reattach/close and detach refusal with no claim. No system workaround, entitlement bypass or hardware command was performed by this reviewer.

## Evidence and limitations

Reviewed the latest retained fake-final.log and sanitizer.log: all 31 registered cases pass, including the positive-rational readback check and paired USB ownership cases. The coordinator owns final candidate build/check records. Reviewer independently ran git diff --check and plutil -lint indigo.xcodeproj/project.pbxproj; both passed.

Physical hardware acceptance remains unverified because of the macOS USB transport/claim failure, including the owner-authorized elevated attempt. No physical capture/settings/readout pass, physical unplug coverage, other-model imaging or other-platform runtime certification is established. The owner subsequently authorized software delivery now with hardware validation deferred to Linux; the failed hardware result remains visible and is not converted to a pass.

## Owner-authorized software delivery reinspection

Read the explicit owner instruction preserved in delivery-decision.md: commit, merge and push now, continue later on Linux. Reviewed the documentation-only diff from checkpoint 2c5744636: specification, test plan, verification, handoff, task state and REFACTOR, plus the new delivery decision and Linux follow-up. The revision consistently limits completion to the verified software scope, identifies physical acceptance as deferred rather than passed, retains the failed macOS record and preserves the hardware/software distinction. It does not conceal a security finding, claim that Docker sees the camera or broaden the recurring-audit deferral.

The Linux follow-up records real-host visibility/MSC prerequisites, preserved card/RAM images, settings restoration, limited-battery grouped testing, actual control/capture/abort/readout checks and mirroring hardware-discovered bugs into the fake backend. It does not create a new task or promise automatic monitoring. Software checks and committed-range verification remain required delivery gates; owner authorization covers the explicitly deferred physical gate only. No source/test changes require renewed behavioral testing for this documentation revision. Reviewer repeated git diff --check; it passed.

No unresolved blocking quality issue remains in the inspected software delivery changes. Exact acceptance status and executed checks belong to verification.md; security conclusions belong to security.md.

Verdict: pass
