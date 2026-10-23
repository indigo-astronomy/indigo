# Security review

Verdict: pass
Reviewer: same Codex agent as implementation in a separate pass; no independence or exact model-tier claim. Existing owner-selected host retained; exact model/effort unavailable.
Reviewed source: included master c5a8b69ab55098f93e723f3d6ffc2a6cf5cae441 plus final task working-tree diff; Git content identities recorded in process.json.

## Boundary and analysis

Image geometry reaches public C helpers from callers. This repair validates required pointers before digest dereferences, rejects nonpositive radii/invalid dimensions, rejects nonfinite/out-of-frame rounded centers before narrowing to int, and ensures complete inclusive selection loops stay within the supplied frame geometry. Size checks avoid overflow of the former doubled-radius expression. Positive-radius border allocation uses size_t multiplication. PSF variance no longer narrows squared mono16 deviations into int or accumulates them in int. All rejected calls return before output mutations or image reads; no-output PSF compatibility is preserved.

The API still requires a caller-supplied buffer sized for the declared supported RAW format/frame; it has no buffer-length parameter. This is not certification of arbitrary forged buffer lengths, giant-frame arithmetic elsewhere, unsupported formats, iterative-wrapper input contracts, or the whole RAW subsystem. No authentication, dependency, process-launch, network, file-write, secret or logging boundary is changed.

## Evidence and disposition

The baseline sanitizer reports the mono16 floating-to-int overflow. Native 15/15 and the repeatable ASan/UBSan RAW target 15/15 pass after the repair, including one-byte invalid-ROI buffers, extreme radii and NaN/infinite coordinates. Apple clang 21.0.0. `./.factory/scripts/security` passes local Factory security-boundary validation; semantic conclusions come from source/test inspection, not that structural check. See verification.md for full evidence. No new blocking issue identified in this scoped pass; finding 006 is closed with its original discovery evidence and this task’s verified repair evidence retained.

The due audit deferral is owner-authorized in this chat. It neither resets the audit nor accepts residual security risk. No hardware, third-party probing, publishing, push or delegation occurred.

Final synchronized review also inspected maintenance.json and finding lifecycle changes. The owner expressly deferred this repair’s audit gate; existing 039/026 authority and audit cadence remain unchanged, with readiness-based integration superseding the former wait. No external service permission, risk acceptance, policy weakening or audit reset was introduced. Xcode changes only add navigation references. Native/sanitizer retests passed on the included target. No unresolved blocking security finding within this scope.
