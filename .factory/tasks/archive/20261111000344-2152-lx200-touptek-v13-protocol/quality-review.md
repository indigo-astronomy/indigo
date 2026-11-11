# Task quality review

Verdict: pass with non-blocking findings (Q2, Q3, Q4 partly, Q5 and Q10 fixed; the rest are recorded)
Reviewer and independence: a quality-reviewer subagent in a fresh context (claude-opus-5-5), read only. It did not implement the change. The coordinator applied the fixes afterwards.
Reviewed revision and working-tree state: master `da414d9fd` plus the uncommitted task diff of the five product files, before the fixes listed under Fix reinspection.

## Scope and method
The reviewer:
- read the final diff against HEAD of `indigo_mount_lx200.driver`, the generated `.c`, the simulator, the test source, `PROTOCOL.md` and `PROPERTIES.md`;
- checked it against the V1.3 PDF text (`pdftotext`) and the root and driver `AGENTS.md`;
- regenerated the `.c`/`.h`/`_main.c` from the working-tree `.driver` with the unchanged `build/bin/indigo_generator` in a scratch directory. The output was byte-identical to the working tree.

The reviewer did not build or run the tests.

## Quality findings
| ID | Location | Finding | Severity | Disposition |
| --- | --- | --- | --- | --- |
| Q1 | `.driver` `meade_slew` | Firmware before V1.3 answered `:MS#` `0#` for success, so on such firmware a goto that started is now reported as rejected. | non-blocking, owner decision | Reported to the owner. V1.3 is followed by the owner's instruction; no version switch was added. |
| Q2 | `.driver` `meade_unpark` | With the mount silent, the confirmation loop could hold the device queue for up to 10 × the reply timeout. | non-blocking | Fixed: the first failed `:GPS#` read ends the wait (V1.3 "a failed query means unknown state"), and there is no sleep after the last read. |
| Q3 | `.driver` `meade_unpark` | Every `:GPS#` reply other than `0`/`2` was reported as "parking". | non-blocking | Fixed: `1` gives "parking"; any other reply gives "Unpark refused, unexpected park state …". |
| Q4 | tests | No case for `:GHS#` 0 after busy, the busy codes 3/4/5, or unpark at `:GPS#` 0 or 1. | non-blocking | Partly fixed: `lx200_touptek_home` now covers a home that ends with `:GHS#` 0 after it was busy. GPS 1 is unreachable through the client, because `unpark_allowed` requires not parking. The GPS 0 and 3/4/5 branches remain source-audited only. |
| Q5 | test, OnStep abort case | The abort-during-home behaviour change was not pinned. | non-blocking | Fixed: `lx200_onstep_park_and_home_abort_settle` asserts `AWAY`. |
| Q6 | simulator `:MS#` | `:MS#` sets tracking on; V1.3 says a goto keeps the tracking state. | non-blocking, out of scope | Recorded. It belongs with the out-of-scope "tracking after goto" gap. |
| Q7 | simulator `:GHS#` | The model is coarse: it reports 1 while busy and never 3, 4 or 5. | informational | Recorded. |
| Q8 | `indigo_drivers/mount_lx200/README.md:44` | The README still describes the "Search" item. | non-blocking | Proposed to the owner; README changes need approval. |
| Q9 | `.driver` property names | The renamed `X_TOUPTEK_MERIDIAN` items, the removed `X_TOUPTEK_MERIDIAN_LIMIT` and the removed `MOUNT_HOME.SEARCH` are client-visible. | informational | The SH20 profile is not yet in a release (CHANGELOG.md has no SH20 entry). It goes into the next changelog as new support. |
| Q10 | `PROTOCOL.md` Tracking row | The row said `:Te#`/`:Td#` → `1`. | non-blocking | Fixed: now `1#` / `0#`. |

The reviewer confirmed:
- the formatting rules are met;
- no `MFH`, `home_search_allowed`, `MOUNT_HOME_SEARCH_ITEM` or `X_TOUPTEK_MERIDIAN_LIMIT` references remain;
- the abort change (`== 3` to `>= 2`) is consistent with the poll's mirroring code for the count-2 profiles (OnStep, ZWO, NYX, ToupTek) and is a fix, not a regression.

## Fix reinspection
- The Q2/Q3 code was regenerated into the `.c`. Re-running the generator gives the same output.
- The Q4/Q5 test additions are covered by the final full-suite run in verification.md.

## Quality verdict
Pass. No blocking finding remains. Q1 and Q8 wait for the owner, as listed in handoff.md.
