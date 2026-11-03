# Specification

AC-001 Meridian flip solver: after the flip slew, when the flip recenters with a solver (`enable_meridian_flip(true, ...)`), it uses the solver agent of the sequence if that agent is loaded (present in `indigo_devices` and defining `AGENT_PLATESOLVER_EXPOSURE`); otherwise it selects a loaded solver agent again as `start()` does (Platesolver, Astrometry or ASTAP Agent, same host as the imager agent preferred). This applies also when the flip was enabled by a sequence that has finished.

AC-002 No solver agent at the flip: the flip fails instead of waiting: `FLIPPER_STATE` Alert, message "Meridian flip failed (no solver agent is loaded)", flipper handler removed and `indigo_sequencer.abort()`, as for the existing slew and sync & center failures.

AC-003 Solver steps and abort: `set_solver_exposure`, `set_solver_filter`, `set_solver_target`, `start_astrometry_process` (used by `sync_center()`, `precise_goto()`, `set_solver_filter()`) and the abort address the loaded solver agent selected as in AC-001. With no solver agent loaded the selected name is kept and the step fails through the existing missing-property path.

AC-004 Explicit selection: a solver agent chosen by `select_solver_agent()` (or by `start()`) is kept while it is loaded.

AC-005 Recovery: a failed solve (solver `AGENT_START_PROCESS` Alert) in `sync_center()`/`precise_goto()` and a non-existent filter in `set_solver_filter()` follow the failure policy; under `recover_on_failure()` the sequence continues at the next `recovery_point()`.

AC-006 Documentation: Sequencer.md describes AC-001 to AC-004.

AC-007 No regressions in the Sequencer.js integration suite; the new behaviour is covered by tests that fail against the previous Sequencer.js where the behaviour changed.

Failure behaviour: no new failure modes besides AC-002; a selected agent that disappears while a step waits for its property is not handled (pre-existing; the step waits as before).
