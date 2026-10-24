# Verification report

Verdict: pass
Reviewer: root Codex agent, verifier role explicitly loaded
Date: 2026-10-24
Reviewed source: working-tree changes against f967c0032969361164be91f97fbe1400646407fb. Local master is unchanged and `git merge --no-edit master` reported already up to date before delivery. No remote fetch or publication was performed.
Independence: verifier also implemented changes; independent read-only quality/security reviewers inspected the actual diff and permission-transition correction. See quality-review.md and security.md.

## Acceptance evidence

| Criterion | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | Cached RO BLOB repeated update, named deletion, device deletion and EOF lifetimes; exact delivered bytes preserved. macOS leak inspection of the normal protocol suite: 0 leaks / 0 bytes. Generic driver RO ownership is unchanged. |
| AC-002 | pass | RO upload rejection/snapshot preservation; one-shot empty/nonempty consumption; XML second request does not dispatch stale staging; upload budget/failure/retirement regressions. Independent review identified and rechecked RO define, targeted define and BUSY/ALERT/IDLE transitions, WO restoration with no stale staging. |
| AC-003 | pass | Actual server file path with deterministic open/close counters: success, read error, premature EOF, body write failure and response-header failure each leave zero handles. |
| AC-004 | pass | Initial idle connection closes, established XML idle survives, eight-worker instrumented cap rejects excess and recovers; injected asynchronous worker creation failure reclaims socket/reservation. POSIX and Windows cleanup branches inspected; Windows not executed. |
| AC-005 | pass | XML/JSON item boundary and boundary+1, accumulated long-text wire budget below/above limit, sequential requests reset budget, repeated populated JSON arrays reject without dispatch. Existing large upstream BLOB and malformed EOF checks preserved. |

## Executed checks

All commands ran from `/Users/polakovic/Development/indigo` on macOS arm64, Apple clang, with existing dependencies. Native library build also emitted x86_64 slices; x86_64 execution, Linux and Windows runtime checks were not performed.

| Exact command | Exit | Result |
| --- | --- | --- |
| `make -C indigo_libs libindigo` | 0 | Server/client static and shared libraries built; repeated after final permission fix. No driver build or generator changes. |
| `make -C indigo_test test-server-security-sanitize` | 0 | Final 50/50 cases: protocol 18, cache 10, loopback HTTP 11, cache-allocation-failure 11. ASan/UBSan instrument parser/cache/server and portable listener. Short test-only deadlines/caps keep resource checks bounded. |
| `make -C indigo_test test-unit` | 0 | 315/315 cases in 21 suites, normal production limits, linked built library. |
| `MallocStackLogging=1 leaks --atExit -- indigo_test/build/unit/test_protocol_security` | 0 | macOS leaks: 0 leaks for 0 total leaked bytes; protocol suite passed. |
| `./.factory/scripts/check` | 0 | Diff whitespace, findings integrity, decision index, Factory boundaries passed. Rechecked after record reconciliation. |
| `./.factory/scripts/test` | 0 | 188 Factory regression tests passed. |
| `./.factory/scripts/security` | 0 | Configured local Factory security-boundary checks passed. |
| `make -C indigo_test test-clean` | 0 | Removed test build artifacts after validation. |
| `git merge --no-edit master` | 0 | Already up to date; included local target remains base above. |

Raw runtime results are retained in test-results.log. Build and preliminary command logs were local `/tmp/indigo-*.log`; this report retains commands, outcomes and limitations, not a claim that temporary logs persist. Required clean committed-candidate process/check gates are recorded externally after the final commit.

## Baseline and correction evidence

Initial existing sanitizer parser/cache suites passed before edits; HTTP baseline could not bind in the sandbox. Initial broader unit attempt likewise failed socket/pseudo-terminal cases under sandbox restrictions. No environment failure was counted as pass: authorized escalated reruns passed. Initial macOS leaks launch was denied task-port access inside sandbox; authorized rerun passed.

During implementation a new cache test incorrectly passed NULL device to a device-requiring update API, causing an instrumented fault; the fixture was corrected. Early tests also exposed stale metadata on writable redefinition. Independent final review generalized this to RO redefinitions/non-OK updates; synchronization was centralized and the full affected sanitizer/native suites rerun successfully. No test was weakened: the old HTTP expectation permitting in-flight upload to overwrite a newly RO snapshot was corrected to the AC-002 contract, independently reviewed, and now requires rejection plus unchanged snapshot.

## Limits and reconciliation

All five selected canonical findings are verified fixed and linked to this report/security.md. No hardware, third-party endpoint, deployment, whole-server certification or authentication redesign was exercised. Existing public BLOB identifiers select an item without client authentication; one-shot staging does not add per-client identity. Admission covers workers after first-byte detection, while initial timeout covers silent protocol selection. These preserved contracts are documented in PROTOCOLS.md. No property definitions were added/removed, so PROPERTIES.md requires no change.

## Authorized integration verification

Owner explicitly authorized merge to master, push and deletion of this completed task branch. Fetch advanced origin/master to 6a62fa7856480dd2db77dead66786ce3182ed47b (GIOTTO driver work). Merged that target into the existing task branch; one Xcode end-of-group conflict was resolved by retaining both complete groups, while Makefile changes merged disjointly. Independent /root/parser_review inspected both parents: original security sources/tests unchanged, all 34 other incoming files match origin/master, both sets of project/build references preserved. `plutil -lint indigo.xcodeproj/project.pbxproj` passed. Incoming Windows CRLF whitespace is pre-existing in origin/master; no unrelated reformatting performed.

On the merged working candidate, `make -C indigo_test test-server-security-sanitize` passed 50/50, `make -C indigo_test test-unit` passed 316/316 across 21 suites, and `./.factory/scripts/check` / `./.factory/scripts/security` passed. Commands ran at the same root/macOS host with escalated local socket permissions as above. Original five-fix C/test paths compare byte-for-byte to 5951a142a; no repeat leak inspection needed for unchanged source. Updated runtime output retained in test-results.log. Refreshed staged content identities cover merged Xcode/Makefile content. Additional merge commit is required to preserve published upstream ancestry and tested conflict resolution, not a rewrite of the original task commit. Final committed-range checks, fast-forward integration, publication and cleanup outcomes are reported externally.
