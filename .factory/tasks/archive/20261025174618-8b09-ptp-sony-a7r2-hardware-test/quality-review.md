# Task quality review

Verdict: pass
Reviewer and independence: three fresh read-only general-purpose subagents (same model), one per review round, plus the coordinator's own pass
Reviewed revision and working-tree state: uncommitted diffs on the task branch at each round; the final candidate differs from the last reviewed diff only by the fixes listed below

## Scope and method
Driver (`indigo_ptp_sony.c/.h`, `indigo_ccd_ptp.c`, `indigo_ptp.h`), hardware suite, fake USB suite and Makefile flags, with the surrounding exposure, event, live view and property-write paths; AGENTS.md style rules.

## Quality findings
Round 1 (live view window, stale objects): missing expiry of pending/stale shots, readout flag after a polling-path discard, 10 s step wait too short, missing idle-discard and expiry coverage, misleading discard log — all fixed.
Round 2 (retry, confirmation, drain): drain could fetch a pair partner before it is reported ready (camera-crash risk) — fixed (single-object drain); stepping test wait shorter than the rewrite period — fixed; count of a drained pair RAW against another late shot — commented; retry after a very slow real shot, 15 s BUSY window — recorded in REFACTOR.md.
UBSan: the fake's release after an ignored press used a stale press time — fixed in the previous task, still covered.

## Fix reinspection
Coordinator reinspected each fix; negative controls and retests in verification.md.

## Quality verdict
Pass. Limitations recorded in the driver REFACTOR.md.
