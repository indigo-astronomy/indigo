# Security review

Verdict: pass
Scope of verdict: PASS for the offline USB acquisition/rollback repair. Physical USB restoration and resolution of the K-m -99 failure remain unverified; this verdict is not hardware acceptance.

## Scope and independence

Reviewed base 34d0c2869422978a012f84c218d39eae05ba5f7c plus the uncommitted final driver/library/test/documentation diff on task/20261026223438-4f82-pentax-terminal-validation. The reviewer previously performed read-only backend analysis, but did not implement the production or test changes. The independent security-reviewer role and applicable INDIGO/libusb rules were loaded. Write scope for this assignment is this report only.

The changed trust boundary is the transition from application-owned libusb handles to macOS USB capture/interface ownership. The owner authorized the scoped repair and explicitly required the camera remain asleep. No hardware access, administrator action, entitlement change, system installation, third-party contact or dependency download occurred during this review. The library patch stays in the vendored Darwin backend and preserves the public API/ABI.

## Findings and safeguards

No new blocking security finding was identified in the reviewed diff.

FND-20261026223438-4f82-pentax-terminal-validation-001, the previously recorded functional ownership defect, is fixed in the reviewed software. The Darwin auto-claim wrapper records whether its own detach operation returned success. If raw interface claim then fails, it invokes the locked attach helper exactly once under the cached-device lock. It preserves the original claim result and logs rollback failure. Failed detach and no-binding paths do not run an unowned attach. Existing capture reference counting retains other outstanding captures. This addresses the known failed-claim leak without using kernel-presence heuristics to invent ownership.

The driver restores the original auto-detach/claim acquisition order while preserving checked open, exact LUN validation and existing protocol bounds. It releases only an interface recorded as claimed and does not perform a second manual attach after automatic release. Any failed claim latches cleanup uncertainty because the public claim result cannot expose rollback failure; release errors and an absent/unknown expected binding also latch uncertainty. The next open is rejected until driver restart, so an apparently disconnected property does not silently authorize a repeated capture attempt in an uncertain handle state. Driver restart is a software reset requirement, not proof that the operating system recovered its original binding.

The release check using libusb_kernel_driver_active is limited: the current Darwin implementation checks whether an interface has an IORegistry child. A result of 1 does not identify the exact original Mass Storage driver. The check is a conservative failure signal, not independent proof of physical restoration. Actual OS binding restoration remains a hardware follow-up.

The narrowly shared Darwin backend behavior changes all failed auto-claim callers, not just Pentax. The change is confined to releasing capture acquired by the failing call. No public declarations, caller return codes, privileges, authentication behavior, file paths or external input parsers change. No secrets or sensitive camera image data were added to the diff.

## Evidence

Working directory: /Users/polakovic/Development/indigo. Commands run by this reviewer:

- `./.factory/scripts/security`: exit 0; configured local Factory security boundary validation passed. This is a repository metadata check, not a native USB scanner.
- `git diff --check`: exit 0.
- `python3 --version`: Python 3.9.6.

Inspected production code in indigo_drivers/ccd_pentax/indigo_ccd_pentax.c and indigo_libs/externals/libusb/libusb/os/darwin_usb.c, including capture ownership, lock coverage, release behavior and kernel-activity semantics. Inspected the actual-wrapper extraction in indigo_test/Makefile and fake USB cases in indigo_test/integration/test_ccd_pentax_usb.c. The generated include is build-only and tests the production wrapper rather than copying its rollback logic.

The separate test agent's retained auto-baseline.log reproduces missing rollback before repair. auto-full.log and auto-sanitize.log show all cases passed; the coordinator records 36/36 normal, 36/36 ASan/UBSan and the canonical recorded run in verification.md. Those runs were inspected as evidence, not rerun or claimed as this reviewer's independent execution. Build/local-library installation evidence is also linked from verification.md.

## Limits and next action

The fake backend cannot establish real IOKit ownership, device re-enumeration timing, a physical privilege requirement or successful image capture. No current-version hardware run was attempted during this repair. The older 3.0.0.3 failures remain evidence and must not be replaced by a passing physical result. Linux/Windows runtime checks, macOS LeakSanitizer and power-loss/hotplug validation were not performed. The existing owner instruction to avoid physical USB unplug remains in effect.

Coordinator: retain the finding's regression evidence, reconcile its fixed disposition, finish final-diff quality review and record task state honestly. Before any physical retry, notify the owner and obtain camera readiness. Report restored source behavior and verified software contracts separately from the still-unverified physical -99 cause.

## Final software-delivery recheck

Rechecked the final specification and delivery-decision.md following the owner's 2026-10-27 explicit commit/merge/push instruction. The acceptance scope is the reviewed software repair; previous physical failures and current physical uncertainty remain retained. This authorization is delivery approval, not a claim that hardware passed. No additional camera access occurred.

The MIGRATION_STATUS.md addition is only the Pentax coverage/status table row (36 simulated cases and 1 available hardware case). Despite the process path classification, this is not a data-model or stored-data migration: no schema, stored record, user data, conversion, deletion or migration execution changes. The HW case count does not claim it passed.

Also rechecked the subsequent indigo_libs/Makefile source prerequisite: the existing macOS libusb dylib target now depends on the patched Darwin source so an ordinary build cannot silently retain the old backend. It reuses the existing configure/build/install recipe and local build destination. No new dependency package, installer, network source, public API, ABI or privilege grant is added. The reviewed native behavior is otherwise unchanged.

Final scoped security verdict remains PASS for software delivery, with the physical-validation and runtime limitations above. Independent quality review records the dependency prerequisite fix and no remaining blocking quality finding. Coordinator owns final process checks, registry disposition, archival, commit/integration/publication and branch cleanup.
