# Plan (as executed)

1. Reproduce from the code: `indigo_flipper` uses `indigo_sequencer.devices`, whose solver slot is set only by `Sequence.prototype.start()`/`select_solver_agent()`; the flip remains armed after the sequence ends.
2. Sequencer.js: add `sequencer_is_solver_agent()` and `sequencer_loaded_solver_agent()` (null when none loaded); in the flipper resolve the solver after the slew and fail when none is loaded (AC-001, AC-002).
3. Owner extended the scope to all solver usage: add `indigo_sequencer.loaded_solver_agent()` and use it in the four solver steps and the abort (AC-003, AC-004).
4. Sequencer.md: `start()` section and `enable_meridian_flip` row (AC-006).
5. Tests in test_agent_scripting_sequencer.c: flip after solver change, flip without solver, solver replaced during a sequence, solver failures with recovery points (owner question, AC-005), abort reaching a replaced solver (stand-in solver gains `AGENT_PLATESOLVER_ABORT`, closes FND-20261102123603-6a7f-select-solver-agent-001). Mutation run against the previous Sequencer.js.
6. Independent read-only review, fixes, final suite and Factory checks, process evidence, archive, owner approval of the two commits required by root AGENTS.md (product change, then `factory: ` artifacts).
