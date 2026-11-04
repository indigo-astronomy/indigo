# Read-tools quality review

Date: 2026-11-04. Reviewer: independent `mcp_review` subagent, same host/provider; no production or test authorship. The same reviewer performed a separate security pass. Assignment permits only this report and read-tools-security.md; the coordinator owns handoff and finding lifecycle.

Verdict: **pass for the three read-tool increment**, after correction and reinspection of FND-20261103191737-ccab-mcp-agent-002. No unresolved blocking quality issue identified in this scope. This does not close the task, authorize a commit or establish real-client/platform interoperability.

## Reviewed candidate and method

Original checkout `/Users/polakovic/Development/indigo`, branch `task/20261103191737-ccab-mcp-agent`, HEAD `82e884b80e3713d49051020b5d6a4a50fa80202d`, with preserved uncommitted POST/MCP work. Reviewed actual agent source, focused test additions, README, acceptance amendments and related status metadata. Earlier POST, skeleton and tokenizer reviews remain in [quality-review.md](quality-review.md); their unchanged behavior was not audited again.

Git blob identities:

- `indigo_drivers/agent_mcp/indigo_agent_mcp.c`: `d4059a59cf626b707d4849e4050851a2116cb146`.
- `indigo_test/integration/test_agent_mcp.c`: `a0bea6a7878245a76e14d6c66c2aaa872f693ccf`.
- `indigo_drivers/agent_mcp/README.md`: `8a8104b41045528a024d4b895ee86d1b8512bdf5`.
- `TEST_SUMMARY.md`: `537319616a4c0dbc4458843ed72c911214eff565`; `MIGRATION_STATUS.md`: `b7fc5696689a8f1489b048e1163d42e1ebcca387`.

Inspected owned text/cache/snapshot allocation and cleanup, partial item-name merges, numeric value/target separation including explicit zero targets, schema redefinition, property/device deletion and reload enumeration. HTTP serialization occurs after releasing the cache lock. BLOB metadata uses the original published item address for local URLs and never owns the payload. List snapshots avoid long-text duplication; device snapshots are deduplicated before copying. Dynamic output preserves typed JSON, escaped UTF-8 names and text/structured-result consistency. No additional framework/property/control change is required by this increment.

## Finding disposition and retest

**FND-20261103191737-ccab-mcp-agent-002, medium functional, initially blocking:** original `call_tool` list snapshots forced count zero and `property_json(false)` omitted items, so `list_properties` violated the existing item-schema contract. Minimal fix retains item metadata without duplicating long text and emits names/labels plus numeric min/max/step/format and switch rule without values. Reinspected `copy_cached_property` (line85), `property_json` (line316) and `call_tool` (line710). The final numeric regression checks list item metadata and absence of value/target, while get retains values/targets. Both native and sanitizer suites pass that regression. Coordinator owns canonical fixed/archived disposition; no acceptance criterion was weakened.

The original test expectation that a published numeric target zero meant “absent” was corrected against the C bus contract: the item has no target-presence flag. The final test publishes33, then0, and preserves the untouched first item. No production workaround was needed. Synthetic CONFIG-auto-load and invalid BLOB-fixture corrections are test setup corrections, not MCP defects.

Inspected final retained test-writer logs [native](read-tools-logs/native.log.gz) and [ASan/UBSan](read-tools-logs/sanitize.log.gz):23/23 each, including the retained14 transport/lifecycle cases and9 read cases. Binary timestamps postdate final source/test. Tests cover visibility, intentional readable password/token inclusion,600KiB owned text and large output, BLOB URL identity/no-fetch, typed values/redefinition, delete/reload, escaped Unicode, concurrent reads/updates and argument failures. Also inspected the coordinator's final [recorded driver run](read-tools-logs/recorded.log.gz):23/23, exit0, matching the README/status rows. Runtime checks are attributed to the test writer/coordinator, not a fresh execution by this reviewer. Exact commands, retained logs, recorded driver run and build evidence belong in [verification.md](verification.md#read-tools-increment-baseline-and-developer-checks).

Fresh reviewer `git diff --check` and `python3 .factory/scripts/validate_project.py --security` passed from the original checkout (Git2.54.0, Python3.9.6). No unrelated suites, off-limits drivers, hardware or external services were exercised. Linux/Windows execution, real Codex/Claude GUI interoperability and allocation-fault injection remain unverified. This scoped diff review does not reset recurring-audit cadence.

## Final recorded-run identity refresh

The first recorded run preceded the test writer's final edit; the coordinator repeated the mandatory recorded runner after file ownership release. Reinspected the replaced [recorded log](read-tools-logs/recorded.log.gz):23/23, exit0, README timestamp15:08. Source/test identities remain unchanged; README and generated summary identities above now describe this final run. No source fix or new test scope occurred. Coordinator confirmed FND-20261103191737-ccab-mcp-agent-002 archived as fixed/retested. Scoped verdict unchanged.
