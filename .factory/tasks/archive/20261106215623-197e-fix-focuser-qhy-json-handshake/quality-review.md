# Task quality review

Verdict: pass
Reviewer and independence: fresh general-purpose subagent (same host and model), read-only, independent of the implementation context; the R2 addition was reinspected by the implementer
Reviewed revision and working-tree state: master 86d3e884e plus the uncommitted task diff on task/20261106215623-197e-fix-focuser-qhy-json-handshake (2026-11-06, before the R2 addition)

## Scope and method
Static review of `git diff master -- indigo_drivers/focuser_qhy indigo_test/integration/test_focuser_qhy_simulator.c`,
the whole `qhy_parse_json()`, `qhy_json_*()` and `qhy_command()` code of the `.driver`, the simulator `fault()` and
`dispatch()`, the test runner, the root AGENTS.md formatting/C/generated-driver rules and the simulator modelling rule
of `indigo_test/AGENTS.md`. The reviewer also regenerated the `.driver` in a scratch directory and compared the
output with the checked-in files.

## Quality findings
| ID | Severity | Location | Finding | Disposition |
| --- | --- | --- | --- | --- |
| R1 | low | simulator `--quoted-version`, test `quoted_version_handshake` | the quoted-string handshake form was never observed on hardware; it is the driver's former assumption | kept as an option (the default is the hardware reply); recorded as a backward-compatibility form without hardware evidence in verification.md |
| R2 | low | test `escaped_quote_connection` | no case proves an accepted escape other than `\u` in a well-formed string (`\"`, `\\`) | fixed: simulator option `--id VALUE`; new case `escaped_id_handshake` connects with `"id":"a\"b\\c"` |
| R3 | low | `MIGRATION_STATUS.md:110` | test count 44 -> 46 is outside the reviewed diff | expected: the count is maintained by hand per `indigo_drivers/AGENTS.md`; now 47 after R2 |

No medium or high findings. No style findings.

The reviewer confirmed: the escape handling advances at most onto the terminating NUL and returns false for a
trailing backslash; `\"` and `\\` are consumed as one escape; the 32/64-byte length guards are unchanged; the
numeric helpers still reject quoted values; the quoted `version`/`bv` form is still accepted; keeping escapes
undecoded is appropriate (only `version` and `bv` are published, no control bytes reach a text property); the
rename `qhy_json_string` -> `qhy_json_text` is complete; the regenerated `.c`, `.h` and `_main.c` are byte-identical
to the checked-in files and differ from master only by the parser edits and `DRIVER_VERSION 0x0300000F -> 0x03000010`;
formatting rules are met; the `escaped-quote` fault reply ends in `}` and reaches the parser intact; the argument
array of the runner has room for every option plus the terminator.

## Fix reinspection
R2: `--id` sets the simulator's `device_id` string, which `dispatch()` prints unchanged inside the quotes of the
default reply, so `--id 'a\"b\\c'` sends `"id":"a\"b\\c"`; the case checks CONNECTION OK through `driver_start()`,
one handshake command and the published firmware text. Run result in verification.md.

## Quality verdict
Pass. Remaining limitation: the quoted-string handshake form (R1) is kept without hardware evidence.
