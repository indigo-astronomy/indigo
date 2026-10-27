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
The URL is a placeholder until the owner selects the hosting.

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

## Final test summary

Simulated (hardware-free) tests: 15 run, 15 passed (9 unit, 6 integration). Hardware tests: 0 run, 0 passed.
