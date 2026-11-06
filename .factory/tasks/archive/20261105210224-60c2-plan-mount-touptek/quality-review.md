# Task quality review

Verdict: pass
Reviewer and independence: fresh general-purpose subagent (same host and model), read-only, independent of the implementation context
Reviewed revision and working-tree state: base c180d7b32 plus the uncommitted task diff (2026-11-06). Findings were fixed afterwards and re-verified by the implementer.

## Scope and method
The diff of `indigo_mount_lx200.driver` and its generated `.c`, the simulator, the test suite, `PROTOCOL.md`, `PROPERTIES.md` and
`MIGRATION_STATUS.md`. The review was static, compared against protocol.md and specification.md, and checked against the root AGENTS.md
formatting, C and generator rules. It looked in particular for regressions in the 15 existing dialects.

## Quality findings
| ID | Severity | Location | Finding | Disposition |
| --- | --- | --- | --- | --- |
| Q1 | medium | driver `meade_update_touptek_state`, home admission | A home search started at home could complete on the first poll, because `H` with `N` was still reported before the mount moved. The simulator modelled `:MFH#` like `:MH#` and finished instantly, so the test could not catch it | fixed: home pending/moved latch like the park one. The simulator search now moves (about 2 s). The test asserts the search lasts more than 1.5 s |
| Q2 | low | driver home | No timeout for a home or search the mount never starts | fixed: `TOUPTEK_HOME_START_TIMEOUT` → ALERT "did not start to move"; test with a dropped `:MH#` |
| Q3 | low | driver `:SBz` | Read as a one-character reply, while S1 lists none | kept: S5 (TMount, authoritative) waits for a reply. A missing reply costs one timeout, and the `:GBz#` readback shows the real state |
| Q4 | low | driver guide rate | Set with 2 decimals, read back with 1 | fixed: step 10 (0.1×), so the set value survives the readback |
| Q5 | low | driver park | A failed `:MP1#` write left the park pending, which produced a second alert | fixed |
| Q6 | low | driver guide rate | Duplicated range clamp | removed; the framework validates 10–90 step 10 |
| Q7 | low | simulator | Two blank lines between functions | fixed |

These are review findings fixed within this task before delivery, so no canonical FND records were created.

## Fix reinspection
Q1, Q2, Q4 and Q5 are covered by `lx200_touptek_home_and_home_search`, `lx200_touptek_rates_buzzer_and_meridian` and the park cases. See the
retest evidence in verification.md. Confirmed sound by the reviewer:
- No behavior change for the other 15 dialects. The shared changes are guarded by `count == 3` or the TOUPTEK item, and `count == 2` → `count >= 2` is equivalent for them.
- The UTC sign in both directions and the east-positive longitude.
- The `:MS#`/`:CM#` reply handling and the `:GPS#`, `:Gm#` and `:GT#` parsing.
- The bounded `strcpy` from the 128-byte response buffer.
- The generated `.c` matches the `.driver`, and the version is 83.

## Quality verdict
No open blocking issues after the fixes. Remaining limitation: the simulator models SH20 replies that the hardware has not confirmed (D6).
