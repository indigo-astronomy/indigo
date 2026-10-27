# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 imager binned region equals block means | AC-002 | integration `indigo_test/integration/test_ccd_simulator.c` `simulator_imager_binning_averages_the_bin_block` | CCD Imager Simulator, RAW, CCD_FRAME 616/880/128/96 (brightest star of the image), CCD_BIN 1, 2, 4 | every pixel within [mean, mean + 127]; at least one block with a sum beyond 65535 for binning 2 and 4, averaged | with the fix |
| T-002 guider binned star profile | AC-003 | integration, `simulator_guider_binning_scales_the_star_profile` | CCD Guider Simulator, STARS, Orion at rotation 0, noise range 1 (deterministic), gradient 0, star peak below 10 000 ADU at least 32 px from the edges | binned centroid within 1 px of half the unbinned one, width 0.3 to 0.7 of the unbinned, flux a quarter within 20 % | with the fix |
| T-003 Rosetta camera binned | AC-001 | integration `indigo_test/integration/test_agent_solver_simulator.c` `rosetta camera binned` (opt-in) | Imager Agent CCD_BIN 2 x 2, Solver Agent, Tycho-2 indexes | RA 6.5269 h Dec 5.095° angle 179.2°, 16.88"/px over 800 x 600 | with the fix; also run against the rejected summed variant |
| T-004 preserved behaviour | AC-004 | `test_ccd_simulator` (all cases), `test_agent_imager`, `test_agent_guider`, `test_agent_astrometry`, `test_agent_solver_simulator rosetta` | built driver | all pass | after the fix |

## Baseline evidence
Before the fix the camera binned Rosetta case did not solve (task 20261027131225-a980, hash tolerance 2 and 3 tried
without success); the subsampled frame has single pixel stars. T-001 and T-002 are derived from the requirement (a
binned pixel carries its block) and fail against the old driver by construction: the old imager frame equals one
sample of each block (the minimum pixel of the image is 455, so a sample is never within 127 of the block mean of
the brightest star region), the old guider star is as wide in binned pixels as in unbinned ones (ratio 1, outside
0.3 to 0.7). T-003 was also run against the summed and clamped variant: it fails deterministically (three runs,
5 of 57 consistent matches), which decided for the mean.

## Bug reproduction
The solver symptom is reproduced by T-003 against the old driver (recorded in the previous task); T-001 and T-002 pin
the rendering itself.

## Negative cases and permission boundaries
The bin handler's rejection of 3 x 3 and asymmetric binning stays covered by `simulator_raw_geometry_and_bins`.

## Isolation and cleanup
Each case connects and disconnects its device through `sim_begin()` / `sim_end()`, frees the captured frame and
clears the capture flag; the guider case sets its own SIMULATION_SETUP values, which are not persistent.

## Deferred tests and coverage limitations
The guider gradient and noise are not scaled with the binning (out of scope, unchanged behaviour). Whether the
summed frame fails because of its clamped cores or its additional faint detections is not separated
(FND-20261027140318-5859-fix-simulator-binning-002). Linux and Windows not run.
