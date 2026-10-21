# Audit plan

1. Inventory tracked paths and classify vendor, generated, data, archive, legacy and owner-protected exclusions; establish a per-component/per-file coverage ledger.
2. Capture unit/static/build-system/Factory baselines where safe. Driver integration requires an explicit mode and is not inferred from this audit request.
3. Inspect framework public APIs and ownership/I/O/protocol/image/resource boundaries; revalidate related legacy findings and audit tools/server/examples/platform sources.
4. Inspect every maintained driver/agent source-of-truth group, generator contracts, asynchronous operation patterns and property naming/visibility/documentation contracts. Review existing test coverage and harness integrity without changing tests to make them pass.
5. Inspect documentation/build/CI/Windows/system integration and Factory configuration/host loading/project instructions; triage candidates against callers/contracts and current code.
6. Record confirmed issues only, with reproducible source/evidence and accountable follow-up. Separate unsupported candidates, stale historical claims and environment limitations.
7. Reconcile findings, validate evidence/navigation, complete the audit report and separate quality/security passes. Advance maintenance only with actual full coverage; otherwise retain the live task and precise continuation.
