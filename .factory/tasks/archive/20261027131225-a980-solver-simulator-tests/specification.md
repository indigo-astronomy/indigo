# Specification

Status: accepted (owner instruction in conversation, 2026-10-27)

## Problem and outcome
Verify the Solver Agent end to end against the CCD simulators through the production Imager and Guider agents, with
the real Tycho-2 indexes, over many rendered fields, and fix what the verification reveals.

## In scope
- Opt-in integration test `indigo_test/integration/test_agent_solver_simulator.c`, make target
  `test-agent-solver-simulator`, listed in `OPT_IN_DRIVER_TESTS` so `tools/run_driver_test.py agent_solver` runs it.
- Set 1 (CCD Imager Simulator, Rosetta frame): full resolution, agent downsampling, sub-frame, JNow output, each
  index alone, a wrong scale hint, repeated solves.
- Set 2 (CCD Guider Simulator): at least 100 distinct `SIMULATION_SETUP` combinations of RA, DEC,
  IMAGE_ROTATION_ANGLE, SIDE_OF_PIER, IMAGE_WIDTH, IMAGE_HEIGHT, MAGNITUDE_LIMIT, EPOCH, IMAGE_NOISE_VAR,
  IMAGE_GRADIENT, IMAGE_HOTPIXELS, IMAGE_HOTCOL and IMAGE_HOTROW, each solved and compared with the rendered field;
  some of them captured through the Guider Agent instead of the Imager Agent.
- Index handling in the test: the 2° and 5° indexes are taken from the persistent cache and downloaded by the agent
  only when missing there.
- Fixes of solver defects found, with regression coverage in the existing unit/integration tests where possible;
  driver version bump; REFACTOR.md, README.md, MIGRATION_STATUS.md and `indigo_test/AGENTS.md` updates; Xcode
  registration of the new test source.

## Out of scope
CCD simulator changes (its camera binning subsamples instead of summing, so a camera-binned Rosetta frame is not a
valid solver input and is not tested), hardware, other platforms, index hosting.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The Rosetta frame captured through the Imager Agent solves to the astrometry.net solution (centre within 36", scale within 2 %, angle within 0.2°) in every variant of set 1 | `test_agent_solver_simulator rosetta` |
| AC-002 | At least 100 distinct guider simulator setups solve to the rendered centre (36"), scale (2 %), angle (0.2°, precession of JNow fields accounted for) and frame size | `test_agent_solver_simulator guider` |
| AC-003 | The WCS angle and parity agree with the astrometry.net convention on the same simulator frames | reference run of `test_agent_astrometry_solver "solve only"` (angle 36.00° for rotation 36°, parity 1) compared with the sweep |
| AC-004 | No index is downloaded when present in the persistent cache; a missing one is downloaded once within the test | first run with an empty `~/.indigo/solver`, second run without download messages |
| AC-005 | The existing unit and agent integration tests still pass with the convention and accuracy fixes | `test_agent_solver_core`, `test_agent_solver`, `make -C indigo_test test-unit` |
| AC-006 | The driver record and documentation are updated | `tools/run_driver_test.py agent_solver`, README.md, REFACTOR.md, MIGRATION_STATUS.md, PROPERTIES.md unchanged |

## Failure behavior and edge cases
A missing index with no network makes the install step fail with the agent's message; a field the simulator renders
with too few catalogue stars (small frames with a bright magnitude limit) is kept out of the sweep rather than
expected to fail; noise above the amplitude of the rendered stars is likewise not a valid input.

## Permissions, data, and security requirements
The test writes only to the persistent index cache (downloads through the agent) and to its private home; it needs
`curl` and network access only for a missing index.
