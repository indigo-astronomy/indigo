# Task quality review

Verdict: pass
Reviewer and independence: fresh general-purpose subagent (same host and model), read-only, independent of the implementation context; two passes (the initial diff, then the additions after the first full run); the last two hardenings (R5, R6) were reinspected by the implementer
Reviewed revision and working-tree state: master 237b84532 plus the uncommitted task diff on task/20261106223707-63b0-fix-synscan-j2000-epoch (2026-11-06/07)

## Scope and method
Static review of `git diff master -- indigo_drivers/mount_synscan indigo_test MIGRATION_STATUS.md`, the whole
coordinate path of the `.driver` (`synscan_read_mount_coordinates()`, `synscan_radec_to_steps()`,
`synscan_ha_dec_to_steps()`, `synscan_encoder_to_eq()`, `synscan_eq_to_encoder()`,
`synscan_select_encoder_solution()`, `synscan_position_to_steps()`, `synscan_slew_axis_to_steps()`, the park/home
handlers and finalizers, the connect sequence), the sync branch and the alignment-model transforms of
`indigo_libs/indigo_mount_driver.c`, the precession helpers of `indigo_libs/indigo_align.c`, the simulator's
`:G`/`:H`/`:J`/`:I` handling and axis timer, the new and changed test code and helpers, the hardware case, the root
AGENTS.md formatting/C/generated-driver rules and the quality-review routine. The reviewer compared the regenerated
`.c` with the `.driver` diff.

## Quality findings
| ID | Severity | Location | Finding | Disposition |
| --- | --- | --- | --- | --- |
| R1 | low | new case, readback check | the raw RA readback within 0.005 h had about a 2x timing margin against the sidereal drift between the GoTo and the fresh poll | fixed: the readback is checked as the pointing of the date (`synscan_fresh_date_pointing()`), time-invariant with tracking off |
| R2 | low | compatibility | alignment points saved by version <= 21 hold raw values that were effectively of the date; applied by version 22 they mis-point by about the precession | documented in the specification and the delivery note; a changelog/README line is the owner's call at release; no code change |
| R3 | style | `.driver` `synscan_read_mount_coordinates()` | the second `synscan_range24()` after the precession is redundant given `indigo_precess()`'s own normalisation | kept as a guard against a rounding result of exactly 24.0 |
| R4 | style | new case, `pointed_ha` | the inverse assumes the west encoder solution | correct for the fixed target, stated in the comment; the east solution is exercised by the pier-side case |
| R5 | low | `synscan_set_position()` CURRENT | the last poll's counters can be a second old or, after a failed first reading at connect, those of a previous session | fixed: the handler reads the counters with `synscan_update_axis_positions()` and reports ALERT on both properties if that fails |
| R6 | low | new park case cleanup | the park left `synscan-EQ8.park` in the suite's private folder for the next case to restore (a pre-existing pattern of the suite) | fixed: `synscan_remove_park_file()` in the case's cleanup and in `main()` |
| R7 | informational | park/home positions | a CURRENT taken at a counterweight-up position maps back to the other pier side, so a park then flips | pre-existing (same with the base driver and the previous driver); not changed |

No medium or high findings. No confirmed defects, no formatting-rule violations, no scope drift.

The reviewer confirmed: the goto and read paths are exact inverses (bus J2000, alignment model, precession, hour
angle of the date); the sync branch stores both the raw and the aligned coordinates in J2000, so the pure-offset
model stays consistent; park and home positions stay mechanical; `indigo_precess()` returns RA in [0, 2π); the
regenerated `.c` differs from master only by the version define, the edited functions and the generator's output for
the two new `on_change` blocks (queued handlers with the OK prologue and the final update); the new case's step
arithmetic matches the driver's zero positions, the simulator's steps per revolution and the direction character of
`:G`; the park case's "no `:J`" assertion holds because a zero delta returns before any motion command; the
stopped-axis checks on the declination of the date are LST-independent to about 1e-12; `MIGRATION_STATUS.md` matches
the 96 entries of `main()`; formatting rules are met.

## Fix reinspection
R1: the readback now goes through `synscan_fresh_date_pointing()` (0.01 h / 0.01°); run result in verification.md.
R5: `synscan_update_axis_positions()` is a bounded transaction on the device queue the handler already runs on;
both momentary items are cleared and `set->state` plus the position property go ALERT with a message on failure,
the generated epilogue publishes the set property. R6: the helper replaces the inline unlink of `main()` as well.
Both reinspected by the implementer only (same-agent limitation); the final recorded run covers them.

## Quality verdict
Pass. Remaining limitations: R2 (stale saved alignment points of earlier versions) is a documentation follow-up for
the owner; R7 is a pre-existing behaviour outside this task.
