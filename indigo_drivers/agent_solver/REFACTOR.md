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

- Unit (`test_agent_solver_core`): limits and cone filter; noisy frame with defects detected and cleaned; tangent plane round trip and far hemisphere rejection; hash component packing and
  clamping; quad code invariance under translation, rotation, scale and mirroring; canonical code first among
  variants and code bounds; index file round trip, truncated file, bad magic, bad version, dangling star reference,
  unsorted input; index selection by field size and presence; synthetic field solve, mirrored field solve, unrelated
  field rejected.
- Integration (`test_agent_solver`): installed index reflected in `X_AGENT_SOLVER_INDEX` and
  `AGENT_PLATESOLVER_USE_INDEX`; RAW frame solved at full resolution (centre within 10", scale within 0.3%, angle
  within 0.2°, parity, index, epoch); downsampled solve reported in unbinned pixels; refusal without a selected
  index; the same frame uploaded as FITS written by `indigo_raw_to_fits()`, as floating point FITS (BITPIX -32,
  values far outside 16 bits), as 8 bit mono RAW, as 16 bit RGB RAW and as grayscale JPEG solved to the same
  solution; data in no known format and a FITS shorter than its header refused; deleting an index removes the
  file and withdraws the item.
- Not covered: the download path (network), camera raw files through libraw (no fixture; the decoder is the one the
  Astrometry agent uses), Linux and Windows builds.

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

## Noise reduction before star detection (one-off 1000 setup sweep, 2026-10-27)

A one-off sweep of 1000 `SIMULATION_SETUP` combinations (40 positions down to Dec ±89°, ten rotations, twelve frame
sizes from 640 x 480 to 3000 x 2000, magnitude limits 8.5 to 10, noise 100 to 400, four gradients, six hot pixel,
column and row settings; not kept in the suite, which stays at 120) was stopped after 529 cases: 520 solved, 9 did
not. All nine were sparse fields (RA 0.85 h Dec −27.1°, RA 13.00 h Dec +50°, RA 2.35 h Dec +89°) at 13 to 15"/px
with IMAGE_NOISE_VAR 250 to 400; astrometry.net solved every one of them through the Astrometry agent. The cause was
the star detection: the simulator draws a magnitude 8 star with a peak of 500 ADU and sigma 1.4 px, a magnitude 9
star with 200 ADU, over uniform noise of sigma 72 ADU at IMAGE_NOISE_VAR 250, and `indigo_find_stars_precise_threshold()`
decides per pixel (local maximum above median + 3.5 sigma with six of eight neighbours above it), so it found 7 to 8
stars where 15 were rendered and no quad matched.

`indigo_solver_image_stars()` now detects mono frames on a filtered copy (`indigo_solver.c`): the pixel noise sigma
is estimated from the median absolute difference of adjacent pixels; pixels standing more than 10 sigma above the
median of their eight neighbours whose horizontal or vertical neighbours carry less than 20 % of that excess (hot
pixels, hot columns, hot rows; a star image has both) are replaced by that median, repeated until nothing is left
so crossings of defects go too; then a separable gaussian blur of sigma 1 px matched to the star images raises the
peak signal to noise ratio about 2.4 times. The frame itself is left untouched, the centroid of a symmetric star
image is unchanged by the symmetric kernel, colour frames are detected as before. The nine failed setups now solve
with 12 to 34 detected stars and centre errors of 0.5 to 2.3"; a 1600 x 1200 frame with 200 hot pixels, a 600 px hot
column and an 800 px hot row has every defect replaced and solves. Version 3.0.0.6. Regression:
`image_stars_noisy_frame` (unit): a frame with 30 gaussian stars of 200 to 500 ADU over noise of sigma 72, 20 hot
pixels, a hot column and a hot row; every star is detected within 1.5 px, no defect is, the frame is unchanged.

## Input formats (2026-10-27)

INDIGO RAW stays the preferred input and MONO16 frames are solved as they are. `indigo_solver_load_image()`
(`indigo_solver.c`) turns every other input into a RAW MONO16 image, the way the Astrometry agent turns every input
into FITS: RAW MONO8 is spread over 16 bits, RAW RGB24/RGB48 are averaged to mono; FITS primary images with BITPIX
8, 16, 32, -32 and -64, BZERO/BSCALE and NAXIS 2 or 3 (planes averaged) are read, physical values within 0 to
65535 kept and any other range mapped onto it; JPEG is decoded with libjpeg and its components averaged; anything
else goes through libraw like in the Astrometry agent. The agent reports the reason when a file cannot be read
(truncated RAW, truncated FITS header or data, unsupported FITS, broken JPEG, unsupported image format). Since
every image is MONO16 after loading, the downsampling hint applies to all of them. Version 3.0.0.7.

