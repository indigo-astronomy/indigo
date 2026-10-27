# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 Rosetta full resolution | AC-001 | integration `indigo_test/integration/test_agent_solver_simulator.c` | Imager Agent, CCD Imager Simulator, 2° and 5° indexes | RA 6.5269 h Dec 5.095° angle 179.2° 8.44"/px, index 2 or 5 | with implementation |
| T-002 Rosetta downsampled by the agent (hint 2) | AC-001 | integration | as above | same solution | with implementation |
| T-003 Rosetta centred 1000 x 750 sub-frame | AC-001 | integration | CCD_FRAME on the Imager Agent | same centre and scale, WCS size of the sub-frame | with implementation |
| T-004 Rosetta in JNow (hint EPOCH 0) | AC-001 | integration | as above | J2000 solution converted with `indigo_j2k_to_jnow`, WCS EPOCH 0 | with implementation |
| T-005 Rosetta with only the 2° / only the 5° index selected | AC-001 | integration | USE_INDEX | solved with that index | with implementation |
| T-006 Rosetta with a scale hint ten times too large | AC-001 | integration | HINTS SCALE | solved (hint only orders the indexes) | with implementation |
| T-007 Rosetta solved three times | AC-001 | integration | as above | three identical solutions, catalogue reused | with implementation |
| T-008 guider sweep, 120 combinations | AC-002, AC-003 | integration, cases `guider 001`-`guider 120` | 24 sky positions x rotations {0, 36, 90, 137, 270} x pier x 7 frame sizes x magnitude {8.5, 9.5, 9.0} x epoch {2000, JNow} x noise {100, 150, 250, 400} x gradient {0.2, 0, 0.5} x hot pixels/columns/rows {0, 50, 200/600/800, 500}; every 20th through the Guider Agent | centre < 36", scale 2 %, angle 0.2° with precession offset for JNow, frame size | with implementation |
| T-009 index reuse | AC-004 | integration, manual observation of the first and second run | empty then filled `~/.indigo/solver` | "downloading it once" printed on the first run only | with implementation |
| T-010 convention regression | AC-005 | unit `test_agent_solver_core` (`solve_synthetic_field`, `solve_synthetic_mirrored_field`), integration `test_agent_solver` | synthetic frames rendered with the WCS convention | parity and angle returned as rendered | with implementation |
| T-011 reference convention | AC-003 | opt-in `test_agent_astrometry_solver "solve only"` | real astrometry.net, index 4112 | angle 36.00° parity 1 for rotation 36° | before the fix |

## Baseline evidence
Before the fixes: angles of the guider frames off by 180° plus up to 1.1° (parity −1 reported), Rosetta angle 179.16°.

## Bug reproduction
The tangent point error was reproduced with `INDIGO_SOLVER_SETUP="7.5,-60,0,0,1600,1200,9.5,2000,100,0,0,0,0"`
(angle 359.08° for rotation 0°, pier 0, i.e. 0.92° off after the 180° convention difference); the convention
difference with the astrometry.net reference run; both are covered by T-008 and T-010 after the fix.

## Known exclusions
Camera binning of the simulator (subsampling), noise above the star amplitudes (IMAGE_NOISE_VAR ≥ 600 buries stars
fainter than magnitude 8), polar error and pixel offsets of the simulator (they move the rendered centre by an
amount the test does not model).
