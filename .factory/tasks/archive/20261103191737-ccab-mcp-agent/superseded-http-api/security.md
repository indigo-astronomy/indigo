# HTTP transport increment security review

Date: 2026-11-04
Reviewer: independent `http_review` subagent; same reviewer also performed the separate quality pass, without implementation/test edits.
Reviewed base: `67a7e06fd91254322307860b0ff4e26fda216ebb`, uncommitted first HTTP increment. Final reviewed path/blob identities are retained in [quality-review.md](quality-review.md).
Verdict: **pass for the scoped transport API**; no confirmed unresolved blocking security finding. This is not remote MCP, hardware-control or whole-server security certification.

## Trust boundaries and inspected behavior

Untrusted socket input crosses request-line/header parsing, declared-length body allocation and exact-handler dispatch. The registration context and callback code are trusted application-owned objects. No MCP endpoint, bus control, authentication scheme, outbound request or credential store is enabled by this increment.

- Reviewed header preflight and the new parser together: fixed line/header-block budgets, validated method/header tokens, preserved embedded CR for rejection, overflow-safe decimal Content-Length, duplicate-length rejection, and rejection of any Transfer-Encoding/unsupported Expect. Body reads are length-delimited and bounded by progress/total deadlines. Error responses contain fixed status text rather than reflected request data.
- Body allocation is capped at 1 MiB with at most 16 admitted buffered requests. Invalid/incomplete requests do not reach callbacks. Close-after-response avoids interpreting trailing bytes as another request. These bounds apply to the additive API, not all legacy listener work.
- New exact paths cannot alias suffix or encoded variants through implicit decoding. Header duplicates remain visible to application policy; the convenience helper deliberately returns only the first. The documented legacy fallback contract was inspected rather than interpreted as authentication.
- Registry references retain route/context lifetime through reception and callback execution. Route withdrawal/draining occurs outside callbacks and without locks needed by those callbacks; no new lock inversion or use-after-free was identified in this scope. Strengthened tests prove the active callback and incomplete-body drain cases.
- Request-line/header/body/response content is suppressed in new-handler transport logging; peek preflight does not log payload bytes. No actual secrets are present in fixtures or reviewed reports.
- The listener remains network-wide and this API supplies no access policy. Documentation explicitly makes authentication, visibility checks, bounded callback work and response size the integration's responsibility. No change is represented as making future MCP access safe by itself. Existing socket write timeouts are not an overall callback deadline.

No new dependency, process launch, file-write capability, generator change, driver behavior or public legacy ABI change was introduced. Windows portability was inspected at the API/source level only; runtime platform checks remain unverified.

## Checks and evidence

The reviewer independently executed `python3 -B .factory/scripts/validate_project.py --security` from the task worktree: exit 0, `Factory local security boundaries valid.` Independently executed `git diff --check`: exit 0. Tools observed: Python 3.9.6, Apple clang 21.0.0. These structural checks do not replace product review.

[verification.md](verification.md) owns exact acceptance/run commands and limitations. Reviewed retained HTTP regression log reports 23 passing cases, WebSocket log 5 passing cases; parent reports ASan/UBSan instrumentation of changed TCP code. The test writer reports the final 13-case buffered-handler suite passing natively and with ASan/UBSan. The reviewer did not rerun those network binaries. Initial sandbox binding/build limitations are recorded as limitations/retries, not passing checks.

No new confirmed security defect or risk acceptance is asserted; the coordinator owns canonical findings and all remaining gates. Missing full rebuild, untested Windows/Linux runtimes, general callback preemption, allocation-failure coverage and future MCP authorization/interoperability remain outside this increment. The owner requested inspection before commit, so no commit/publication/archival or committed-range result is claimed. This review is separate from the due recurring repository audit and changes no checkpoint.
