# Scoped security review

Verdict: pass
Date: 2026-10-26
Reviewed base: a35823d61820273d653499f661faab78ae733b10 and final task source identities in process.json.
Review independence: independent in-host reviewers performed quality then low-risk security triage in security-core-review.md and security-lifecycle-editor-review.md. Root aggregates those reports and reviews build/registry metadata; root authored I/O/SHA/build changes, which received separate-agent inspection. No substantive authentication, cryptographic design or exposure-policy change requiring an expanded security investigation was introduced.

## Boundaries and evidence

Local device/private-data ownership: corrected base identities and per-clone shared allocation are covered by repeated real-bus lifecycle assertions. Guider callbacks stop before property releases; existing MKGP release is verified rather than duplicated. Tests fail closed if private-HOME isolation cannot be established and remove private temporary state.

Resource and allocation failure: discovery closes its socket for successful bind, timeout and failed bind on both platform branches; macOS runtime counter regression passes, Windows source-inspected only. Both LibRaw initialization callers stop before NULL dereference and retain existing error conventions. No sensitive payload or credential is logged.

Network/untrusted bytes: SHA byte assembly now uses unsigned shifts; known digests and WebSocket handshake pass halt-on-error sanitizers. This changes C arithmetic, not the SHA-1 protocol algorithm, authentication or authorization policy. No algorithm-wide SHA or whole-server security certification is claimed. All socket tests use controlled ephemeral loopback ports; no third-party target contacted.

Editor changes only map generated text using existing identities and function contexts. No generator execution policy, path allowance, host/origin/token check, disk-write boundary, dependency or public API change. Mapping tests generate only temporary output. Scientific/image arithmetic fixes preserve the existing cache/gain/transform interfaces; no new external-data transport.

The process data-model path signal is caused solely by MIGRATION_STATUS.md's test inventory count (88 to 89); no persisted application schema or data migration exists. Xcode edits add four source references to existing test groups only; no build membership or Factory references. Registry movements preserve original evidence and add verified repair references; no record deletion or risk waiver.

Configured ./.factory/scripts/security passes (factory-security.log), with Python 3.9.6. Source-including ASan/UBSan and scoped allocation counters support failure-path evidence; details and tool versions in verification.md. No new security finding or accepted risk in this scoped diff. Existing catalog follow-up is nonblocking quality tracking.

Limits: Linux/Windows execution, hardware, external service probing, full security audit and broad resource/leak certification were not performed. Copied external libraries are reused unchanged and not all instrumented. The overdue repository audit remains due.
