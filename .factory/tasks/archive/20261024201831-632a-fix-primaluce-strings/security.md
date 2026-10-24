# Security review

Verdict: pass
Reviewer: independent Codex subagent review_contract
Date: 2026-10-24
Reviewed source: base caa4c7787 plus current scoped working tree on task/20261024201831-632a-fix-primaluce-strings.
Independence: separate from the implementer; this reviewer also performed the quality pass. No whole-repository or whole-server security claim is made.

## Trust boundaries and analysis

Scope is untrusted controller firmware strings entering the fixed-size INFO text item and client WiFi values entering serial/HTTP JSON requests. Credentials in tests are synthetic. Authentication, authorization, dependency, filesystem, deployment and cryptography behavior do not change.

FND-20261024121920-3bd2-repository-audit-029: the existing INDIGO_COPY_VALUE initializes and terminates all 512 bytes of the firmware item, leaving strlen at most 511. The new snprintf receives the remaining capacity, at least one byte, and a non-overlapping source in the protocol response. It cannot append beyond the item. Reading the web field sends no nested request, so the application value and web pointer remain valid.

FND-000052: the encoder handles quote/backslash with two bytes and every byte below 0x20 with a six-byte Unicode escape, preserving other bytes including UTF-8. Its reserve-one-byte guard prevents overflow; callers supply nonzero fixed buffers. Each 3072-byte encoding buffer supports the worst-case 511-byte property value plus terminator. Both encoded lengths and the fixed JSON envelope are checked before calling the transport; short-circuit failures neither send a partial request nor read an unterminated failed output. The two internal templates contain exactly two %s conversions, so subtracting four template bytes gives the actual formatted request length. No encoding failure logs credential contents, and ordinary request envelopes remain unchanged.

No new security finding was identified in this diff. The two requested findings are technically repaired; coordinator owns their canonical closure after complete acceptance evidence.

## Checks and evidence

Reviewer ran ./.factory/scripts/security from /Users/polakovic/Development/indigo: exit 0; configured validate_project.py --security reported valid Factory local security boundaries. This checks repository boundaries, not product protocol correctness.

Reviewer inspected /tmp/primaluce-fixed-unit.log and /tmp/primaluce-sanitize-unit.log: 32 unit cases pass, including escaped credentials, full property expansion and rejected oversized encoding followed by successful transmission. /tmp/primaluce-sanitize-firmware.log shows all three oversized firmware scenarios pass. The sanitizer build recipe instruments the actual shared protocol implementation and generated focuser source with AddressSanitizer and UndefinedBehaviorSanitizer; linked framework code is not fully instrumented. Original same-profile runs crashed with signal 5. Exact retained commands/results and tool versions are owned by [verification.md](verification.md).

## Limitations

Full recorded simulator/motion verification and final evidence/documentation were still pending at this review; this scoped security verdict does not replace those acceptance gates. No physical controller, Linux/Windows runtime or live HTTP controller validation was performed. The review preserves existing credential transport/logging behavior beyond the requested encoding fix and does not certify unrelated protocol parsing. No risk was accepted or security gate waived. Reinspect final evidence before closure.
