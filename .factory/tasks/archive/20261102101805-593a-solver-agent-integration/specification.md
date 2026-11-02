# Specification

AC-001 Imager Agent: before its own processes (batch, focusing, the other paths calling `disable_solver()`), the Imager Agent sets `AGENT_PLATESOLVER_SOLVE_IMAGES` to `DISABLED` on the related solver agent whether it is an Astrometry Agent, ASTAP Agent, Platesolver Agent or an older-server "INDIGO Solver Agent"/"Solver Agent", including instance and "@ host" names. Driver version increases.

AC-002 Sequencer.js solver selection: `Sequence.prototype.start()` accepts an optional fourth argument `solver_agent`. When omitted, the solver is a device in `indigo_devices` whose name starts with "Platesolver Agent", "Astrometry Agent" or "ASTAP Agent" and that defines `AGENT_PLATESOLVER_EXPOSURE`; one on the host of the imager agent is preferred, otherwise one on any host; with none, "Astrometry Agent" on the imager agent's host is used (previous default). `sync_center()`, `precise_goto()`, `set_solver_filter()`, meridian flip recentering and abort address the selected solver. The `ASTROMETRY_AGENT` constant remains valid for existing scripts.

AC-003 Sequencer.js abort: the solver slot receives `AGENT_PLATESOLVER_ABORT` irrespective of its name.

AC-004 Web UI: astrometry.html drives "Platesolver Agent" when loaded, otherwise "Astrometry Agent"; renders with either; does not try to load/enable `indigo_agent_astrometry` while `indigo_agent_platesolver` is loaded; selects Imager and Mount Agent as its related agents. imager.html and mount.html select the loaded solver agent as related agent. Behaviour with Astrometry Agent is unchanged.

AC-005 Documentation: Sequencer.md describes the new `start()` argument and solver selection.

AC-006 No regressions in the existing Imager Agent and Sequencer.js integration suites.

Failure behaviour: with no solver agent, Sequencer.js falls back to the previous "Astrometry Agent" name (operations fail as before); the web UI waits and loads the Astrometry Agent as before.
