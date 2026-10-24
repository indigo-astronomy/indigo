# Security review

Verdict: pass
Scope: FND-000036/FND-000037 repairs.
Reviewer: separate `/root/review` agent; independent of implementation and test authoring. Quality and security were separate passes by this one reviewer.
Date: 2026-10-24.
Reviewed revision: HEAD/base `25c92ed424f747af067a2009981915c7f65b186a` plus the uncommitted changes and new tests. Git blob identities: `indigo_tools/indigo_prop_tool.c` = `011ddf561dd18b5295f87262567c1a28d43b193c`; `indigo_tools/indigo_raw_crop.c` = `be667c8291d585d131826de9f8fda6f3b9f604f3`; parser test = `6a1d7dae59c0bca5bd91e0de7600c9efb6e194b2`; Python runner = `d14245771ce82d10ff20e97b26f407c432c523da`.

Model/host: requested strong OpenAI `gpt-6-astra` / `high`, matching the explicitly read model policy and coordinator assignment. Independent runtime model/effort metadata is not exposed to this reviewer; observed values remain unknown. Host is Codex desktop with local shell/files/tests. No provider change, external probing or additional-charge action was performed.

## Scope and trust boundaries

Untrusted local command-line strings enter fixed property request buffers and a remote hostname/port parser. Untrusted local RAW headers, payloads and crop arguments enter allocation/offset arithmetic and file I/O. Review covered the changed paths and their immediate consumers. Inputs and test outputs are synthetic; no secrets or private data are involved. The task does not assess remote-server replies, server authentication, drivers or the wider repository.

## Risk coverage

| Area | Review result |
| --- | --- |
| Input and memory safety | Field lengths exclude the terminating NUL from capacity; counts are checked before array access. Empty-string trimming checks length before indexing and passes unsigned bytes to ctype. Parsing no longer overlaps source/destination buffers. |
| Remote endpoint | Host copies are bounded; empty/overlong hosts and malformed/out-of-range explicit ports return before connection. Host-only input leaves the caller's current/default port intact. No new endpoint selection or network capability is added. |
| RAW arithmetic | All supported signatures map to nonzero pixel sizes. Positive dimensions and checked size_t products precede allocation. ROI sizes are positive, conversions exact, and subtraction bounds prevent coordinate-addition overflow. Row-copy offsets stay within the validated complete image. |
| File access and failure behavior | Header and payload reads are checked. File length is verified before allocation, then the exact payload read is checked again. Allocation failures, output-open failures, short reads/writes and close failures return errors. Acquired buffers and streams are released on each examined path. |
| Authentication, secrets and cryptography | No relevant changes; not independently audited. |
| Dependencies/configuration | No new runtime dependency or security-setting change. Test subprocesses use argument arrays and isolated temporary paths. |

## Automated checks

Working directory for all reviewer checks: `/Users/polakovic/Development/indigo`. Host: Darwin 27.0.0 arm64; Apple clang 21.0.0 (`clang-2100.3.34.2`), Python 3.9.6.

- `SANITIZERS=address,undefined make -C indigo_test test-tool-input-safety`: passed all 11 cases after final test corrections, no sanitizer diagnostics. This instruments the changed tool translation units; the linked existing client library is not rebuilt with sanitizers. Existing deprecated-sprintf compiler warnings arise in unchanged response-formatting code and are not silently represented as a clean full-tool audit.
- `./.factory/scripts/security`: exit 0, configured local security-boundary validation passed. This structural Factory check is not an application vulnerability scanner.
- `git diff --check`: exit 0.

The focused suite checks maximum fitting/overlong property fields, MAX_ITEMS acceptance and excess rejection, ordinary whitespace/quotes, malformed strings, remote boundaries, all four RAW pixel formats, exact crop bytes, malformed headers/payloads, invalid ROI and synthetic allocation/read/write/close failures. It makes no network connection. Acceptance and test-author evidence are in [verification.md](verification.md) and [test-evidence.md](test-evidence.md).

## Finding dispositions

FND-000036: repair verified within the identified parser/endpoint scope. Bounded span copies and pre-index item checks replace the unsafe scan patterns, and the focused boundary cases pass under sanitizers.

FND-000037: repair verified within the RAW crop scope. Header/signature/payload validation, checked allocation sizing, positive/bounded ROI handling and checked I/O prevent the reported unsafe crop paths; valid formats retain exact bytes and negative cases return controlled positive error exits.

No additional unresolved security finding was identified in the changed paths. Coordinator owns canonical registry closure and final evidence binding. No residual security risk was accepted by this reviewer or inferred from a passing check.

## Limitations

This is bounded defensive validation, not proof of vulnerability absence. Windows/Linux execution, 32-bit builds, real allocation pressure and full OS I/O fault behavior were not exercised. `ftell`/`fseek` use the platform long range: files beyond that representable range or nonseekable input fail safely and are not claimed supported. The preexisting whole-image allocation strategy remains. The unavailable `docs/security.md` supplied no additional task-specific contract. No full repository audit, hardware certification, integration or publication is implied.
