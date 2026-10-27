# Solver Agent implementation and test record

## Baseline and current-state audit

Before task 20261027102245-431a-port-quad-solver the driver was a skeleton (version 3.0.0.3): it attached the shared
`indigo_platesolver.c` base, registered `solver_solve()` returning `false` and an empty `solver_abort()`, and exposed
only the shared plate solver properties. No driver-specific property, index handling, solving code or automated test
existed. The driver is built by the generic driver Makefile and the Xcode project; it is not part of the Windows
solution. Baseline on 2026-10-27, Darwin arm64 host: `make -C indigo_drivers/agent_solver -f ../../Makefile.drv all`
succeeded; no driver test existed to run.

The solving code was developed and validated outside this repository in the `Solver` macOS tester (quad hashing of
Tycho-2 stars in HEALPix cells, astrometry.net style geometric codes, hash matching with voting, robust affine fit on
the tangent plane). This task ports its C part and wires it into the agent. No hardware testing applies: the agent
consumes frames from the bus, its camera and mount behaviour belongs to the related agents. No hardware validation
is claimed.

## Atomic plan

1. Port the core as `indigo_solver_utils.{c,h}` (geometry, quad codes, index file format), `indigo_solver_healpix.{c,h}`
   and `indigo_solver.{c,h}` (index selection/loading, image stars and quads, matching, centre fit) in repository
   style, keep the index files out of the repository — complete.
2. Port the index builder as `indigo_tools/indigo_solver_indexer.c` building `$(BUILD_BIN)/indigo_solver_indexer`
   from the shared core objects — complete, builds and reproduces the 5° and 20° indexes of the tester byte for byte
   in size and content counts (36 864 stars / 6 205 903 quads and 3 840 / 1 145 554).
3. Implement the agent: `X_AGENT_SOLVER_INDEX` index management (download with curl into
   `$INDIGO_CACHE_BASE/solver` or `~/.indigo/solver`, header validation of the download, deletion, publication in
   `AGENT_PLATESOLVER_USE_INDEX`), native `solver_solve()` for INDIGO RAW frames with downsampling, scale hint based
   index ordering, WCS output and abort; version 3.0.0.4 — complete.
4. Register the new files in the driver Makefile (automatic), the tools Makefile, the test Makefile and the Xcode
   project; ignore `*.isix`; document the property in `indigo_docs/PROPERTIES.md` — complete.
5. Add `indigo_test/unit/test_agent_solver_core.c` (9 cases) and `indigo_test/integration/test_agent_solver.c`
   (6 cases, synthetic index and RAW frame, no network) — complete, all passing.
6. Run the driver build, the new tests, the whole native unit suite and the Factory check/test/security groups;
   record results here and in the task — complete on Darwin arm64; Linux and Windows builds were not available.

## Index files

Index files (`tyc2-<fov>deg.isix`, 1°, 2°, 5°, 10° and 20°, 32 MB to 534 MB each) are never checked in. They are
built from the Tycho-2 catalogue (CDS I/259, `I_259/tyc2.dat.00` … `tyc2.dat.19` in the working directory) with
`indigo_solver_indexer 1 2 5 10 20`, published under `AGENT_SOLVER_INDEX_URL` and downloaded on demand by the agent.
The URL is the `index_release_v1` release of the INDIGO repository on GitHub, where the five files built by the tool are published.

## Found defects and fixes

All defects were found in the ported code by the new tests; none existed in this repository before the task.

- `fit_2d_line()` evaluated the normal equations in float: the determinant of three seed points cancelled to noise,
  so every robust fit seed produced garbage. The sums and determinants are now double. Regression:
  `solve_synthetic_field` (unit).
- The Cramer determinant of the third coefficient had a typo (`sum_dz * sum_dxdy` instead of
  `sum_dx_dz * sum_dy`). It was masked in the tester, which centred the data on its mean so the erroneous term
  vanished; the robust fit centres on the image centre and exposed it. Regression: `solve_synthetic_field` (unit).
- The inlier limit of the robust fit was `max(3 sigma, 2 px)`: a wrong seed with a huge residual sigma accepted every
  match and won. The limit is now an absolute 3 px of the fitted scale. Regression: `solve_rejects_unrelated_field`
  (unit) and the 20° index run of the tester frame in the task verification.
- A solution could consist of the four self-consistent pairs of a single matched quad. At least 6 inliers are now
  required. Regression: the 20° index run in the task verification (`4 of 28` rejected).
