# Bahtinov mask focusing in INDIGO

Revision: 20.11.2026

Author: **Peter Polakovic**

e-mail: *peter.polakovic@cloudmakers.eu*

This is the technical reference for Bahtinov mask analysis in
`indigo_libs/indigo_bahtinov.c` and its use by the Imager Agent. It connects the
optical pattern to the measured pixel error, explains how the agent learns mask
angles and moves a focuser, and records the limits of the current implementation.
The agent's general setup is in its [README](../README.md); bus properties are
listed in [Properties](../../../indigo_docs/PROPERTIES.md#imager-agent).

The algorithm description follows the source. Optical guidance and synthetic
test coverage are identified separately; neither is a claim of accuracy on every
mask, telescope, filter or camera. The analysis rewrite is credited separately
under [Attribution and references](#attribution-and-references).

## Contents

- [Optical principle](#optical-principle)
- [Components and data flow](#components-and-data-flow)
- [Image preparation and star location](#image-preparation-and-star-location)
- [Finding the three spikes](#finding-the-three-spikes)
- [Geometry, coordinates and signed error](#geometry-coordinates-and-signed-error)
- [Acceptance and confidence](#acceptance-and-confidence)
- [Imager Agent integration](#imager-agent-integration)
- [Iterative mechanical autofocus](#iterative-mechanical-autofocus)
- [Limits and practical use](#limits-and-practical-use)
- [Existing test coverage](#existing-test-coverage)
- [Attribution and references](#attribution-and-references)

## Optical principle

A Bahtinov mask divides the telescope entrance pupil into regions containing
parallel slots at three orientations. Each region acts as a diffraction grating.
Its positive and negative diffraction orders lie along an axis perpendicular to
the slots: the familiar six rays are three lines, each extending on both sides
of the star. They are not six separate focus measurements.

For grating period `p`, wavelength `λ` and order `m`, the small-angle picture is
`sin(α) = m λ / p`; near the focal plane the order displacement is approximately
`f α`, where `f` is focal length. This describes spacing along a spike, not the
focus error measured below. Broadband light spreads orders of different
wavelengths into extended spikes. A narrow passband produces distinct spots;
multiple narrow passbands can produce several sets of spots along each line.
For equal-period mask regions, matching orders occur at matching radii. The
INDIGO matched filter uses that correspondence.

The regions also sample different parts of the pupil. Defocus changes where
light from those regions meets the detector, so their diffraction patterns
shift relative to one another. In the intended focused geometry, the central
spike passes through the intersection of the two outer spikes. On opposite sides
of focus, it lies on opposite sides of that intersection. Thus the useful
measurement is the **perpendicular distance between the outer-line intersection
and the central line**, not the distance between diffraction orders and not the
width of the star core. This geometric focus criterion follows the pupil-zone
explanation in [van den Born, Jellema and Dijkstra (2022)](https://doi.org/10.1093/mnras/stac845).

Close to focus this distance is a useful local indicator of defocus. Converting
it to detector displacement in micrometres, optical wavefront error or focuser
motor steps would require optical and mechanical calibration. INDIGO does not
perform that conversion. A small pixel error characterizes this star through
this mask; it does not establish collimation or uniform focus across the field.

## Components and data flow

| Component | Responsibility |
| --- | --- |
| [indigo_bahtinov.c](../../../indigo_libs/indigo_bahtinov.c) | Pixel conversion, star location, line scoring, central-spike subtraction, outer-line search, geometry and acceptance checks. |
| [indigo_bahtinov.h](../../../indigo_libs/indigo/indigo_bahtinov.h) | `indigo_bahtinov_result`, analyzer API and compatibility entry points. |
| [indigo_agent_imager.c](../indigo_agent_imager.c) | RAW capture, frame-size gate, configured/learned angles, spike/statistic publication and iterative focuser control. Relevant functions are `bahtinov_focus_error()`, `capture_and_process_frame()`, `estimator()` and `autofocus_iterative()`. |
| [indigo_names.h](../../../indigo_libs/indigo/indigo_names.h) | Public property and item names used by clients. |

```text
RAW pixels -> intensity + 2x2 smoothing -> background/noise + star centre
           -> core radius and usable annulus -> strongest central line
           -> central radial template + central-line subtraction
           -> two outer lines (free angles or known-angle windows)
           -> intersection + signed distance + reliability checks
           -> Imager: absolute pixel error + three overlay lines
           -> quality comparison -> relative focuser moves -> next exposure
```

The current analyzer is an intensity-based line search. It does **not** call
`indigo_binarize()` or `indigo_skeletonize()`. Those exported helpers remain in
the same source: the former thresholds at mean plus `sigma × standard deviation`
and rejects more than 10% bright pixels; the latter thins a binary image. Their
thresholds and behavior are not stages or rejection rules of the current analyzer.

`indigo_bahtinov_error()` is a compatibility wrapper: it calls
`indigo_bahtinov_analyze()` with free angle search, ignores its `sigma` argument,
returns the absolute error on success and `-1` on failure, and clears its six
line outputs on failure. The agent calls the richer analyzer directly.

## Image preparation and star location

The analyzer consumes a pixel buffer, type, width and height. File decoding and
the INDIGO RAW header are the caller's responsibility. Supported types are
`INDIGO_RAW_MONO8`, `MONO16`, `RGB24`, `RGBA32`, `ABGR32` and `RGB48` (the latter
five have the same `INDIGO_RAW_` prefix). Monochrome samples become floats;
colour samples are the sum of R, G and B, ignoring alpha. There is no colour
weighting or wavelength calibration.

1. A 2 × 2 box filter suppresses the Bayer sampling pattern without debayering.
   It keeps the image dimensions and uses the last row/column again at an edge.
   Subsequent line samples use bilinear interpolation with the filter's
   half-pixel placement accounted for.
2. A regularly subsampled image supplies the median background and
   `1.4826 × median absolute deviation` noise estimate. The stride is
   `max(1, floor(width × height / 65536))`; this is not a hard 65,536-sample cap.
   A nonpositive noise estimate becomes 1. The background is subtracted.
3. A further 3 × 3 average is used only for star location. The intensity-weighted
   centroid includes all pixels above 30% of that image's peak. There is no
   connected-component selection or multi-star deblending here. A nonpositive
   weight sum or peak below five times the noise rejects the frame.
4. The core radius is the first integer radius, starting at 2 pixels, whose
   azimuthal median is below the noise. The fallback radius is 3 if no ring
   crosses that threshold before half the smaller image dimension.
5. Samples exclude the core. The outer radius is initially `8 × core + 20`
   pixels, limited by the closest frame edge minus 4 pixels. Less than 10 pixels
   between inner and outer radii rejects a star as too large or too near an edge.

The analyzer measures the supplied image as a whole. A tight, properly centred
camera subframe around one star is therefore preferable to relying on an Imager
selection marker to isolate a star within a larger frame.

## Finding the three spikes

### Line score and central line

For each candidate line, the analyzer samples both sides of the core every
0.5 pixel. At each point it subtracts the mean of samples on two parallel lines,
3 pixels to either side. Summing this contrast suppresses a smooth stellar halo
and background while retaining a narrow diffraction line.

The central spike is assumed to be the strongest line. Its initial search covers
angles from −90° through 89° in 1° increments and perpendicular offsets from
`−core` through `+core` in 1-pixel increments. `bahtinov_refine()` then performs
five local angle/offset searches, halving the search increments at each iteration.
The subpixel search resolution is not a statement of measurement accuracy.

### Radial template and subtraction

The central line's side-band-subtracted profile is smoothed over up to five
samples, separately on each side of the core. Negative weights become zero;
the vector is normalized by its Euclidean norm. Outer-line scores use these
weights, so radial positions where the central spike is bright contribute more.
This is especially useful for narrowband diffraction spots that would not form
a continuous skeleton.

A separate model estimates the central spike's amplitude along its length and
its mean transverse profile over ±8 pixels. Their product is subtracted from a
copy of the image. If a usable model cannot be built, the outer search proceeds
on the original background-subtracted image. In free search, outer samples
within 1.5 pixels of the fitted central line are excluded; known-angle mode
reduces that exclusion width to zero.

### Outer-line angles

A known pair is active only when a non-NULL `mask_angle` has both entries nonzero.
The array convention is a negative then a positive relative angle, in degrees;
the search uses their magnitudes on the two respective sides of the central line.

| Search | Initial angle grid and pair selection |
| --- | --- |
| Free | Each outer line is 4°…40° from the central line, sampled every 0.25°. The magnitudes of the pair's angles may differ by at most 1.5°. Choose the largest sum of the two scores. |
| Known | Search each supplied magnitude within ±0.5°, with a 0.05° grid and an initial lower grid bound of at least 1°. The two supplied magnitudes may differ; the free-search symmetry constraint is not applied. |

Each grid angle searches offsets from `−core` to `+core` every 0.5 pixel. The
winning lines receive local angle/offset refinement. The free-search symmetry
check selects the initial pair; the final independently refined angles need not
remain within exactly 1.5° of one another in magnitude.

Known-angle mode narrows the hypothesis space and can recover faint or dotted
patterns that free search cannot identify confidently. It is a prior, not proof
that the entered mask angle is correct. An incorrect known angle can still pass
and return a wrong focus error.

## Geometry, coordinates and signed error

Every exported spike uses normal form:

```text
x cos(theta[i]) + y sin(theta[i]) = rho[i]
```

Coordinates are pixel indices in the supplied frame: the first pixel centre is
`(0, 0)`, x grows with column index and y with row index (downward in a usual image
display). `rho` is in pixels and can be negative. `theta` is in radians in
`[0, π)`, an angle of the **normal**, not a slope. The line direction can be written
`(sin(theta), −cos(theta))`: theta 0 denotes a vertical line, theta π/2 a horizontal
line. Rotating or flipping a displayed image requires transforming these lines
with the image. A camera subframe's coordinates are local to that frame.

Index 0 is the central line, index 1 the negative-relative-angle outer line,
and index 2 the positive-relative-angle outer line. `angle[0..1]` are relative
angles in degrees computed before canonicalizing the exported line normals;
do not subtract canonical `theta` values without handling the π wrap.

Internally, let `tc`, `tl`, `tr` and `rc`, `rl`, `rr` be the central and two outer
normal angles and offsets. The outer intersection `P = (px, py)` is:

```text
det = cos(tl) sin(tr) − sin(tl) cos(tr)
px  = (rl sin(tr) − rr sin(tl)) / det
py  = (rr cos(tl) − rl cos(tr)) / det
error = px cos(tc) + py sin(tc) − rc
```

The last expression is a signed perpendicular distance because the normal is a
unit vector. A positive error places the intersection on the positive side of
the internal central normal. It does **not** mean a universal “move inward” or
“move outward”: mask orientation, camera transforms and focuser mechanics matter.
The code computes `error` before normalizing exported angles. Normalization may
reverse a line's normal and `rho`, preserving the line but reversing a distance
recomputed from that exported normal. Use `result.error` for the analyzer's sign;
use `fabs(error)` to compare focus quality independent of that convention.

For example, take a vertical central line with internal `tc = 0` and `rc = 152`,
and outer lines crossing at `(150, 150)`. The measured error is `150 − 152 = −2`
pixels, and the magnitude published by the agent is 2 pixels. Moving the central
line to x = 150 makes the error zero; moving it to x = 148 makes the error +2.
These coordinates describe geometry only, not a two-step motor command.

## Acceptance and confidence

The function returns `true` only after the geometric and score checks pass.
It zeros the result at entry, but may populate diagnostic fields before a later
rejection. A caller must check the boolean result before using those fields as
a valid measurement.

| Check | Current rule |
| --- | --- |
| Input | Non-NULL pixels, supported type, width and height each at least 32. The caller supplies valid buffers and a writable result. |
| Star and annulus | Peak ≥ 5 × image noise, positive centroid weight, usable radial span ≥ 10 pixels. |
| Line search | A finite winning outer pair and positive plain/matched score-noise estimates. |
| Intersection | Absolute determinant at least `1e-9`; intersection within `0.5 × core` pixels of the estimated star centre. |
| Central line | `snr[0] ≥ 8`, in both search modes. |
| Free outer search | Confidence ≥ 1.2. There is no additional per-outer-line SNR threshold in this mode. |
| Known outer search | Each outer line's SNR ≥ 2.75. Free-search confidence is not used and is reported as 0. |

Line SNR is line score divided by a robust score-noise estimate; it is not the
star's conventional photometric SNR. The implementation generates 400 candidate
noise lines deterministically, retaining those at least 50° from the central
line, then uses `1.4826 × median(abs(score))` for the plain and weighted scores.
These are estimated before central-spike subtraction.

Free-search confidence compares the best pair score to the strongest competing
symmetric pair whose mean angle magnitude differs by at least 3°. If that
competitor's score is positive, confidence is their ratio; otherwise it is set
to 10. It is neither a probability nor an error bar, and 10 is a fallback value,
not evidence of ten-sigma detection. The crossing-distance gate is distinct from
the focus error: it compares the outer intersection to the star centroid,
whereas focus error compares that intersection to the central line.

## Imager Agent integration

### Settings and published values

| Property/item | Meaning |
| --- | --- |
| `AGENT_IMAGER_FOCUS_ESTIMATOR.BAHTINOV` | Selects “Iterative (Bahtinov)” for preview measurements and autofocus. |
| `AGENT_IMAGER_FOCUS.BAHTINOV_ANGLE` | A positive value supplies symmetric relative angles `{−a, +a}` in degrees. Zero enables learning; property range 0…45°, default 0. This is separation from the central spike, not rotation on the detector. |
| `AGENT_IMAGER_STATS.BAHTINOV_ERROR` | Absolute error in pixels, or `−1` when unavailable. This sentinel is used despite the property's declared nonnegative range. |
| `AGENT_IMAGER_SPIKES.RHO_1/THETA_1` through `RHO_3/THETA_3` | The three lines, central first. Theta values are radians despite the current display labels saying “(px)”; rho can be negative despite the declared nonnegative ranges. |
| `AGENT_IMAGER_STATS.FOCUS_POSITION` / `FOCUS_OFFSET` | Focuser position associated with measurement / relative displacement tracked by iterative autofocus, in focuser steps. |

SNR, confidence, detected relative angles and the signed error are available in
the analyzer result and debug logs, not as extra statistics items. A detected
angle change also produces a human-readable message.

### Learning and fallback

`bahtinov_focus_error()` chooses angles in this order:

1. A positive configured angle takes precedence. It tries known-angle analysis
   only. Failure does not trigger free search while this setting is positive.
2. With setting zero and a previously learned pair, it first tries that pair.
   If it fails, it tries free search. This replacement measurement must have
   confidence at least 1.5, stricter than the library's 1.2 minimum.
3. With no configured or learned pair, it starts with free search. A result at
   confidence 1.2…less than 1.5 can be published, but is not learned.
4. A successful free result with confidence ≥1.5 updates the learned pair if
   either relative angle differs by more than 0.5° from the stored value.

The learned pair is per-agent private memory, not written back into
`BAHTINOV_ANGLE` or saved as a separate configuration property. The explicit
angle setting is saved with the focus settings. Setting zero resumes learning
mode but does not clear a previously learned pair. A changed mask is rediscovered
only if the known-angle attempt fails and the free result is sufficiently
confident; a wrong prior that still passes cannot be detected by this fallback.

### Capture and failure reporting

The agent processes Bahtinov frames only when both dimensions are at most 500
pixels. Preview/autofocus start emits a warning for a larger configured frame,
but still starts the process; capture processing then reports `−1` for an oversized
received frame. This is an agent gate, not a library maximum or automatic resize.
Bahtinov autofocus does not use the HFD path's automatic selection subframe;
configure an appropriate camera frame beforehand. Macro mode skips these focus
statistics in `capture_and_process_frame()`.

When the analyzer is called and rejects the pattern, the agent sets all six
spike numbers to zero and returns `−1`. When an oversized frame bypasses the
analyzer, previous spike numbers can remain. Spike-property state alone therefore
does not validate an overlay: clients should require a current, nonnegative
`BAHTINOV_ERROR` from the corresponding processed frame. A preview can complete
successfully as a capture even when its focus statistic is `−1`.

## Iterative mechanical autofocus

The Bahtinov analyzer measures one frame; the shared iterative controller decides
how to move. It uses `quality = 100 − BAHTINOV_ERROR`, rejecting errors below 0
or above 100 as unavailable. Thus improving quality means reducing the absolute
pixel error. It neither follows the sign nor fits a calibrated pixel-to-step
relation, and it does not use the HFD U-curve algorithm.

At each position it tries up to 20 captures to obtain `STACK` valid measurements
(default 3, range 1…5). It keeps the **maximum quality**, equivalent to the smallest
error, rather than averaging or stacking pixels. Fewer than the requested number
can be used if at least one succeeds; no valid frame or quality zero causes the
loop to stop with “Failed to evaluate quality”. An error of exactly 100 has zero
quality and cannot form a usable position sample on its own.

The initial direction is outward, with `ITERATIVE_INITIAL` steps (default 20).
While quality is at least the previous position's quality, it continues in that
direction. When quality falls it reverses direction; after the first reversal,
subsequent reversals halve and round the step, with a minimum of one. Once a
reversal is needed at or below `ITERATIVE_FINAL` (default 5), it moves one current
step back towards the previous position and ends the search. The relative travel
limit for Bahtinov uses the shared non-HFD value, `40 × ITERATIVE_INITIAL`.
It is a bound on the tracked offset from the start, not total path length or
physical end-stop protection.

Moves use the shared relative focuser command and wait for its BUSY/OK transition.
`BACKLASH` and `BACKLASH_OVERSHOOT_FACTOR` control the existing compensation path,
taking account of whether the focuser advertises backlash support; overshoot mode
can temporarily disable driver compensation. These settings are in mechanical
steps and must suit the hardware. Good optical measurements cannot remove
uncompensated play, slipping, or inadequate step resolution.

A final capture supplies the final statistics. Bahtinov completion checks the
travel limit and whether the final estimator is valid. There is **no explicit
“error below N pixels” acceptance threshold** and no Bahtinov-specific rejection
based on `BEST_FOCUS_DEVIATION`. That percentage is relative to qualities sampled
during this run, not an optical tolerance. Some early loop exits proceed to the
same final evaluation, so an intermediate failure message alone does not describe
the final process outcome. Check `AGENT_START_PROCESS` and the final measurement.

Ordinary focus-failure evaluation can request return to the starting position via
`AGENT_IMAGER_FOCUS_FAILURE.RESTORE` (the default, also used for Bahtinov despite
the property's “Peak / HFD” label). Abort and capture/transport error exits do not
guarantee this recovery; a restoration command also needs a working focuser.
`REPEAT` supplies additional attempts after failure, with `DELAY` doubled between
attempts; success stops retries and abort stops the run. The outer autofocus
wrapper restores camera format/upload mode and focuser mode, and publishes
finished/failed/aborted status. Inspect the actual position after interruption
before assuming the original focus was restored.

## Limits and practical use

The numeric bounds above are implementation facts. The following practical
consequences explain why a measurable pattern still needs sensible acquisition:

- **Use one isolated star near the frame centre.** The centroid includes all
  sufficiently bright pixels; companions, crowded fields, trails, hot pixels
  or reflections can bias it or win the central-line search. The analyzer has
  no multi-star association, bad-pixel map or dedicated trail rejection.
- **Start near focus with enough visible spike length.** The core exclusion,
  annulus size, bounded line offsets and crossing gate assume a recognizable
  pattern. Severe defocus, clipped rays or a star close to an edge can make the
  result unavailable. A small crop must still retain the useful diffraction
  orders; shrinking it indefinitely is counterproductive.
- **Expose for the pattern without clipping it.** Low signal weakens the line
  scores; saturation and blooming distort the core, centroid and line profiles.
  The Bahtinov path has no dedicated saturation mask or saturation rejection;
  the RMS estimator's masking is a different branch. Seeing and tracking errors
  also change line width and the best-of-frames statistic.
- **Learn on a clear pattern when possible.** Broadband data often make free
  search easier. A known angle can help narrowband data, but fixed angles are
  not independently verified, and successful analysis cannot certify a prior.
  The free search covers only approximately symmetric 4°…40° geometry. The
  agent can configure up to 45°; that property range is not free-search coverage.
- **Use a compatible three-region mask.** The central line must be strongest
  and the radial template assumes comparable grating periods across regions.
  Strong telescope spider diffraction, unequal periods, extra orientations or
  a multi-pattern mask are outside that simple model. No dedicated multi-Bahtinov
  collimation analysis is implemented.
- **Keep acquisition scale consistent.** Error is in pixels of the delivered
  image, including any camera binning. A pixel value from another binning or
  optical scale is not the same physical tolerance. Filter changes can alter
  diffraction spacing and best focus; the algorithm does not calibrate chromatic
  focus shifts. Remove the mask before science imaging.

The library allocates full-frame float arrays for intensity, filtering and
central subtraction, with two such arrays alive at the main image-processing
stages, plus profiles and search buffers. Searches repeatedly sample candidate
lines; work grows with the usable radius, core-offset range and angular grid.
There is no library 500-pixel cap or in-analysis abort callback. The agent's small
frame gate limits this synchronous work; it does not make arbitrary direct
library inputs equally cheap.

## Existing test coverage

These are descriptions of checked-in tests, not a record that a hardware run
was performed for this document. [test_bahtinov.c](../../../indigo_test/unit/test_bahtinov.c)
contains 12 registered synthetic unit cases. Its 300 × 300 patterns use a Gaussian
star, known line offsets, deterministic noise, and broadband or dotted profiles.
The source notes that their geometry follows Poseidon-C PRO frames; synthetic
success is not a general camera accuracy specification.

| Covered behavior | Existing checks |
| --- | --- |
| Pixel-error magnitude | Broadband offsets 0, ±1.5, ±4 and +8 pixels, with 0.35-pixel absolute-error tolerance. |
| Sign and orientation | Opposite central offsets produce opposite signs; vertical, horizontal and oblique patterns are measured. The sign test does not calibrate a motor direction. |
| Free-angle diagnostics | A 20° mask reports angles within 0.5°, confidence ≥1.2 and central SNR ≥8. |
| Narrowband | Dotted patterns with known angles at several offsets; one brighter free-search dotted pattern. |
| Pixel formats | MONO16 throughout, one simulated Bayer mosaic, and explicit MONO8/RGB24 conversion cases. RGBA32, ABGR32 and RGB48 support is visible in source but not separately exercised here. |
| Rejection and compatibility | Noise-only frame, star without mask, absolute-error wrapper, canonical line angles/coordinates and cleared wrapper outputs on failure. |

The agent suite [test_agent_imager.c](../../../indigo_test/integration/test_agent_imager.c)
has `bahtinov_preview_and_focus`, which checks a nonnegative preview measurement
and completed autofocus with initial/final steps 2/1, and
`bahtinov_configured_mask_angle`, which checks preview and published relative
angles for the simulator's 20.1° mask. These use the CCD Bahtinov Mask Simulator.
They do not establish an optical error tolerance after autofocus, real backlash
performance, or comprehensive angle-learning/fallback behavior.

The cited tests do not exhaustively validate saturation, multiple stars, edge
clipping, wrong-prior acceptance, all angle/size boundaries or all real telescope
aberrations. Their assertions support the specific synthetic and integration
contracts above; the source-derived limits and acquisition guidance remain
separate from that coverage.

## Attribution and references

The implementation and public-header version history credit **Rumen Bogdanovski**
(rumenastro@gmail.com) with moving Bahtinov analysis from `indigo_raw_utils.c` and
rewriting it for version 3.0. Both files carry CloudMakers and Rumen Bogdanovski
copyright notices. That implementation credit is distinct from this document's
author line. The code uses the [INDIGO Astronomy open-source license](../../../LICENSE.md).

- [van den Born, Jellema and Dijkstra (2022), *Demonstration of an imaging technique for the measurement of PSF elongation caused by Atmospheric Dispersion*](https://doi.org/10.1093/mnras/stac845): optical discussion of Bahtinov pupil regions, diffraction orders and focus geometry.
- [Analysis source](../../../indigo_libs/indigo_bahtinov.c) and [API contract](../../../indigo_libs/indigo/indigo_bahtinov.h): authoritative implementation, constants, result units and compatibility behavior.
- [Imager Agent source](../indigo_agent_imager.c) and [property reference](../../../indigo_docs/PROPERTIES.md#imager-agent): capture, learning, publication and focuser control.
- [Library tests](../../../indigo_test/unit/test_bahtinov.c) and [agent tests](../../../indigo_test/integration/test_agent_imager.c): specific existing automated assertions.
