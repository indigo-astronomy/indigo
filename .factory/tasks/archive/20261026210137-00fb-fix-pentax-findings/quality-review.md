# Pentax final-diff quality review

Reviewer: independent in-host Codex subagent `/root/pentax_quality`; did not implement production or test changes. Reviewed local base `aa8faa422` and the uncommitted task candidate on `task/20261026210137-00fb-fix-pentax-findings`, including the untracked test and REFACTOR document. This is a task-diff review, not a recurring full-repository audit.

## Inspected scope and method

Read the task specification, test plan, handoff and verification evidence; inspected the actual driver diff and surrounding SCSI, response decode, open/close and unplug code. Reviewed the full new fake USB test, normal/sanitizer Makefile rules, Xcode file/group additions, testing README/summary records, REFACTOR coverage/limits and scoped maintenance deferral. New Xcode entries reference only the test and driver document; Factory records remain outside the project.

The implementation checks failed open before handle use, bounds both readiness polling modes with monotonic time, centralizes bounded response reads before 16-bit narrowing, validates every full-state field extent before state updates, rejects incomplete SCSI stages/invalid CSW and captures private ownership before detach. Private response storage is bounded by the existing wire length representation. Removing the former separate read success gate accounts for the full-state indentation diff; the decoded valid-state operations retain their order.

The tests include the actual production C implementation rather than copied decoder logic. USB/time interception provides deterministic readiness and malformed-reply cases; detach/free interception proves the specific allocation survives until detach and is then released. Original-driver safe regression failures are distinguished from preserved success cases. Table-driven boundary tests cover each model/field, with independent field widths and accepted exact-layout scenarios. Test registration and machine-readable runner use are consistent with repository conventions. The README changes are limited to the authorized generated testing record.

No new dependency, generator modification, unrelated driver edit, API/property addition, secret or publication action was found. The code/test complexity is proportionate to the four defects. No confirmed new quality finding or unresolved blocking issue was identified.

## Evidence and limits

Acceptance results and exact baseline/repaired commands belong to [verification.md](verification.md#executed-commands-and-results); this review inspected those records rather than rerunning the driver suites. Reviewer independently ran `git diff --check` and `plutil -lint indigo.xcodeproj/project.pbxproj` from the task worktree; both passed. Configured Factory regressions and committed-candidate process validation remain coordinator completion gates.

The test suite is intentionally helper/lifecycle-level: it does not establish public-bus concurrency, complete camera capabilities, physical hotplug or hardware/platform certification. The five-second polling budget can be exceeded by an already-running bounded SCSI transaction, as documented. LeakSanitizer availability is not overstated; explicit free tracking is the leak regression evidence. Existing under-development camera limitations are outside this repair. The owner-approved overdue-audit deferral remains scoped to Pentax and does not reset the audit checkpoint.

## Security-repair reinspection

Reinspected the additional FND-20261026210137-00fb-fix-pentax-findings-001 repair in `process_plug_event` and its seventeenth registered test. Manufacturer/product arrays now have initialized fallbacks; libusb receives one byte less than capacity, positive valid counts receive explicit NUL termination, and failed or invalid counts restore the appropriate fallback. The unchanged configuration/attach sequence consumes only terminated strings. The fake hotplug fixture executes production plug/unplug code with absent string IDs, failed string reads and nonterminated maximum requested-length replies; it checks the resulting name and balanced config, USB reference and private-state ownership. The new interception is limited to the USB/bus boundaries under test.

The additional repair is a directly related camera-provided string boundary found during security review. It adds no dependency or broad refactor. The final recorded suite reports 17/17; the coordinator must refresh all aggregate counts and sanitizer evidence before task closure. Prior housekeeping suggestions (unrelated Unicode serialization churn and inserted blank lines) are resolved. Repeated `git diff --check` passed. No new quality blocker was found.

Verdict: pass