- The rotation angle was derived from the xi row of the fit, which flips with the image parity, so mirrored frames
  reported a wrong angle. It is now taken from the eta row and matches `indigo_radec_to_xy()` for both parities.
  Regression: `solve_synthetic_mirrored_field` (unit).
- `indigo_angular_distance()` used `acos()` of the dot product, which in float cannot resolve separations below a
  few arcminutes. It is now the haversine form. Regression: the centre assertions of `solve_raw_image`
  (integration).

## Scenario mapping

- Unit (`test_agent_solver_core`): tangent plane round trip and far hemisphere rejection; hash component packing and
  clamping; quad code invariance under translation, rotation, scale and mirroring; canonical code first among
  variants and code bounds; index file round trip, truncated file, bad magic, bad version, dangling star reference,
  unsorted input; index selection by field size and presence; synthetic field solve, mirrored field solve, unrelated
  field rejected.
- Integration (`test_agent_solver`): installed index reflected in `X_AGENT_SOLVER_INDEX` and
  `AGENT_PLATESOLVER_USE_INDEX`; RAW frame solved at full resolution (centre within 10", scale within 0.3%, angle
  within 0.2°, parity, index, epoch); downsampled solve reported in unbinned pixels; refusal without a selected
  index; refusal of a non-RAW upload; deleting an index removes the file and withdraws the item.
- Not covered: the download path (network), FITS/JPEG uploads (refused by design in this version), Linux and Windows
  builds.

## Simulator verification (task 20261027131225-a980-solver-simulator-tests)

`indigo_test/integration/test_agent_solver_simulator.c` (opt-in, `make -C indigo_test test-agent-solver-simulator`)
drives the agent with the production Imager and Guider agents, the CCD simulators and the real Tycho-2 indexes of the
`index_release_v1` release, taken from the persistent cache `$INDIGO_CACHE_BASE/solver` (default `~/.indigo/solver`)
and downloaded there by the agent only when missing. Set 1 solves the Rosetta frame of the CCD Imager Simulator
(full resolution, agent downsampling, sub-frame, JNow output, each index alone, wrong scale hint, repeated solves)
against the astrometry.net solution of the same frame. Set 2 renders 120 distinct `SIMULATION_SETUP` combinations
of the CCD Guider Simulator (24 sky positions down to Dec ±85°, five rotations, both pier sides, seven frame sizes,
three magnitude limits, J2000 and JNow, four noise levels, three gradients, four hot pixel/column/row settings, every
twentieth frame captured through the Guider Agent) and compares every solution with the rendered field: centre within
36", scale within 2 %, angle within 0.2° (the precession turn of JNow fields against J2000 north is added to the
expectation), frame size, WCS epoch and state.

Defects found by the sets, fixed in this task:

- The angle of the solution referred to north at the tangent point of the fit, which is a reference star up to half a
  field away from the image centre; the meridians converge between the two points by up to a degree at high
  declination (0.92° at Dec −60° in a 5.3° frame, nothing at the Dec 5° Rosetta frame, so the first task did not see
  it). `indigo_solver_compute_center()` now refits the inliers with the found centre as the tangent point, so angle,
  scale and centre refer to the image centre. Angles agree with the rendering to 0.03°.
- The parity sign and the angle of mirrored frames did not follow the astrometry.net convention the Astrometry agent
  reports: the simulator frames (east to the right, north up) came out as parity −1 with an angle 180° from the
  reference run of `test_agent_astrometry_solver` (angle 36.00°, parity 1 for rotation 36°). Parity is now 1 for
  such frames and their angle is turned by 180°, the Rosetta frame (parity −1, 179.2°) is unchanged.
  `indigo_radec_to_xy()` takes the parity in the same convention; the unit tests render and expect it, the agent
  integration test expects parity −1 for its sky-as-seen synthetic frame, the indexer passes −1 (unchanged
  projection). Version 3.0.0.5.

Not a solver defect, excluded from the sets: the CCD simulator bins the imager frame by taking every second pixel
instead of summing, which turns the stars into single pixel spikes whose codes do not match (the agent's own 2 x 2
downsampling, which averages, solves). IMAGE_NOISE_VAR at or above the amplitude of magnitude 8 stars (600 and
1000 ADU) buries most of the rendered field and is likewise not a valid input; the sweep uses 100 to 400.

## Final test summary

Simulated (hardware-free) tests: 143 run, 143 passed (9 unit, 6 agent integration, 128 simulator integration:
8 Rosetta cases and 120 guider setups). Hardware tests: 0 run, 0 passed.
