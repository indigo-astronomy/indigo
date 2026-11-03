# Specification

AC-001 With the imager agent loaded (the device has `AGENT_IMAGER_BATCH`), a solver agent on its host is selected, also when a local one is loaded.

AC-002 Otherwise the local solver agent is selected, then one on any other host. This holds when the imager agent is loaded but its host has no solver agent, and when the imager agent is not loaded, whatever its name says.

AC-003 With no solver agent loaded, `"Astrometry Agent"` is assumed on the host of the loaded imager agent, otherwise locally.

AC-004 A selection assumed at `start()` with no solver agent loaded is corrected at use by the same rule. A selection that is loaded stays in use.

AC-005 Documentation: Sequencer.md describes AC-001 to AC-004.

AC-006 Tests cover AC-001 to AC-004, independent of the order of `indigo_devices`, and fail against the previous Sequencer.js; no regressions in the Sequencer.js integration suite.
