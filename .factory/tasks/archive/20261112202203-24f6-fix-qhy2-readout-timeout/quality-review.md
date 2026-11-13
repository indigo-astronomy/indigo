# Task quality review

Verdict: pass
Verdict note: non-blocking findings only; Q3, Q4 and Q5 fixed
Reviewer and independence: a general-purpose subagent in a fresh context (claude-opus-5-5), read only. It did not implement the change. The coordinator applied the fixes afterwards.
Reviewed revision and working-tree state: master `e0280e650` plus the uncommitted task diff of the three product files, before the fixes listed under Fix reinspection.

## Scope and method
The reviewer:
- read the diff of `indigo_ccd_qhy2.driver`, the generated `.cpp` and `test_ccd_qhy_sdk.cpp`, together with the surrounding acquisition code, the SDK header, the root, driver and test `AGENTS.md`;
- checked the Git history of the timeout call;
- regenerated the driver from the `.driver` in a scratch directory with the unchanged `build/bin/indigo_generator`, and the output was byte-identical;
- ran the new case in both builds.

It did not run the full suites.

## Quality findings
| ID | Location | Finding | Severity | Disposition |
| --- | --- | --- | --- | --- |
| Q1 | history | The fixed 1000 ms first appears in `3768cd34f` (generator migration). The earlier driver used the SDK default of 60000 ms, so this is a regression. Unconfirmed on a QHY168C. | informational | Recorded in brief.md and verification.md |
| Q2 | `.driver` `qhy2_setup` | The fix is correct: the unit is ms, `duration` is set before setup, it matches the driver deadline, the read-directly path is covered, streaming and legacy `ccd_qhy` are unaffected. | informational | — |
| Q3 | `.driver` timeout cast | A cast to `uint32_t` overflows for an SDK-reported maximum exposure above about 50 days, because the maximum is `fmax(SDK max, 900)`. | low | Fixed: `fmin(..., UINT32_MAX)` |
| Q4 | test, fake timeout branch | The payload check was vacuous. The timeout branch left the previous frame's identical pattern in the buffer, and the 0.1 s case stayed inside 1 s. Only the timeout-value assertion pinned the fix. | low/medium | Fixed: the timeout branch writes zeros and the readout is 1 s. Both frames are now zeros with the old driver (verification.md), and the parameter assertion is dropped in favour of the observable payload. |
| Q5 | test case list | The new case ran vacuously in the legacy `ccd_qhy` build. | informational | Fixed: case and function under `#ifdef QHY2`; `bad_raw` reset at case start |
| Q6 | test state | `verify_raw` is not reset if an assertion fails midway. | informational | Pre-existing pattern (`raw_color_payload`), left as is |
| Q7 | conventions | Version bump, single-line calls, tabs, braces and the specific comment are fine. No property change, so no `PROPERTIES.md` update; not a migration, so no `MIGRATION_STATUS.md` update. | informational | — |

## Fix reinspection
The coordinator inspected the Q3–Q5 fixes:
- `.driver` and `.cpp` were regenerated;
- the reproducer against the HEAD `.cpp` fails on the payload (2 bad payloads);
- the fixed driver passes;
- the full suites pass.

The exact commands are in verification.md.

## Quality verdict
Pass. The change is minimal and pinned by an observable regression test. The remaining limitation is hardware confirmation of the SDK timeout behaviour on a QHY168C.
