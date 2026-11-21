# Native INDIGO plate solver and index builder

Revision: 20.11.2026

Author: **Peter Polakovic**

e-mail: *peter.polakovic@cloudmakers.eu*

This is the technical reference for the native `indigo_agent_platesolver`, the
`indigo_solver` command line tool and `indigo_solver_indexer`. They share quad
geometry and the ISIX index format; solving requires no external astrometry
executable. The agent's user setup is in its [README](../README.md), and the
bus properties are described in [Properties](../../../indigo_docs/PROPERTIES.md).

The descriptions below follow the implementation. Commands under catalogue
acquisition, building and publishing are operational instructions, not a record
that those operations have been performed. Published asset metadata was checked
on 2026-11-20; the catalogue contents and publication dates can change independently
of the software.

## Contents

- [Components and data flow](#components-and-data-flow)
- [Image decoding and star detection](#image-decoding-and-star-detection)
- [Quad construction and invariant codes](#quad-construction-and-invariant-codes)
- [Search and correspondence verification](#search-and-correspondence-verification)
- [Astrometric fit and outputs](#astrometric-fit-and-outputs)
- [Comparison with Astrometry.net and ASTAP](#comparison-with-astrometrynet-and-astap)
- [Limits, concurrency and memory](#limits-concurrency-and-memory)
- [Index generation](#index-generation)
- [Catalogue acquisition and index builds](#catalogue-acquisition-and-index-builds)
- [ISIX version 2 file format](#isix-version-2-file-format)
- [Distribution, installation and updates](#distribution-installation-and-updates)
- [Adding scales or catalogues](#adding-scales-or-catalogues)
- [Attribution and references](#attribution-and-references)

## Components and data flow

| Component | Responsibility |
| --- | --- |
| [indigo_agent_platesolver.c](../indigo_agent_platesolver.c) | Native backend callbacks, selected-index management, progressive solve orchestration, persistent mapping of the last index, INDIGO WCS publication and abort handling. |
| [indigo_platesolver.c](../../../indigo_libs/indigo_platesolver.c) and [public header](../../../indigo_libs/indigo/indigo_platesolver.h) | Shared plate-solver agent layer: related Imager/Guider/Mount agents, image BLOB reception or URL loading, asynchronous task ownership, preview, solve/sync/center and polar-alignment workflows. Calls the selected backend's `solve` and `abort` callbacks. |
| [indigo_solver.c](../indigo_solver.c) and [header](../indigo_solver.h) | Image conversion/filtering, image quads, index mapping/building/writing/downloading, hash and star matching, robust tangent-plane affine fit. |
| [indigo_solver_utils.c](../indigo_solver_utils.c) and [header](../indigo_solver_utils.h) | Projection, angular distance, canonical quad codes, hash layout, format constants, index names and header reader. |
| [indigo_solver_healpix.c](../indigo_solver_healpix.c) and [header](../indigo_solver_healpix.h) | HEALPix RING angle/pixel conversion and neighbours for index construction; the runtime matcher uses code buckets, not HEALPix queries. |
| [indigo_raw_utils.c](../../../indigo_libs/indigo_raw_utils.c) | Shared background estimation, star detection, centroid and PSF measurements. |
| [CLI solver](../../../indigo_tools/indigo_solver.c) | File input, options/hints, the same staged solving pipeline, text/JSON/XML/WCS output and index installation commands. |
| [Indexer](../../../indigo_tools/indigo_solver_indexer.c) | Tycho-2/Gaia parsing, proper motion, sky sampling, quad generation, star compaction and ISIX output. |

```text
Catalogue files -> parse + proper motion -> HEALPix sampling -> sky quads
                -> ambiguity variants -> compact stars + sorted arrays -> ISIX
                                                                        |
Image/BLOB -> MONO16 + binning -> filtering + centroids -> image quads     |
                -> stage/index selection -> code-bucket search <---------+
                -> shared-correspondence votes -> scale consistency
                -> one-to-one star matches -> robust affine TAN fit
                -> centre / scale / angle / parity / residual -> agent or CLI
```

The shared agent owns the original task image and releases it after the backend
returns. The native backend owns temporary converted images and matching arrays.
An `indigo_solver_catalog` either owns constructed arrays, owns a mapped file,
or is a cone view sharing another catalogue's arrays and owning only a star mask.
Free the cone view before freeing its parent. `indigo_solver_free_catalog()`
releases owned resources and clears the structure.

## Image decoding and star detection

### Input normalization

`indigo_solver_load_image()` detects the input from its bytes, returns a newly
allocated INDIGO RAW MONO16 image, and separately reports the original dimensions,
actual bin factor and detected Bayer status. It checks supported geometry and
payload lengths before conversion. Recognized inputs are:

| Input | Conversion |
| --- | --- |
| INDIGO RAW MONO16 | Preserve intensity values; bin directly from the source. |
| INDIGO RAW MONO8 | Multiply samples by 257 to span the 16-bit range. |
| INDIGO RAW RGB24/RGB48 | Luminance `0.299 R + 0.587 G + 0.114 B`; RGB24 is also expanded by 257. |
| FITS primary image | `BITPIX` 8, 16, 32, -32 or -64; `NAXIS` 2 or 3; up to 16 planes. Decode big-endian stored values and apply `BZERO + BSCALE × value`. Non-finite physical values become zero. |
| JPEG | libjpeg decoding and luminance weighting. |
| Other camera raw | libraw through INDIGO's DSLR decoding interface, then luminance conversion. Unsupported data fails decoding. |

For three colour channels the weights are the RGB luminance weights; other
multi-plane FITS channel counts use equal weights. FITS physical values already
inside 0…65535 retain their range; an image with values outside that range is
linearly mapped into it. Floating point images wholly inside that range are
rounded to integers, not automatically stretched to fill 16 bits. A deliberate
compatibility case reads FITS `BITPIX=16`, `BZERO=0`, `BSCALE=1` as unsigned:
negative signed samples wrap above 32767. This is not general signed-FITS behavior.

A RAW mono `BAYERPAT` appendix or FITS `BAYERPAT` card forces an even bin factor
of at least 2; a requested odd factor is raised to the next even value. This
averages the mosaic before star detection rather than demosaicing that input.
The ordinary default is 2×2. Binning averages each complete bin×bin block with
integer division; incomplete right/bottom blocks are discarded. Single-plane
FITS and RAW MONO16 can convert/bin without allocating a full-size converted
frame. Other paths may need full-size intermediate data.

### Defect removal and filtering

The detector works on a copy, leaving the normalized image intact. For mono
images of at least 3×3 pixels it:

1. Estimates pixel noise from a histogram of horizontal adjacent differences:
   `sigma ≈ median(|I[x] − I[x−1]|) / (0.6745 × sqrt(2))`, floored at the
   smallest nonzero difference and at 1. The floor avoids treating a single
   quantization step of 8-bit/JPEG input as a defect.
2. Replaces hot pixels, rows and columns with the median of their eight
   neighbours. A pixel must exceed the lowest neighbour by at least `10 sigma`,
   and either its horizontal or vertical neighbour maximum must carry less
   than 10% of that excess. Up to four passes deal with crossing defects.
3. Applies a separable Gaussian with sigma 1 pixel, radius 3 (seven taps per
   axis), repeating border pixels at the edges.

These are fixed source constants, not measured PSF widths or user controls.
Filtering before detection raises faint stellar peaks relative to noise; defect
removal happens first so isolated defects are not blurred into star-like objects.

### Detection and centroid refinement

`indigo_solver_image_stars()` calls `indigo_find_stars_fast_threshold()` with
radius 16, local threshold factor 3.5 and a maximum of 500 returned stars.
All distances in this subsection refer to the binned detection image.

- A 12×12 grid supplies robust local background medians and MAD noise estimates
  (`sigma = 1.4826 MAD`, with mean/variance fallbacks). The normal edge exclusion
  is 20 pixels; small images use a height-dependent smaller margin.
- Initial candidates are eight-neighbour local maxima above cell median plus
  `4.5 sigma`, ranked by peak above that background.
- The brightest candidates are evaluated until up to `4 × stars_max` pass.
  A 33×33 local median supplies the refined background; at least six of the
  eight adjacent pixels must exceed local background plus `3.5 sigma`.
- Two-iteration frame-digest centroid refinement measures luminance and position.
  PSF measurement rejects unmeasurable/unsuitable HFD (`HFD <= 1.5` or
  `HFD > 1.8 × radius`). Duplicates less than 2 pixels apart are discarded.
  Close neighbours and saturated peaks are flagged by the shared detector;
  the solver does not subsequently filter on those flags.
- The final list is brightness ordered. The solver stores `magnitude =
  1/luminance` as a ranking proxy, not as a calibrated astronomical magnitude.

Fewer than four detected stars fails early. Four stars can form a quad but are
insufficient to pass the final six-star verification.

## Quad construction and invariant codes

### Image sampling

For each depth stage, `indigo_solver_build_image_quads()` divides the binned
image into square cells of integer size `max(width, height) / 10`. It keeps the
brightest stage star in each complete cell, omitting trailing partial cells,
then enumerates all four-cell combinations that contain stars. It stops at
1,000,000 quads. Thus the depth is the number of brightest detections considered,
not the number of stars that necessarily reach quad construction. The grid
reduces clustering and the combinatorial cost, but the truncation is in cell
traversal order, not a ranked best-quad selection.

### Canonical coordinates

The image and indexer use the same geometry. Among four stars choose the
farthest pair as baseline endpoints P1/P2. With `v = P2 − P1`, `L = |v|`,
`ex = v/L`, `ey = (−ex.y, ex.x)`, the remaining point P has normalized coordinates:

```text
x(P) = dot(P − P1, ex) / L
y(P) = dot(P − P1, ey) / L
```

P3 is the lexicographically smaller remaining `(x,y)` point (x first, then y).
Reflect y when necessary so y3 is nonnegative, using y4 if y3 is effectively
zero. Compute this canonical code for both baseline directions and choose the
lexicographically smaller `(x3,y3,x4,y4)`. Translation, rotation, uniform scale
and reflection therefore do not change the ideal code. Noise, distortion and
tangent-plane changes can still perturb it. The P1…P4 correspondence order is
preserved with the code and is essential for matching.

The indexer emits up to eight ambiguity variants, with duplicate hashes removed:
when the baseline-direction x3 comparison, P3/P4 x comparison, or the relevant
y3 sign is within `INDIGO_SOLVER_CODE_AMBIGUITY = 0.005`, it also tries the
alternative choice. This does not enumerate alternative farthest pairs.
Image quads keep only the first canonical code; extra index variants make nearby
ordering changes more likely to match.

Each coordinate is quantized with `rint(1000 × coordinate)` and packed into a
64-bit code (see [Quad hash](#quad-hash)). The field called `hash` is this ordered
packing, not a randomized hash. Catalogue baseline lengths are measured in the
indexer's tangent-plane units of 16 arcseconds; image baselines are binned pixels.
Their ratio is used only for consistency voting, not as the final pixel scale.

## Search and correspondence verification

### Index choice and progressive stages

The built-in scales are 0.2°, 0.5°, 1°, 2°, 5°, 10° and 20°. A scale labels the
quad tier, not the image field. For full dimensions W,H and a scale hint s in
degrees per original pixel, the preferred index minimizes:

```text
target = 0.7 × sqrt(W × H) × s
score(index_scale) = abs(log(index_scale / target))
```

Only selected, readable indexes are candidates in the agent. The CLI defaults
to all installed known scales, or restricts them with `--index`. With known s,
all candidates are ordered by this logarithmic distance at every stage. The
agent falls back from its explicit scale hint to the related imaging agent's
pixel scale. A scale hint orders searches; it is not a hard acceptance bound.

Default depth stages are the brightest 40, then 100, then all detected stars;
stages at/above the detected count collapse into the final stage. A positive
`DEPTH`/`--depth n` instead tries n and then all, when n is smaller than the count.
The loop is **stage first, indexes second**. With unknown scale, the first stage
tries largest scale first (generally smaller files), and subsequent stages
reverse direction to reuse the last mapping. It stops at the first accepted fit.

### Position and parity hints

A positive radius selects catalogue stars near the hint. The cone radius is
`hint_radius + 0.5 × sqrt(W² + H²) × s` when scale is known, or
`hint_radius + index_scale` otherwise (degrees). JNow hints are converted to
J2000 before selection. A declination bound followed by a spherical unit-vector
dot-product test builds one byte per catalogue star; no quad arrays are copied.
A quad is eligible only when all four stars are marked, and fewer than four
cone stars makes that index unusable for the attempt.

The cone is a catalogue-selection hint, **not** a final bound on fitted centre
separation. A wrong position/radius can exclude the real field; there is no
subsequent blind retry outside that cone. Parity 0 accepts either orientation;
±1 rejects an otherwise fitted solution of the opposite parity. Reflection
invariance means parity does not narrow the hash search.

### Hash matching, voting and conflict removal

1. For each image quad, scan up to nine `(hx3,hy3)` buckets within ±1 quantum.
   Accept a catalogue hash only if hx4 and hy4 are also within ±1. The matcher
   checks star indices before dereferencing them and applies the cone mask.
   It records at most 1,000,000 matches.
2. For each match compute `ratio = image_baseline / catalogue_baseline`.
   Every pair of quad matches sharing at least one identical
   `(image star, catalogue star)` correspondence votes once for both matches.
   The highest-voted quad supplies `best_ratio`; without positive votes no
   scale is selected.
3. Retain matches satisfying `abs(1 − ratio/best_ratio) < 0.01`.
   This is the fixed relative scale tolerance, independent of the user's hint.
4. Accumulate votes for each unique image/catalogue star pair, up to 1,000
   distinct pairs. Sort by decreasing votes and greedily retain pairs so each
   image star and each catalogue star occurs at most once.
5. Pass those correspondences to the robust fit below. Hash agreement alone,
   a single matching quad or a high vote count is not a solved image.

This is a deterministic vote/fit pipeline, not a probabilistic false-match
likelihood test. It verifies consistency of the quad-derived star pairs; it
does not perform an additional full-catalogue photometric or unseen-star check.

## Astrometric fit and outputs

### Tangent-plane geometry

`indigo_radec_to_tangent()` uses gnomonic (TAN) projection about `(alpha0,delta0)`.
For `Dalpha = alpha − alpha0`, wrapped to ±pi:

```text
c   = sin(delta0) sin(delta) + cos(delta0) cos(delta) cos(Dalpha)
xi  = cos(delta) sin(Dalpha) / c
eta = [cos(delta0) sin(delta) − sin(delta0) cos(delta) cos(Dalpha)] / c
```

xi points east and eta north. `c <= 0` or non-finite projected coordinates
rejects a point. The tangent-plane coordinates are dimensionless and treated
as radians in the small-angle scale/residual reporting. The inverse is:

```text
denom = cos(delta0) − eta sin(delta0)
alpha = alpha0 + atan2(xi, denom)                 (wrapped to 0…2pi)
delta = atan2(sin(delta0) + eta cos(delta0), sqrt(xi² + denom²))
```

### Robust affine fit

For each matched image point use `dx = x − width/2`, `dy = y − height/2` and fit
six affine coefficients by least squares:

```text
xi  = a0 + a1 dx + a2 dy
eta = b0 + b1 dx + b2 dy
pixel = [hypot(a1,b1) + hypot(a2,b2)] / 2
rms = sqrt(sum((xi − xi_fit)² + (eta − eta_fit)²) / inlier_count)
```

`fit_2d_line()` solves the normal equations for each tangent coordinate using
double-precision sums and Cramer's rule; it rejects a normal-matrix determinant
with absolute value below `1e-14`. The robust search tries each of
the best-voted eight (or fewer) catalogue stars as a tangent point, and every
triple among those candidates as a seed. Each seed is refitted and grown over
all valid matches within `3 × pixel` tangent-plane distance. Iteration stops
when the inlier set stabilizes, degenerates below three points, or reaches 20
iterations. The largest inlier set wins; smaller RMS breaks equal-size ties.

At least **six inliers** are required (`FIT_MIN_INLIERS`). This prevents a lone
four-star quad from certifying itself. A second fit reprojects around the first
fitted centre and regrows the same inlier seed, correcting the orientation
reference for meridian convergence across the field. If that refit fails to
retain six inliers, the preceding valid fit remains available. There is no SIP
or other higher-order optical distortion model.

### Centre, orientation and units

The inverse projection of `(a0,b0)` is the image centre. For the fitted matrix:

```text
det = a1 b2 − a2 b1
parity = +1 if det < 0, otherwise −1
sign = +1 if det >= 0, otherwise −1
angle = −atan2(sign b1 − a2, sign a1 + b2)        (normalized to 0…2pi)
```

Parity +1 is the mirrored sky (east right with north up); −1 is the sky as
seen (east left with north up), following the Astrometry orientation convention.
The agent and CLI convert core results as follows:

| Quantity | Core | Agent/CLI |
| --- | --- | --- |
| RA | radians | hours |
| Dec, angle | radians | degrees; angle in [0,360) |
| Pixel scale | radians per binned pixel | degrees per original pixel, divided by actual bin factor |
| Width, height | not fitted separately | original pixel dimensions × reported scale, in degrees |
| RMS | tangent-plane residual | arcseconds; not a separate estimate of absolute catalogue accuracy |
| Matches | final inliers / candidate star pairs | solved message and CLI result fields |
| Index | scale of selected catalogue | numeric scale in agent WCS; descriptive label in CLI |

EPOCH=0 converts the centre RA/Dec to JNow; otherwise the agent reports J2000.
The CLI accepts 0 or 2000. **Only the centre coordinates are precessed: the
reported angle is left as computed by the catalogue-frame fit.** Catalogue
positions include proper motion to index build time, not to each exposure's
date. Changing the output epoch is not a proper-motion refresh of the index.

The CLI emits text, JSON, XML or a header-only FITS WCS. `--format wcs` always
uses J2000/ICRS coordinates, `RA---TAN`/`DEC--TAN`, `CRPIX=((W+1)/2,(H+1)/2)`
and a 2880-byte-padded header. It reconstructs CD from scalar angle, scale and
parity, rather than exporting the fitted affine shear/unequal-axis scales:

```text
theta = angle + (parity == −1 ? 180 degrees : 0)
f = parity == −1 ? −1 : +1
CD = [ −s cos(theta)     s sin(theta) f ]
     [  s sin(theta)     s cos(theta) f ]
```

The supplied image is not rewritten. JSON/XML output includes a failure result
when unsolved; WCS mode emits nothing on stdout when unsolved. Progress from
`-v` goes to stderr for JSON/XML/WCS and to stdout for text output. Solve exit codes are 0 solved, 1 unsolved, 2 argument error.

## Comparison with Astrometry.net and ASTAP

All three solvers recognize star patterns and derive an image-to-sky mapping.
Their main algorithmic differences are the pattern representation, how reference
patterns are searched, and how candidate matches are verified. The comparison
below describes this INDIGO implementation and the cited project documentation.

**INDIGO** stores precomputed quad codes in ISIX scale tiers. A code contains
the normalized coordinates of two stars relative to the farthest pair,
quantized into four components. Direct bucket lookup finds nearby codes;
matches that share an image/catalogue star correspondence vote for a consistent
baseline scale. Votes then select one-to-one individual star correspondences.
A robust affine TAN fit must retain at least six stellar inliers. Thus a single
quad match cannot establish a solution, and acceptance uses geometric agreement
among the matched stars. The current fit has no higher-order distortion terms.
See [quad construction](#quad-construction-and-invariant-codes),
[verification](#search-and-correspondence-verification) and
[fitting](#astrometric-fit-and-outputs) for the exact thresholds and limits.

**Astrometry.net** also uses precomputed geometric pattern indexes. Its quad
descriptor uses the coordinates of two stars in a frame defined by the
farthest pair, with its own normalization and ordering conventions. Its
documented solving path searches a code KD-tree; a matching field/index quad
supplies catalogue star identities and an initial TAN WCS. It then retrieves
catalogue stars in the proposed field and tests the field-star agreement under
true-match and false-match models. The Bayesian acceptance test supplies
evidence beyond the original quad, whereas INDIGO verifies the star pairs
accumulated from its quad matches. Astrometry.net can subsequently refine the
solution with SIP distortion coefficients.
See the [code structure](https://astrometry.net/doc/code.html) and
[Lang et al. paper](https://arxiv.org/abs/0910.2233).

**ASTAP**, in its documented method, constructs reference patterns from a star
database for trial sky areas with a density matched to the image. Quads use
nearby stars. Their six pairwise distances are sorted, and the five shorter
distances divided by the longest form the shape descriptor. After ratio matches
are found, ASTAP rejects quads whose reference/image longest-side ratio differs
too much from the median. It fits six linear plate constants to the centres
of the remaining matched quads. INDIGO instead searches stored coordinate
codes and fits individual star correspondences. ASTAP's local pattern
construction therefore depends on the selected stars and density as well as
the trial sky area; the algorithm description also allows larger neighbourhoods
to form additional quads in sparse fields. See Han Kleijn's
[algorithm description](https://www.hnsky.org/astap_astrometric_solving.htm)
(version 2026-10-08) and the [ASTAP manual](https://www.hnsky.org/astap.htm)
for search modes and settings.

## Limits, concurrency and memory

The fixed caps are 500 detections, 1,000,000 image quads, 1,000,000 quad matches
and 1,000 unique star-pair candidates. Image enumeration can grow as
`choose(occupied_cells,4)` until capped; quad voting is quadratic in the number
of hash matches. Dense false matches can therefore cost much more than a clean
field despite direct bucket lookup.

`AGENT_PLATESOLVER_HINTS.CPULIMIT`/`--cpu-limit` defaults to 180 seconds; 0 disables it. Despite its
name, the code uses a **monotonic elapsed-time deadline**, starting before image
conversion, so conversion and loading consume the budget. Matching checks the
deadline/abort flag at entry and periodically: normally each 4096 outer
iterations, and each 64 outer iterations in the quadratic voting loop. These
are cooperative checks, not a strict preemptive timeout: image decoding,
filtering, quad construction, cone creation, fitting and individual long inner
loops do not poll the deadline continuously.

The agent uses its configuration mutex to exclude simultaneous native solves,
refuses index changes while WCS is BUSY, and refuses solving during an index
download. Shared task abort generations prevent an abort issued after image
arrival from being lost. The native abort sets the solver flag and also stops
an active download. CLI signal handlers specifically cancel downloads; its
solve path uses the deadline without an equivalent agent abort flag.

Only the most recently used index remains mapped in an agent instance between
solves. Selecting a different index unmaps it; returning to the same path reuses
the existing mapping. This is not an all-index RAM cache. The CLI keeps one
mapping for its current run and frees it on exit. Cone masks are per attempt.
The OS may evict mapped pages, so mapping reuse does not promise zero later disk
I/O. External replacement at the same pathname is not detected by the agent's
path cache; unload/reload the agent for externally installed replacements.

Mapped index bytes are not copied into heap arrays. Hash/bucket lookup mostly
touches those arrays, then baselines and star IDs for matches; hints scan the
star section. The earlier format reference reported roughly 300 MB of index
pages touched while solving a 0.5° field with the 911 MB Gaia 0.5° index. That is
an illustrative historical working set, not a total-process memory bound or a
new benchmark: converted/filter images, large quad/match arrays, masks and
framework data are additional. Index building requires substantially more RAM
than solving because it retains the input star allocation, buckets, expanded
quads, sorting data and output arrays.

Available indexes, star density, centroids and projection/distortion determine
practical coverage; index labels are selection ranges, not guarantees. The
existing narrow-tier guidance notes that G<14 is sparse for many 0.2°×0.16°
fields, especially at high Galactic latitude. More depth cannot invent missing
catalogue stars. Wrong hints, nonstellar defects, too few distributed stars,
strong distortion, truncated candidate arrays or a deadline can all prevent a
solution.

## Index generation

### Input parsing and proper motion

The indexer parses one catalogue into memory, then builds each requested scale.
Without `-g`, it reads exactly `I_259/tyc2.dat.00` through `.19` relative to its
working directory. Tycho-2 fixed-width offsets are zero based:

| Field | Offset | Use |
| --- | --- | --- |
| TYC3 | 11 | Keep component `1`. |
| pflag | 13 | Exclude `X` records. |
| RA, Dec | 15, 28 | Degrees at reference epoch J2000. |
| pmRA, pmDE | 41, 49 | mas/year; pmRA includes cos(Dec). |
| VT | 123 | Keep nonzero values strictly below 11.5; missing/zero VT is skipped. |

Lines shorter than 130 bytes are skipped. Missing required files or more than
2,600,000 accepted stars terminates the process. The Tycho parser uses `atof`
for fixed-position numeric fields; it expects the original catalogue layout,
not a general delimited export.

`-g DIRECTORY` instead reads all non-hidden, lowercase `.csv` files in that
directory, sorted lexicographically. Each file's first line is skipped as a
header without checking its labels. Subsequent lines must have exactly five
unquoted comma-separated fields, in this order:

```text
ra,dec,phot_g_mean_mag,pmra,pmdec
```

RA/Dec are degrees, G is magnitude, and proper motions are mas/year. Literal
lowercase `null` is allowed only for the two proper motions and becomes zero;
empty values, quoted CSV, extra columns, malformed or non-finite numeric values
are not accepted. Invalid rows are skipped. The input must already have the
intended magnitude cut: **the Gaia parser does not impose G<14 itself**. Its
allocation/cap is 24,000,000 stars; files are not recursively discovered and
source IDs are not loaded or used for deduplication.

Both parsers propagate linearly to the wall-clock build date. With t measured
in Julian years of 365.25 days from Unix time 946728000 (J2000 noon):

```text
Delta_t = (time_now − 946728000) / (365.25 × 86400)       # Tycho-2
Delta_t = (time_now − 946728000) / (365.25 × 86400) − 16  # Gaia DR3 J2016.0
alpha += pmra × MAS_TO_RAD × Delta_t / cos(delta)
delta += pmdec × MAS_TO_RAD × Delta_t
```

The RA correction is omitted when `abs(cos(delta)) <= 1e-6`; RA is wrapped to
0…2pi. This is a simple linear proper-motion correction, without parallax,
radial velocity or perspective acceleration. Final coordinates are stored as
single-precision radians in the ICRS catalogue frame. Version 2 does not store
the build epoch, magnitude, source ID or source catalogue name in each star or
in the header; preserve that provenance externally.

### Sky sampling parameters

`select_params()` selects hard-coded parameter bands. The ordinary seven scales
have these exact values; all six star-pair angular distances must be in the
listed interval:

| Scale (degrees) | Catalogue by naming convention | NSIDE | Cells (12 NSIDE²) | Retained stars/cell | Pair distance min…max (degrees) | Reuse setting | Mode |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0.2 | Gaia DR3 | 256 | 786432 | 30 | 0.02…0.12 | 5 per anchor | anchored, 10 nearest neighbours |
| 0.5 | Gaia DR3 | 128 | 196608 | 20 | 0.05…0.30 | 10 per anchor | anchored, 10 nearest neighbours |
| 1 | Tycho-2 | 64 | 49152 | 8 | 0.1…0.8 | 512 | cell-owned |
| 2 | Tycho-2 | 32 | 12288 | 10 | 0.2…1.6 | 512 | cell-owned |
| 5 | Tycho-2 | 16 | 3072 | 12 | 0.4…3.2 | 512 | cell-owned |
| 10 | Tycho-2 | 8 | 768 | 16 | 0.8…6.4 | 512 | cell-owned |
| 20 | Tycho-2 | 4 | 192 | 20 | 1.6…12.8 | 512 | cell-owned |

The often-used description “about 0.1 to 0.8 times the scale” is approximate;
the table is authoritative for actual generator limits. Arbitrary input scales
use the corresponding `<=0.2`, `<=0.5`, `<=1`, `<=2`, `<=5`, `<=10`, or final
parameter band, not parameters continuously scaled to the requested number.

### Sampling and quad selection

1. Place every propagated star in a HEALPix RING cell. Sort each cell by
   magnitude, retain brighter stars while excluding subsequent stars closer
   than half `d_min` to an already retained star in that cell, then truncate
   to `stars_per_cell`. This separation filter is local to the cell, not a
   global catalogue thinning pass.
2. For each cell gather its retained stars and those of up to eight neighbour
   cells. Project them about the current cell centre with gnomonic geometry,
   reference image 1000×1000, angle pi, parity −1 and 16 arcseconds per unit.
   Angular distance tests use spherical distances, while codes/baselines use
   projected x/y.
3. In the Gaia tiers, each retained current-cell star becomes an anchor. Keep
   its ten nearest neighbours within the angular interval, enumerate their
   triples nearest-first, check all inter-neighbour distances, and accept up
   to five or ten physical quads per anchor. Emit ambiguity variants for each.
   Afterwards sort by hash and unordered four-star identity, then remove
   duplicates discovered from multiple anchors.
4. In cell-owned tiers, sort the gathered candidates by magnitude and enumerate
   four-star combinations with pairwise distance/reuse checks. The star with
   the lowest address (input-array position) assigns ownership to its cell,
   preventing repetition across neighbouring cells. The `used > 512` pruning
   checks operate at nested-loop entry and counters reset per cell; this is
   a generation heuristic, not a strict global 512-quad-per-star limit.
5. Sort quads by hash and mark all stars referenced by a surviving quad.
   Compact only those stars, assign zero-based IDs and rewrite quad pointers
   into that compacted array without disturbing P1…P4 order.
6. `indigo_solver_catalog_build()` creates stars, hashes, baselines and four-ID
   arrays, sorts aligned quad records by hash and constructs the bucket starts.
   `indigo_solver_write_index()` writes the version 2 layout below and frees
   the per-scale constructed catalogue afterwards.

The builder is not a streaming converter and does not include a transactional
publication step. It writes into the current directory using `wb`, so rebuilding
there replaces an existing same-name file. Use a separate output directory,
never an index file currently mapped by a running solver.

## Catalogue acquisition and index builds

### Prepare the tools

Use the platform prerequisites in the root [README](../../../README.md) and
[Makefiles guide](../../../indigo_docs/MAKEFILES.md). The following POSIX-shell
commands start at the repository root and build the library prerequisites plus
only the two requested tool targets; they do not build every driver:

```sh
make Makefile.inc
make init
make -j8 -C indigo_libs all
make -j8 -C indigo_tools "$PWD/build/bin/indigo_solver_indexer" "$PWD/build/bin/indigo_solver"
```

The indexer links the INDIGO library; the CLI embeds the needed framework source
objects and links libraw/libjpeg. Both compile the same solver core/geometry.
The Windows solution has the CLI solver project; the above Makefile recipe is
for supported Unix-like builds, not a claim that a separate native MSBuild
indexer target exists. Install the usual system/build dependencies before using
this recipe; do not bypass the repository makefile dependency management.

Save the absolute tool paths before changing into the catalogue/output directory:

```sh
INDEXER="$PWD/build/bin/indigo_solver_indexer"
SOLVER="$PWD/build/bin/indigo_solver"
mkdir -p /tmp/indigo-index-build/I_259 /tmp/indigo-index-build/gaia-dr3-g14
cd /tmp/indigo-index-build
```

For a durable build use a retained work directory with adequate disk/RAM instead
of `/tmp`. The commands use an empty staging directory deliberately; published
indexes total several gigabytes and builder peak memory is larger than output.

### Obtain Tycho-2

Use the CDS [Tycho-2 catalogue I/259](https://cdsarc.cds.unistra.fr/viz-bin/cat/I/259)
and its `ReadMe`. Obtain the original main catalogue parts, decompress if
necessary and place the twenty files as:

```text
I_259/tyc2.dat.00
I_259/tyc2.dat.01
...
I_259/tyc2.dat.19
```

The indexer does not fetch or decompress them, and it does not read the
supplement files. Preserve the catalogue `ReadMe`, source URL and checksums.
With this directory under the working directory:

```sh
"$INDEXER" 1 2 5 10 20
```

No scale arguments means 5°. Up to sixteen numeric scale arguments are handled.

### Obtain Gaia DR3

Use the [ESA Gaia Archive](https://gea.esac.esa.int/archive/) TAP service to export
`gaiadr3.gaia_source`. The following ADQL selects exactly the required columns:

```sql
SELECT ra, dec, phot_g_mean_mag, pmra, pmdec
FROM gaiadr3.gaia_source
WHERE phot_g_mean_mag < 14
```

Request an asynchronous query and CSV output for a large export. If archive row
limits require partitioning, use disjoint ranges such as `ra >= 0 AND ra < 10`,
then 10…20 and so on through `<360`, retaining the magnitude predicate in each
query. Choose smaller ranges if any partition hits a service limit. Confirm
successful completion and no truncation/overflow for **every** partition and
check returned counts against corresponding `COUNT(*)` queries. An incomplete
extract can still produce a syntactically valid but incomplete index.

Export to the `gaia-dr3-g14` directory. Ensure each file has one header and
exactly the five fields in the required order, with unquoted numeric data and
literal `null` proper motions; normalize blank/null encoding if the archive
export differs. Keep manifests and unrelated CSVs outside this directory because
every matching CSV is ingested. Do not overlap partitions or retain both a full
export and its partitions. G<14 is approximately 16.8 million catalogue sources,
not the number of stars eventually retained in the index.

```sh
"$INDEXER" -g gaia-dr3-g14 0.2 0.5
```

`-g` selects the parser independently of the numeric scale. The filename suffix
is derived solely from `scale < 1`: the CLI does not prevent accidentally
building a `-tyc2` filename from Gaia input, or a `-gaia` filename from Tycho-2.
Use the pairings above. A deeper Gaia magnitude selection is a deliberate new
index build requiring capacity/coverage review, not an extra indexer option.

### Reproducibility and validation

Record the source revision, compiler/platform, exact input files and SHA-256s,
query/partition definitions, row counts, commands, stdout/stderr, build UTC time
and printed proper-motion epoch. Keep an output manifest containing filename,
bytes, SHA-256, star/quad counts, catalogue selection, format version and epoch.
The current indexer derives proper motion from `time(NULL)` and has no fixed
epoch option. These instructions reproduce the procedure; later builds are
**not promised to be byte-identical**, even with identical catalogue files.

Before installing or distributing a new index:

1. Check nonzero counts and expected scale, full format/mapping validity, bucket
   endpoints and monotonicity; do not rely only on the header-only `--list` check.
2. Compare input/output counts, sizes and digests with the build record. Sudden
   sparsity can indicate skipped CSV rows, missing data or unexpected filtering.
3. In a separate index directory, solve a representative known image corpus
   with the new files, including tier boundaries, sparse/dense fields, both
   parities, hints, blind solving and proper-motion-sensitive fields. Compare
   centre, scale, angle, match count, RMS, failures, elapsed time and memory
   against the preceding release. A successful solve is required evidence of
   coverage but is not a substitute for complete data/format checks.
4. Relevant existing code checks are in [core unit tests](../../../indigo_test/unit/test_agent_platesolver_core.c),
   [agent integration tests](../../../indigo_test/integration/test_agent_platesolver.c),
   [simulator tests](../../../indigo_test/integration/test_agent_platesolver_simulator.c)
   and [CLI integration tests](../../../indigo_test/integration/test_indigo_solver_tool.c).
   Follow [test instructions](../../../indigo_test/AGENTS.md) when executing them;
   distinguish synthetic checks from validation using the actual release assets.

Example for an existing local test image (the path must be supplied by the maintainer):

```sh
"$SOLVER" -i /tmp/indigo-index-build --list
"$SOLVER" -i /tmp/indigo-index-build -x 0.5 -v -f json /path/to/known-field.fits
```

## ISIX version 2 file format

This section consolidates the earlier format reference, preserving its binary
layout and attribution. ISIX stores catalogue stars and star quads in the
layout used by the runtime. Version 1 packed records are refused; there is no
on-load conversion. Version 2 files are memory mapped on little-endian hosts.

### Files and catalogue conventions

A filename is `index-<scale>deg-<catalogue>.isix`. Scale has two integer digits
(`00.2`, `00.5`, `01`, `02`, `05`, `10`, `20`) for lexical ordering. The built-in
catalogue suffix is `gaia` below 1° and `tyc2` otherwise. Scale is also stored in
the header; filenames alone do not guarantee catalogue identity.

### Common conventions and stars

All integers are little endian and unsigned unless noted. Floats are IEEE 754
single precision. Star IDs are zero-based positions in the star array.

| Star byte offset | Type | Field | Meaning |
| --- | --- | --- | --- |
| 0 | float | ra | ICRS right ascension, radians, with proper motion applied to build time. |
| 4 | float | dec | ICRS declination, radians, with proper motion applied to build time. |

A quad's four IDs are stored in canonical P1,P2,P3,P4 order as defined above,
not sorted numerical ID order. Its baseline is the tangent-plane distance
between P1 and P2 in the indexer's 16-arcsecond units.

### Quad hash

| Bits | Component | Range |
| --- | --- | --- |
| 48…63 | hx3 = rint(1000 x3) | 0…1000 |
| 32…47 | hy3 = rint(1000 y3) | 0…1000 |
| 16…31 | hx4 = rint(1000 x4) | 0…1000 |
| 0…15 | hy4 = rint(1000 y4), signed two's complement | −1000…1000 |

Components are clamped to those ranges before packing. Quads sort by the
unsigned 64-bit packed value, so each `(hx3,hy3)` pair is contiguous. The signed
hy4 component must be decoded as `int16_t`; unsigned lexical ordering within
that last field is not signed numerical y4 ordering.

### Header

The complete header is 64 bytes:

| Offset | Type | Field | Meaning |
| --- | --- | --- | --- |
| 0 | char[4] | magic | `ISIX` |
| 4 | uint32 | version | 2 |
| 8 | float | fov | Quad scale in degrees. |
| 12 | uint32 | star_count | Number of stars. |
| 16 | uint32 | quad_count | Number of quad records, including variants. |
| 20 | uint32 | bucket_side | 1001 |
| 24 | uint64 | stars_offset | Star section offset. |
| 32 | uint64 | buckets_offset | Bucket table offset. |
| 40 | uint64 | hashes_offset | Hash array offset. |
| 48 | uint64 | baselines_offset | Baseline array offset. |
| 56 | uint64 | quad_stars_offset | Quad star-ID array offset. |

### Sections and bucket addressing

Offsets are bytes from the start of the file. Element i in each of the final
three arrays belongs to the same quad i.

| Section | Element | Count | Required alignment |
| --- | --- | --- | --- |
| stars | 2 × float | star_count | 4 |
| bucket table | uint32 | 1001² + 1 = 1,002,002 | 4 |
| hashes | uint64 | quad_count | 8 |
| baselines | float | quad_count | 4 |
| quad star IDs | uint32[4] | quad_count | 4 |

For `k = hx3 × 1001 + hy3`, a bucket covers the half-open quad range
`[buckets[k], buckets[k+1])`. `buckets[0]=0`; entries never decrease; the sentinel
at `buckets[1001²]` equals `quad_count`. This direct address table avoids scanning
all catalogue quads for each image code.

The writer emits sections in the table's order, aligns each start to eight bytes
and zero-fills gaps. Readers must use header offsets, not assume that order or
absence of padding. The written size is:

```text
64 + 8 × star_count (+ padding)
   + 4,008,008 (+ padding)
   + 8 × quad_count
   + 4 × quad_count (+ padding)
   + 16 × quad_count
```

For the Gaia 0.5° file with 3,839,927 stars and 31,293,651 quads this is
910,949,720 bytes. A typical extra quad costs 28 bytes in these arrays; ambiguity
variants therefore count towards both `quad_count` and size.

### Validation boundaries

`indigo_solver_index_read_header()` reads the 64-byte header and checks magic
and current version. `indigo_solver_validate_index()` adds equality of expected
scale and positive star/quad counts. This lightweight check is used for
post-download acceptance and CLI installed/invalid status; **it does not read
all sections or prove the complete file is intact**.

`indigo_solver_load_index()` additionally maps the file and requires:

- a little-endian host and complete header;
- bucket side 1001;
- every section offset at/after the header, element-aligned, and its complete
  declared array inside the mapped size;
- first/last bucket values 0/quad_count and a nondecreasing bucket table.

Star IDs are range checked when a quad is considered; malformed references are
skipped. The loader does not exhaustively recompute code ordering, validate
astronomical positions/baselines, or authenticate the asset with a checksum.
Trusted build provenance and independent digest/coverage checks remain part of
release preparation. The memory mapping behavior is described under
[Limits, concurrency and memory](#limits-concurrency-and-memory).

## Distribution, installation and updates

### Published repository and assets

The distribution repository is
[indigo-astronomy/indigo](https://github.com/indigo-astronomy/indigo), with release
[index_release_v2 — Indigo Solver Indexes V2](https://github.com/indigo-astronomy/indigo/releases/tag/index_release_v2).
The release was published on 2026-10-28; the seven binary assets below were
present when metadata was checked on 2026-11-20. The download base compiled into
`INDIGO_SOLVER_INDEX_URL` is:

```text
https://github.com/indigo-astronomy/indigo/releases/download/index_release_v2
```

The filename is appended to that base, for example
[index-00.5deg-gaia.isix](https://github.com/indigo-astronomy/indigo/releases/download/index_release_v2/index-00.5deg-gaia.isix).

| Asset | Catalogue selection | Bytes | Display size | Approximate preferred field labels |
| --- | --- | ---: | --- | --- |
| `index-00.2deg-gaia.isix` | Gaia DR3 G<14 | 1,216,496,192 | 1216 MB | 0.2°…0.45° |
| `index-00.5deg-gaia.isix` | Gaia DR3 G<14 | 910,949,720 | 911 MB | 0.45°…1° |
| `index-01deg-tyc2.isix` | Tycho-2 VT<11.5 | 538,036,544 | 538 MB | 1°…2° |
| `index-02deg-tyc2.isix` | Tycho-2 VT<11.5 | 357,684,520 | 358 MB | 2°…4.5° |
| `index-05deg-tyc2.isix` | Tycho-2 VT<11.5 | 178,045,032 | 178 MB | 4.5°…10° |
| `index-10deg-tyc2.isix` | Tycho-2 VT<11.5 | 92,385,784 | 92 MB | 10°…20° |
| `index-20deg-tyc2.isix` | Tycho-2 VT<11.5 | 36,114,304 | 36 MB | >20° |

Field labels are rounded selection guidance when all indexes are available,
not minimum/maximum verified fields. The exact selection rule is logarithmic
as above. Files are release assets and cached outside the source repository.

### Install and select

The default directory is `$INDIGO_CACHE_BASE/solver`, or `~/.indigo/solver` when
that environment variable is absent. The CLI's `--index-dir` overrides it.
The cache parent must be usable by the process; create custom parents first.

```sh
indigo_solver --list
indigo_solver --download 0.2,0.5,1
indigo_solver --download all
indigo_solver -i /path/to/index-directory --list
```

Use only the download command(s) appropriate to the needed fields. The CLI
keeps an installed file that passes its header validation and removes/replaces
one that fails. It downloads one file at a time. An explicit alternate source
is supported for CLI downloads, using the same filenames:

```sh
indigo_solver -i /path/to/staging --index-url https://example.org/solver-indexes --download 0.5
```

The example URL is a placeholder for a maintainer's compatible mirror. The
agent uses the compiled base URL and has no equivalent runtime URL property.

In the agent, selecting items in `X_AGENT_SOLVER_INDEX` installs the corresponding
files, and clearing items deletes them. Item names follow
`INDEX_00_2DEG_GAIA`, `INDEX_00_5DEG_GAIA`, `INDEX_01DEG_TYC2`, etc. Installed
readable files appear in `AGENT_PLATESOLVER_USE_INDEX`; select at least one there
to solve. Agent discovery tests readability rather than full validation; a
readable but invalid file can be listed and then fail when loaded. After index
management, newly listed files are selected for use while existing use choices
are preserved.

Downloads use `<filename>.part`, validate the header/scale/counts and rename to
the final path only on success. Failure or abort removes the partial file;
agent startup removes known stale `.part` files. POSIX builds execute `curl -L -s -f`
in a separate process group; ordinary abort sends SIGTERM and exit cleanup can
send SIGKILL. Windows uses WinINet and checks abort between 64 KiB reads, without
requiring an external curl executable. The downloader does not compare a
remote checksum, release revision, timestamp or proper-motion epoch.

### Replace or update installed indexes

There is no automatic “newer release asset” check. Re-running `--download`
retains an existing file with a valid header, even if the remote same-name asset
has changed. The agent likewise keeps a readable installed file.

For an ordinary re-download, with the agent stopped or otherwise not mapping
that file, use:

```sh
indigo_solver --remove 0.5
indigo_solver --download 0.5
```

In the agent, clear the corresponding index-management item, wait for deletion,
then select it again and wait for completion. This route frees its cached
mapping before deleting that index. Reselect it for use if necessary.

For replacing a locally built file, stop the relevant agent, keep the previous
file for rollback, validate the new file in staging, and install it under the
canonical name in the cache directory. Restart/reload the agent so it maps the
new data. Do not truncate/write through a file while another process maps it;
copy into staging and replace only after users of the old file have released it.
Verify the installation with a known image, not just the installed label.
Version 1 files must be rebuilt or re-downloaded; renaming does not convert them.

### Maintainer publication procedure

Publishing is a separate, authorized release operation. First complete the
build/provenance/validation procedure above and preserve previous release
assets and their manifests for rollback. Keep the same tag/filenames only for
format-compatible updates intended for all clients using that URL. A new tag
requires updating `INDIGO_SOLVER_INDEX_URL` and delivering that software change;
old binaries will continue using their compiled tag. Do not put incompatible
format data behind a URL used by readers expecting version 2.

The release page can upload assets, or an authenticated maintainer can use the
[GitHub CLI release upload command](https://cli.github.com/manual/gh_release_upload),
for example after reviewing the staged output:

```sh
gh release upload index_release_v2 /path/to/staged/index-00.5deg-gaia.isix --repo indigo-astronomy/indigo
```

Uploading an existing name fails unless replacement is explicitly requested.
`--clobber` deletes the original before uploading; a failed upload can leave
that asset missing. Retain a recoverable local copy and perform replacement
only as a deliberate release action. Publish provenance, catalogue attribution,
format version, build epoch, sizes and SHA-256s alongside the release notes or
a manifest; the current runtime does not automatically consume that manifest.
After publication verify the actual download URLs and downloaded digests, test
installation in an empty cache and a representative solve, and communicate that
existing installations need explicit refresh. No upload or replacement is part
of generating a local index.

## Adding scales or catalogues

A new output filename alone does not register an index with installed clients.
The current system has a fixed scale list and derives catalogue identity from
scale. These are the implementation touchpoints for a future authorized change:

| Change | Files/functions to review |
| --- | --- |
| Add a registered scale | `INDIGO_SOLVER_INDEX_FOV_COUNT` in `indigo_solver.h`; ascending `indigo_solver_index_fovs`, matching `index_fields` and `indigo_solver_index_sizes` arrays in `indigo_solver.c`. Keep all parallel array lengths/order synchronized. |
| Change naming/catalogue selection | `indigo_solver_index_catalog()`, `indigo_solver_index_scale()` and `indigo_solver_index_path()` in `indigo_solver_utils.c`; catalogue labels in `indigo_solver_index_label()`; agent `index_item_name()`/`index_item_fov()` and CLI scale parsing. |
| Add catalogue input | `indigo_solver_indexer.c` parser/arguments, coordinate frame, reference epoch, magnitude policy, proper-motion convention, invalid-row handling, capacity and ordering/deduplication policy. |
| Add/tune sky coverage | Indexer `select_params()` bands, NSIDE, pair-distance interval, separation, per-cell limit and anchored/cell-owned policy. Rebuild and validate coverage, size and memory. |
| Change code/format semantics | `indigo_solver_utils.h/.c` constants and quad canonicalization; core writer/loader, bucket addressing and matcher. Coordinate a format-version change if existing data would be misinterpreted. |
| Distribute the index | Release assets and `INDIGO_SOLVER_INDEX_URL`; source/display size tables and this reference; cache/update instructions and compatibility with older clients. |
| Change public index items | Agent property documentation in `indigo_docs/PROPERTIES.md`, user guidance and corresponding tests; review saved configurations and clients using exact item names. |
| Add implementation files/build targets | `indigo_tools/Makefile`, driver build metadata, Xcode/Windows projects where applicable, plus relevant core/agent/CLI tests. |

For an additional scale using an existing catalogue, define its sampling
parameters and register it in the scale tables before relying on `--index`,
`--download`, discovery or agent switches. Merely passing an arbitrary float to
the indexer selects one existing parameter band and does not make the scale
known to the solver UI or CLI.

For a new catalogue at an existing scale, the present naming function cannot
distinguish multiple catalogues for that scale. Decide explicitly whether this
is a compatible replacement or requires a richer catalogue identity and
selection model; do not silently label a new source as Tycho-2/Gaia. Convert
coordinates and proper motions to the same frame/units, retain licensing and
provenance, and revisit density limits. A file may be binary-format-compatible
yet unsuitable because its magnitude distribution, quad sizes or epochs differ.

For code quantum, canonical order, baseline units, hash layout or bucket changes,
rebuild affected assets and assess old-reader behavior before selecting a
version/release strategy. Shared source makes generator and runtime consistent
within one build; it does not update already published indexes or old binaries.

## Attribution and references

The original **INDIGO Solver Index Format**, revision 29.10.2026 (draft), was
written by **Rumen G. Bogdanovski** (rumenastro@gmail.com).
Its format tables, conventions, example
size and historical page-working-set observation are consolidated here, with
implementation clarifications and the wider solver/indexer workflow.

The solver/indexer sources credit **Peter Polakovic** and credit **Rumen G.
Bogdanovski** for index management, index optimization and performance
optimization. Code uses the [INDIGO Astronomy open-source license](../../../LICENSE.md).
Catalogue data have their own attribution/usage terms; the code license does
not replace those terms.

- [Tycho-2, CDS I/259](https://cdsarc.cds.unistra.fr/viz-bin/cat/I/259): obtain the catalogue `ReadMe` and preserve its scientific/source attribution.
- [Astrometry.net code structure](https://astrometry.net/doc/code.html) and [Lang, Hogg, Mierle, Blanton and Roweis (2010), *Astrometry.net: Blind astrometric calibration of arbitrary astronomical images*](https://arxiv.org/abs/0910.2233): indexed pattern search, WCS hypotheses and Bayesian verification.
- [Han Kleijn, *ASTAP star pattern recognition and astrometric (plate) solving algorithm*](https://www.hnsky.org/astap_astrometric_solving.htm), version 2026-10-08, and [ASTAP manual](https://www.hnsky.org/astap.htm): local quad construction, distance-ratio matching and plate fitting.
- [ESA Gaia Archive](https://gea.esac.esa.int/archive/): Gaia DR3 data, ESA/Gaia/DPAC; the existing index documentation identifies CC BY-SA 3.0 IGO. Preserve the Gaia/DPAC acknowledgement and applicable data terms with extracts and derived assets.
- [Published ISIX v2 release](https://github.com/indigo-astronomy/indigo/releases/tag/index_release_v2): distribution assets, separate from source.
- [GitHub CLI upload reference](https://cli.github.com/manual/gh_release_upload): maintainer asset upload/replacement semantics.
