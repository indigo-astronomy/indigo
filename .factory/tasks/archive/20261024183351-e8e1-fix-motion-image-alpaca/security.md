# Security review

Verdict: pass

Date:2026-10-24. Scope: base463eb538daf440fa0f459b53a5f02fa949e244ed to final task candidate, three requested findings and bounded test/metadata changes. Independent same-host subagent inspected actual source/tests; parent capacity author runs verification and consolidates this report. Exact runtime model/effort unknown; current host/provider retained. No cross-provider upload or external probing.

Untrusted boundaries: discovered camera identities/counts, cached/raw BLOB shape and payload, and motion readbacks. Bayer insertion checks fixed capacity before computing a new pointer; existing cameras rebind to valid existing items and rejected mappings remain null. Synthetic full-capacity regression passes under ASan/UBSan. No invalid-memory pre-fix reproducer or exploit workflow was run.

Shared RAW validation copies the minimum header before inspecting fields, rejects unsupported/zero/unrepresentable dimensions and truncated source spans, bounds signed-offset consumers and preserves trailing metadata. All three consumers validate before reading pixels/allocating. Alpaca unlocks/releases retained BLOB entries on error; JSON/ImageBytes/GZip options are covered. ImageBytes length uses an unsigned sum; preview checks JPEG intermediate multiplication on32-bit arithmetic. These two review observations belong to existing022, not duplicate findings. Focused instrumented tests are in raw-repair.md and verification.md. LeakSanitizer/native32-bit execution remain unavailable.

Motion ownership distinguishes commanded endpoint checks from external motion and aborts, respects backlash/reversal/quantization and preserves failed targets. No new threads, resources, privileges, authentication rules, filesystem paths or outbound connections are introduced by production code. Tests use synthetic isolated homes, local files, PTYs, fake SDK and loopback UDP. Test queue marker references an in-process connected device; static atomic completion storage avoids callback stack lifetime issues.

Status-only MIGRATION_STATUS changes trigger the data-model risk signal: this file is a documentation table, not a persisted application schema or migration. Counts reflect registered synthetic/HW-case availability; Comment text is preserved. No dependency or deployment change. Generated versions are bumped and outputs remain reproducible with unchanged generator.

Configured `./.factory/scripts/security` passes local boundary validation; it is not a vulnerability scan of every driver or server. No secrets/private datasets are introduced. All requested security031/input-boundary concerns have scoped fixes and regression evidence; no owner risk acceptance is claimed. The independently tracked pre-existing functional race task001 is open, nonblocking and has its next assignment in findings.md.

Final scoped sign-off passes after complete final acceptance and independent quality/documentation reinspection; no whole-product, hardware, Linux/Windows or server-security certification is implied.

## Authorized integration continuation

Origin/master advanced independently after the original candidate. Its synchronization preserves c5193c103 and all original task source/test objects; only Xcode/summary metadata overlaps. Independent integration review passes and focused combined-candidate checks pass; exact evidence, authorizations and concurrency merge exception are in integration.md and integration-review.md. Process content identities for shared metadata are refreshed before the integration merge commit. Original review scope/limitations remain intact.
