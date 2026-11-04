# Write-tools quality review

Date: 2026-11-04. Reviewer: independent `mcp_review` subagent on the same host/provider, no source/test authorship. The same reviewer performed the separate security pass. Only this report and write-tools-security.md are assigned writes; coordinator owns task state, handoff and findings lifecycle.

Verdict: **pass for the scalar set_property increment**, after FND-20261103191737-ccab-mcp-agent-003 correction and focused retest. No further concrete blocking quality issue identified. This does not close the unfinished task, authorize commit/publication or establish real-client/hardware compatibility.

## Candidate and scope

Original checkout `/Users/polakovic/Development/indigo`, task branch `task/20261103191737-ccab-mcp-agent`, HEAD `82e884b80e3713d49051020b5d6a4a50fa80202d` plus preserved uncommitted MCP/POST changes. Git blobs:

- `indigo_drivers/agent_mcp/indigo_agent_mcp.c`: `c484697a32924607c0861625e6999ed7ff98eea4`.
- `indigo_test/integration/test_agent_mcp.c`: `75da8e53e9102ff7f1f91ae2c2a4ff1c3f9e2a50`.
- `indigo_drivers/agent_mcp/README.md`: `6c48cf07cfb4ba704824cc6427286185c0366705`.
- `TEST_SUMMARY.md`: `b3e5ac5888b726ef16e54bf6f3aa2f1d42a1deb1`; `MIGRATION_STATUS.md`: `3de666fecb29b634629fe9e257b20ccbf6ad6d6d`.

Reviewed actual source, new/changed focused cases, README/catalog, specification/plan amendments and native bus/token/unload contracts. Earlier unchanged behavior is covered by [quality-review.md](quality-review.md) and [read-tools-review.md](read-tools-review.md); no unrelated repository audit or suite expansion occurred.

`set_property` validates exact schema targets, nonempty unique known items, per-type scalars and optional full64-bit hex tokens, builds an owned partial property and dispatches outside agent locks. JSON-escaped item aliases resolve through the same exact Unicode matcher before duplicate checks. Source inspection covers that alias combination; no dedicated escaped-item-duplicate runtime case is claimed. Framework/device rules retain ranges, steps, switch semantics and operational acceptance. The response reports a dispatch attempt only, including native lock rejections that return busOK. Self-thread shutdown checks occupied dispatch slots under service_mutex and returns BUSY before lifecycle mutation; normal external unload still drains callbacks. Native remove_driver honors BUSY and standard server driver operations are queued.

WO discovery was intentionally changed by the new specification: visible WO device/property/item schemas are discoverable; get remains denied. The prior whole-WO discovery omission expectation was therefore obsolete, not weakened to conceal a defect. Cache/snapshot copies scrub WO text and history, numeric values/targets/history, switch/light values and BLOB pointer/URL/size. Serializer emits only schemas for lists. No new public framework API, custom property, target denylist, authentication/configuration, hardware or BLOB upload/fetch path was introduced.

## Finding and correction

**FND-20261103191737-ccab-mcp-agent-003: medium functional, initially blocking.** Original numeric_value used indigo_atod. Independent local ctypes inspection of the existing library showed `0.1e309` became infinity despite the finite result1e308; `0.` followed by399 zeroes and `1e400` became0 despite the correct result1. This406-byte legal JSON number could silently send the wrong device value. The finite-only check could not detect that incorrect finite zero.

Minimal agent-only repair uses standard-C strtod, normalizes JSON decimal point to the current locale's full decimal separator, checks complete consumption and rejects nonfinite results. Reinspected numeric_value at line797, allocation arithmetic, suffix/NUL copying and lifetime. It adds no global/thread locale mutation, OS conditional or core change. Finite underflow/subnormal rounding remains permitted. Concurrent external process-global setlocale changes remain unsupported, consistent with existing libc locale use.

The test writer reproduced a pre-fix29/30 failure at the first scaled-number regression, then both scaled finite regressions passed with the repair. Final native30/30 and ASan/UBSan30/30 logs were inspected; binaries postdate the final source/test. A comma-decimal locale check also executed successfully on this host, per writer confirmation and no unavailable-locale diagnostic. Other cases cover full64-bit native tokens/no inherited master token, typed partial requests, WO/no-value behavior, Unicode/long text, invalid requests without bus effects, unsupported types, self-unload BUSY and own-CONNECTION disable. Runtime checks are attributed to the test writer/coordinator, not a fresh run by this reviewer. Independently inspected the retained [native](write-tools-logs/native.log.gz), [sanitizer](write-tools-logs/sanitize.log.gz) and final [recorded driver](write-tools-logs/recorded.log.gz) logs:30/30 after file release, matching the final README15:26 and migration30/0 records; evidence is retained in [verification.md](verification.md#write-tools-increment--baseline-and-found-defects) and write-tools-logs/.

The test's original9-hex-digit rejection expectation was corrected: indigo_token is64-bit and the catalog allows1–16 digits. Final tests reject17 digits and verify nontruncated64-bit device/master tokens. No production workaround was added for that fixture expectation.

Fresh reviewer `git diff --check` and `python3 .factory/scripts/validate_project.py --security` passed from the original checkout (Git2.54.0, Python3.9.6). No additional concrete finding identified; coordinator owns canonical FND003 disposition. Windows/Linux execution, real Codex/Claude GUI clients, physical hardware, allocation-fault injection and ThreadSanitizer remain unverified. This scoped review does not reset recurring-audit cadence.
