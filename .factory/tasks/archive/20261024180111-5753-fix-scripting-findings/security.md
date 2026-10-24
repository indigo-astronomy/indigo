# Security review

## Scope, revision and independence

Independent reviewer subagent; no implementation authorship. Reviewed `166362b61af463ab25c097a751f873c9372afffc` plus working source changes in the native scripting agent, Sequencer and associated integration tests. Reviewed the bus deep-copy/release contract and synchronous bus dispatch, native queue enqueue/removal/deletion, and the agent shutdown ordering. Trust boundary: script-originated property values enter native storage and deferred bus publication. Tests use local synthetic values; no external targets or private datasets were involved.

## Assessment

FND-20261024121920-3bd2-repository-audit-035: the queued object no longer aliases mutable cache allocation. The four supported vector types use the shared property copier; long text receives private storage. Dispatch unlinks the tracked allocation before publishing and releases it afterward, once synchronous consumers return. The list mutex is not held across bus callbacks. Queue deletion precedes cleanup of undispatched copies, so canceled tasks cannot retain allocations or later consume freed wrappers after the worker joins. Failed queue creation also leaves the tracked allocation available for shutdown cleanup. No shared queue semantics were changed.

FND-20261023101753-bc8d-complete-repository-audit-046 is a functional aliasing defect. Its batch snapshot change copies own numeric values and adds no external input sink, permission change, dependency, logging or network behavior. Authentication, cryptography, filesystem paths and outbound request handling are unchanged and outside this repair's security claims.

## Checks and evidence

Reviewer ran `./.factory/scripts/security` from `/Users/polakovic/Development/indigo`: exit 0, “Factory local security boundaries valid.” This configured check validates Factory boundaries; it is not a C memory-safety scanner. Reviewer also ran `git diff --check`: exit 0. Runtime regression/build/sanitizer evidence is owned by [verification.md](verification.md), reviewed at final reinspection.

## Findings and verdict

No additional confirmed security defect in the reviewed diff. Source assessment supports resolving the assigned lifetime finding after acceptance verification and registry reconciliation.

Verdict: pass

Final evidence review confirms native 36/36 under ASan/UBSan with the documented instrumented-driver/uninstrumented-static-dependency split and ASan-only Duktape. Leak detection was disabled. The on-unload case is useful shutdown execution coverage; absence of retained snapshots follows the queue-join/list-cleanup ownership inspection, not a leak-detector result. The all-type callback value regression includes long text, and public focus restoration coverage passes. Final normal recorded suite is 63/63. Exact commands and outputs remain in verification.md and evidence/. Runtime results are coordinator-supplied and were inspected rather than independently rerun. The post-compilation AI notice is comment-only. Native test-helper use of the raw API correctly prevents script text from being interpreted as a format string. No new security finding or risk-acceptance waiver is required for this diff. No universal absence-of-vulnerabilities, platform, hardware or whole-agent lifecycle certification is claimed. Unrelated pre-existing owned queue payload handling remains outside this bounded snapshot change.

## Archival metadata reinspection

Reviewed the post-review REFACTOR evidence-path correction to `.factory/tasks/archive/20261024180111-5753-fix-scripting-findings/verification.md`; the retained target exists. Current REFACTOR SHA-256: `63a02dbf11961a0dd5a8c892544e42d7c230cadfccbcb69cec47d37a34301056`. This changes only the retained evidence location and leaves the pass verdict unchanged. Targeted inspection of both archived finding records confirms closed/fixed dispositions with repair-task evidence; the Xcode navigator diff moves their existing references from active to archive groups without source/build changes. Original finding descriptions remain historical evidence; current disposition is their resolution field.
