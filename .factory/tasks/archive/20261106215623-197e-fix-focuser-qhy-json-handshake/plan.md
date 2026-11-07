# Development plan

Status: done

## Approach and affected components
- `indigo_drivers/focuser_qhy/indigo_focuser_qhy.driver`: `qhy_parse_json()` skips a backslash and the following
  character inside quoted keys and values; `qhy_json_string()` becomes `qhy_json_text()` and accepts quoted or
  unquoted values; version 16. Regenerate `indigo_focuser_qhy.c`, `.h` and `_main.c`.
- `indigo_drivers/focuser_qhy/focuser_qhy_simulator/focuser_qhy_simulator.c`: the default `cmd_id 1` reply is the
  one observed on hardware (`id` with `\u` escapes, numeric `version`/`bv`); a `--quoted-version` option sends the
  quoted-string form; `--firmware`/`--board-version` keep overriding the values; a `VERSION escaped-quote` fault
  sends a reply with an escaped closing quote.
- `indigo_test/integration/test_focuser_qhy_simulator.c`: expected handshake reply and INFO values, extra
  simulator options per case, two new cases.
- `MIGRATION_STATUS.md`: simulator test count of the `focuser_qhy` row.

## Baseline
- Commands and environment: macOS arm64 (Darwin 27.0.0), clang; INDIGO library and drivers already built under
  `build/`; `make -C indigo_test build/integration/test_focuser_qhy_simulator` and the binary.
- Existing failures: see verification.md (baseline section).
- Repository revision / working-tree state: task branch from master 86d3e884e.

## Hardware-test decision
No hardware test: no Q-Focuser is available. The hardware-observed reply from the user's report (raw bytes captured
with pyserial) is modelled in the simulator; hardware validation is not claimed.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | baseline: build and run the unchanged suite | AC-006 | None | 44/44 recorded in verification.md | done: 44/44 |
| 2 | simulator: hardware reply by default, `--quoted-version`, `escaped-quote` fault | AC-005 | None | builds | done: builds without warnings |
| 3 | tests: new expectations, extra options, `quoted_version_handshake`, `init_version_escaped_quote`; run against the unchanged driver to reproduce the defect | AC-001..005 | 2 | handshake cases fail with the old parser | done: `capabilities`, `init_version_failure`, `init_version_escaped_quote` fail with the master driver, `quoted_version_handshake` and `simulator_protocol` pass (verification.md) |
| 4 | driver: parser escapes, `qhy_json_text()`, version 16; regenerate | AC-001..004, AC-007 | 3 | regenerated outputs differ only as explained | done: only the version define and the edited functions changed in the .c; .h and _main.c unchanged |
| 5 | final recorded run `python3 tools/run_driver_test.py focuser_qhy`; ASan build of the suite | AC-006 | 4 | 46/46, README and TEST_SUMMARY.md updated | done: 46/46, then 47/47 after the review addition (R2); ASan: see verification.md |
| 6 | `MIGRATION_STATUS.md` count; independent quality review; security review; verification | all | 5 | reports with `Verdict: pass` | done: count 47 / 0; review R1-R3 dispositioned, R2 fixed; security and verification written |

## Test strategy
Simulator-backed integration tests through the serial PTY simulator; the protocol case checks the raw reply, the
driver cases check CONNECTION, INFO and the command sequence. ASan build of the suite for the parser change.

## Security risks and mitigations
Parsing device input: keep every length check; the new escape handling only advances the cursor by two when a
character follows the backslash.

## Dependencies and architecture decisions
No new dependency. Escapes are not decoded: the driver does not use the fields that carry them.

## Migration, rollout, and rollback
None needed; revert the commit to roll back.

## Deviations and owner decisions
Review finding R2 added a third case (`escaped_id_handshake`) and the simulator `--id` option; the recorded run was repeated in full.
