# Verification report

Own acceptance results, exact executed checks, baseline comparisons and retest evidence here. Quality: quality-review.md. Security: security.md.

Verdict: pass
Reviewer: the implementing session (Claude, Claude Desktop Code tab, claude-fable-5-1 after a mid-task switch from claude-opus-5-5)
Date: 2026-11-01
Revision and working-tree state: uncommitted diff on top of 90e8424bd (origin/master merged into the task branch), worktree /Users/polakovic/Development/indigo-alpaca-guider, macOS 27.0.1 arm64
Review independence: the verifier implemented the change; the diff review in quality-review.md is by a separate subagent.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `paired_guider_follows_alpaca_client_only` steps 1–2; failed before the fix at line 147 (expected 0, got 1). |
| AC-002 | pass | same test, `Connected=true` step. |
| AC-003 | pass | same test, late pairing after the request and late pairing during BUSY; removing the `!connection_busy` clause fails the BUSY step (line 183: expected 4, got 3). |
| AC-004 | pass | same test, client disconnect, foreign disconnect and ALERT steps. |
| AC-005 | pass | server scenario below: guider stays disconnected after the mount selection, Guider Agent selection ends OK, the only guider CONNECTION requests come from `Guider Agent` and, after `PUT connected=true`, from `Alpaca Agent`. |
| AC-006 | pass | `DRIVER_VERSION` 0x03000011 → 0x03000012 (3.0.0.18). |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make init`, `make -j8` in indigo_libs, `make -f ../../Makefile.drv` in agent_alpaca, mount_synscan, agent_mount, agent_guider, ccd_simulator, `make -j8` in indigo_server and indigo_tools | worktree, before fix on cf9496e26 and again after the merge of 90e8424bd | 0 | Builds clean, no warnings in agent_alpaca. |
| `make -C indigo_test test-agent-alpaca-mapping` | worktree, cf9496e26 + new test, before fix | 2 | Baseline: `paired_guider_follows_alpaca_client_only` FAIL at line 147 (expected 0, got 1), existing case PASS. |
| `make -C indigo_test test-agent-alpaca-mapping` | worktree, 90e8424bd + fix, 17:49 CET | 0 | 2/2 PASS; three consecutive binary runs all passed. |
| `make -C indigo_test test-agent-alpaca-cache` | worktree, 90e8424bd + fix | 0 | 3/3 PASS. |
| `make -C indigo_test build/integration/mount_synscan_simulator` then `indigo_server -- -p 7625 -b- -vvb indigo_mount_synscan indigo_agent_mount indigo_agent_guider indigo_agent_alpaca` with temporary HOME, `indigo_prop_tool` selections, `PUT /api/v1/telescope/0/connected` | worktree build, 17:49 CET | 0 | Mount selected 17:49:40 → guider DISCONNECTED; guider selected 17:49:47 → `FILTER_GUIDER_LIST = OK`; `Connected=true` 17:49:59 → guider CONNECTED by `Alpaca Agent`. |
| Same scenario before the fix | main checkout master build, 17:31 CET | — | Reproduction: `Change 'Mount SynScan (guider)'.'CONNECTION' ... 'Alpaca Agent'` 4 ms after the mount, `FILTER_GUIDER_LIST = ALERT`. |
| `indigo_server -- -p 7625 -b- -vvb indigo_ccd_simulator indigo_agent_alpaca`, connect `CCD Guider Simulator` | worktree build, 17:46 CET | 0 | `(guider)` and `(AO)` stay DISCONNECTED, no other CONNECTION change in the trace; camera and AO guiders are not paired by the agent (`pair_guider()` requires a mount). |
| `./.factory/scripts/check` | worktree | 0 | Factory boundaries valid. |

## Baseline comparison and regressions
Before the fix the new test fails and the server scenario reproduces the report; after it both pass. Existing `agent_alpaca` tests unchanged and passing. No other driver tests were run, per the owner's narrow-test preference; the change is confined to the agent.

## Found defects
FND-20261101163708-4785-fix-alpaca-guider-autoconnect-001: `indigo_alpaca_connect_paired_guider()` connected `<mount> (guider)` on every mount CONNECTION update once paired, regardless of the requesting client, so the Guider Agent refused the guider as busy. Fix: `connection_requested` flag set in `alpaca_set_connected()`, cleared when CONNECTION settles disconnected or ALERT, required by the paired connection. Regression test: `paired_guider_follows_alpaca_client_only`.

## Manual checks and limitations
- Not run: ConformU, OmniSim, `system_alpaca` driver tests, hardware. The `system_alpaca` late-attach case is covered by the BUSY pairing step of the unit test, not through that driver.
- Nonblocking limitations from quality-review.md: a tiny race between the request flag and an early non-BUSY CONNECTION update from another thread (relevant to remote mounts with late guider pairing only); a client that exits without `connected=false` leaves the flag set while the mount stays connected; an Alpaca client that never sends `PUT connected=true` no longer gets the guider connected (accepted in the specification).
- Whether the forum reporter has the Alpaca Agent enabled is unconfirmed.
- Test totals: simulated tests run 5, passed 5 (2 mapping, 3 cache) plus 2 server scenarios passed; hardware tests run 0, passed 0.

## Verification verdict rationale
All acceptance criteria have passing evidence on the merged candidate, the regression test fails without the fix and without the BUSY clause, and the reproduced defect no longer occurs.

## Register reconciliation
FND-20261101163708-4785-fix-alpaca-guider-autoconnect-001, status in the findings register; evidence pinned to this report.
