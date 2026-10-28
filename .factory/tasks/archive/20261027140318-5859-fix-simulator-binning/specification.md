# Specification

Status: accepted (task brief from the owner's session of 2026-10-27)

## Problem and outcome
The camera simulators must bin like a real sensor: a binned pixel carries the signal of its whole bin block, so a
binned frame has the stars of the unbinned frame scaled down with their flux preserved, and the Solver Agent solves a
camera-binned Rosetta frame.

## In scope
- CCD Imager Simulator light frames: each binned pixel is the mean of its `horizontal_bin` x `vertical_bin` block of
  the fixed image (integer part); CCD_FRAME, gain, offset, gamma, defocus blur, noise and 8-bit output unchanged.
- CCD Guider Simulator stars: the Gaussian profile is evaluated in unbinned pixels (`(centre - pixel) * bin`) with
  the unbinned amplitude, so the shape is the unbinned one scaled down and the frame keeps its range.
- Driver version 44 to 45, regenerated `.c`, `.h` and `_main.c`.
- Regression cases in `test_ccd_simulator`, the `rosetta camera binned` case in `test_agent_solver_simulator`,
  records.

## Out of scope
Guider background gradient and noise, eclipse mode, the bin handler and frame geometry, solver changes, hardware,
other platforms.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A 2 x 2 binned Rosetta frame captured through the Imager Agent solves to the astrometry.net centre of the frame (within 36"), 2 x 8.44"/px per binned pixel (within 2 %), angle 179.2° (within 0.2°) and an 800 x 600 frame | `test_agent_solver_simulator "rosetta camera binned"` |
| AC-002 | Every pixel of a binned imager light frame equals the integer mean of its bin block of the fixed image plus the 0 to 127 ADU frame noise, for binning 1, 2 and 4; blocks summing beyond 65535 are averaged, not clamped | `test_ccd_simulator simulator_imager_binning_averages_the_bin_block` |
| AC-003 | A 2 x 2 binned guider frame renders an unsaturated star at half the unbinned position (within 1 binned px), with a root mean square radius between 0.3 and 0.7 of the unbinned one and a quarter of the flux above the background (within 20 %) | `test_ccd_simulator simulator_guider_binning_scales_the_star_profile` |
| AC-004 | The existing simulator based suites still pass: `test_ccd_simulator`, `test_agent_imager`, `test_agent_guider`, `test_agent_astrometry`, the Rosetta set of `test_agent_solver_simulator` | the suites |
| AC-005 | Driver version 45, regenerated outputs reproduce from the `.driver` file, the simulator record is updated with a recorded run, the solver test no longer excludes camera binning | `tools/run_driver_test.py ccd_simulator`, README.md, MIGRATION_STATUS.md, TEST_SUMMARY.md, the test header, diff review |

## Failure behavior and edge cases
The mean of a block never exceeds 65535, so no binned pixel clamps; the saturated cores of the fixed image stay at
their value. CCD_FRAME values are divided by the bin as before, so a frame origin not divisible by the bin starts at the
block of the truncated binned origin, unchanged behaviour. Guider stars whose scaled profile falls below a binned
pixel keep their amplitude in that pixel, like a software-binned camera.

## Permissions, data, and security requirements
No new properties, files, network access or permissions. The solver test reuses the persistent index cache as before.

## Compatibility and nonfunctional requirements
Portable C, no platform code; the inner sum loop runs over at most 16 pixels per binned pixel of a frame rendered in
the exposure thread, negligible against the existing blur and noise passes. Clients see the same brightness range at
every binning, as with a software-binned camera and the agent's own downsampling.

## Assumptions and open questions
Assumption: the WCS scale of the Solver Agent refers to the pixels of the captured frame (binned), so the expected
scale of the binned case is twice the unbinned one and the field size is unchanged; verified in
`indigo_libs/indigo_platesolver.c` (the agent divides by its own downsampling only).

## Owner decisions and requirement changes
2026-10-27: the owner's session proposed the task with sum (clamped) or average as acceptable; the sum was
implemented first and rejected by measurement (the summed frame does not solve), the average is delivered, see the
brief and verification.md.
2026-10-28: master archived the driver-local REFACTOR.md files (`c002f790a`) and moved their rules to the task record;
the notes of this task moved into verification.md, the archived files stay as master left them.
