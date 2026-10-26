# Core change security triage

Verdict: pass

Review followed the scoped quality pass in `quality-core-review.md`. Reviewed candidate: base `a35823d61`, uncommitted task branch, exact core content identities listed in that report. Reviewer `editor_mapping` did not implement these numerical/raw/I/O/SHA fixes; own editor implementation excluded. Tier: inherited host model, bounded low-risk security triage; no substantive authentication or authorization redesign.

## Trust boundaries and findings

Reviewed externally supplied image buffers crossing the LibRaw wrapper, local image sampling and astronomy values, UDP socket lifetime, and SHA byte handling used by WebSocket handshakes. No secret, credential, authorization, path-access policy, outbound request, dependency or installation change is introduced.

No introduced security finding requiring remediation or risk acceptance was identified in the scoped diff.

- LibRaw allocation failure is handled before dereference/parameter mutation; it returns a defined existing error, logs no input contents and does not close a nonexistent decoder. Existing successful initialization/cleanup is preserved.
- Bayer and astronomy changes neither expand memory access bounds nor create new allocation or input handling. Numerical correction tests are local and synthetic. Existing cache synchronization and unrelated input-validation contracts are outside the change and not certified.
- The UDP close remains confined to the branch entered after socket creation and is now reached after both bind failure and successful/timeout reception. It cannot introduce a double close within this function; Windows uses closesocket and POSIX uses close. The fixture binds only a local ephemeral port and consumes no external traffic.
- SHA input bytes are converted to unsigned before shifts, and padding has unsigned type, removing the reviewed signed-shift UB. Existing digest/handshake semantics are retained for the verified vectors. This use is the protocol's existing handshake derivation; no new password/signature cryptographic design is introduced. Added synthetic vectors and existing WebSocket loopback fixtures do not probe third-party targets.

## Checks and limitations

Tools observed: Python 3.9.6; Apple clang 21.0.0 (clang-2100.3.34.2), arm64-apple-darwin27.0.0.

After the quality pass, ran `./.factory/scripts/security` from `/private/tmp/indigo-selected-audit-fixes`: exit 0, configured `python3 .factory/scripts/validate_project.py --security` reports Factory local security boundaries valid. This is workflow-boundary validation, not an application vulnerability scanner or proof of security.

Inspected sanitizer failure-to-pass evidence and independently reran the current numerical/raw sanitizer binaries as recorded in `quality-core-review.md`. The parent-owned final I/O run and whole-task verification remain separate acceptance gates. Windows/Linux and real devices/images were not executed. No broad server security review, penetration test, exploit workflow, full SHA implementation audit, or whole-repository security certification is claimed.

Next: coordinator aggregates this triage with independent lifecycle/editor review, final affected retests and finding reconciliation. No owner risk acceptance is inferred.
