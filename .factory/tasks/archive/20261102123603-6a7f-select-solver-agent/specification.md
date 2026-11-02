# Specification

AC-001 `Sequence.prototype.start` takes `imager_agent`, `mount_agent` and `guider_agent` only; a fourth argument has no effect.
AC-002 Every `start()` selects the solver automatically: a device named Platesolver, Astrometry or ASTAP Agent that defines `AGENT_PLATESOLVER_EXPOSURE`, preferring the imager agent's host, then any host, otherwise "Astrometry Agent" on the imager agent's host.
AC-003 `select_solver_agent(agent)` is a sequence step that stores the name in the solver slot and continues, like `select_guider_agent`; following solver steps use it, without any check that the agent exists at `start()`.
AC-004 A meridian flip with `use_solver` sends `CENTER` to the current solver, both automatically selected and selected by the step.
AC-005 Sequencer.md documents the automatic selection and the step.
AC-006 Existing Sequencer suite cases keep passing; the assertion of the removed argument is replaced by the step's coverage (owner decision).

Failure behaviour: a wrong selected name fails at the first step that uses it, as for the other agents. Hardware decision: no hardware; the library is exercised with simulators and a fake solver peer.