Found by the format tests: the noise estimate of the matched filter was the median absolute difference of
adjacent pixels, which is 0 on 8 bit and JPEG data spread over 16 bits (most adjacent pixels are equal), so one
quantisation step of 257 looked like a 257 sigma spike and most of the frame was "repaired". The estimate is now at
least the quantisation step, the smallest non-zero difference. Found by the Rosetta frame downsampled 2 x 2: stars
sharper than sigma 0.7 px passed the 20 % neighbour rule as spikes (900 pixels replaced); the rule is now 10 %, which
keeps stars down to sigma 0.5 px and still removes saturated hot pixels, columns and rows.

Simulator coverage (`test_agent_solver_simulator`): the Rosetta frame captured by the Imager Agent as FITS and as
JPEG, and every fifteenth guider setup captured as FITS, solved like the RAW captures.

## Hints, limits, inputs and diagnostics (task 20261027192244-16bd-solver-hints-input-diagnostics)

Version 3.0.0.8 uses the shared hints the way the Astrometry agent does where the native solver can:

- RADIUS/RA/DEC: `indigo_solver_filter_quads()` keeps the hash ordered catalogue quads whose four stars lie within
  the radius plus the field extent (half the diagonal from the pixel scale, or the index scale when the scale is
  unknown) of the hint, converted from the hints epoch to J2000. A field outside the cone leaves no quads and fails
  with "No solution found". In the sweep a 3° hint keeps 9 000 to 19 000 of 6 to 12 million quads.
- PARITY: a solution of the other parity is rejected and the search continues; when nothing else is found the
  message is "Solution has the wrong parity". The quad codes are parity invariant (canonical flip), so the hint
  cannot prune the candidate search; recorded as a limitation.
- DEPTH: progressive stages over the brightest stars (the DEPTH hint then all, or 40, 100, all); each stage builds
  its own image quads and matches; the Rosetta frame solves in the first stage with 40 stars.
- CPU_LIMIT and abort: `indigo_solver_limits` carries a monotonic deadline and the agent's abort flag into
  `indigo_solver_match_quads()` (hash and voting loops) and `indigo_solver_match_stars()`, checked every 4096
  iterations (64 in the quadratic voting loop); the solve fails with "CPU time limit exceeded" or "Aborted".
- Index order: with a known scale the fitting index first, then the other selected ones ordered by the ratio of
  their scale to the field size; the loaded index stays cached between solves (debug message "already loaded").
- Bayer: `indigo_solver_load_image()` reports a BAYERPAT appendix of a RAW frame or a BAYERPAT card of a FITS; the
  agent bins at least 2 x 2 (odd factors raised) before detection, the WCS stays in unbinned pixels.
- Colour: RAW RGB24/RGB48, three plane FITS, RGB JPEG and libraw colour output are reduced with the luminance
  weights 0.299 R + 0.587 G + 0.114 B instead of the average.
- Message: `indigo_solver_compute_center()` returns the inliers and the residual; the agent reports
  "Solved: N of M stars matched, rms X.XX"".

Tests: unit `match_respects_limits_and_cone` (deadline in the past and abort flag stop the matching with the
reason, a far deadline solves, the cone keeps the field's quads in hash order and none 30° away); agent
`solve_with_position_hint` (J2000 and JNow hints), `wrong_position_hint_fails`, `parity_hint_rejects_mirror`,
`solve_with_depth_hint`, `bayer_raw_is_binned`, `bayer_fits_is_binned`, `solve_rgb_fits_image`,
`solve_colour_jpeg_image` and the message assertion in `solve_raw_image`; simulator: every twenty-fifth guider
setup solved with a 3° position hint one degree off the rendered centre in the hints epoch (JNow setups report
the solution in JNow and are compared after conversion). Not covered: a CPU-limit integration case (not
deterministic on a fast host), libraw colour (no fixture), a simulator Bayer capture (only the file simulator
attaches BAYERPAT).

## Final test summary

Simulated (hardware-free) tests: 160 run, 160 passed (11 unit, 19 agent integration, 130 simulator integration:
10 Rosetta cases and 120 guider setups, 8 of them as FITS, 3 with a position hint and 6 through the Guider Agent).
Hardware tests: 0 run, 0 passed. Recorded by `tools/run_driver_test.py agent_solver` on 2026-10-27 as 149/149
(agent and simulator integration).
