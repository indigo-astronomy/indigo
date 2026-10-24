# Independent math/source-contract closure wave

Revision9d45bc8ccadaf0c3aab6c454c38429fec21d1699. Read-only fresh reviewer from driver/platform assignment; root explicitly assigned this closure. Loaded indigo_libs/AGENTS.md. Root's earlier math passes are complementary input, not independently rerun tests. No unsafe probes, runtime driver/hardware operations, exploit reproductions or source edits. Reviewed source contracts and candidate surrounding callers.

## MATH-01 — DONUTS accepts narrow valid regions that violate its FFT/background window bounds

Category functional; severity medium; confidence high; owner coordinator; open, nonblocking follow-up.

`indigo_libs/indigo_raw_utils.c:1237-1248` rejects only nonpositive include widths/heights. It then allocates projection vectors with length next_power_2(include_width/height) at1360ff and runs calibrate_re1206-1230. BG_RADIUS is5 at1072; calibrate_re unconditionally writes vector[0..5] and starts another edge-zeroing loop at size-5, independent of allocation length. Region width/height1–4 therefore produces a vector allocation of1/2/4 elements but out-of-bounds writes before FFT. For a length1 transform `_fft:318-348` has only n==2 as the recursion base, so1 recurses through0 indefinitely. RGB projection filtering also indexes include_width-2/include_height-2 for a one-column/row region. These are valid positive region dimensions accepted by the public function and by agent_guider validate_include_region878-933; the DONUTS include-region path at998ff passes these values. A bright varied narrow region can pass the max/threshold gate. Validate the algorithm's actual minimum window and frame-bounded include rectangle before any reads/allocation; define a clean INDIGO_FAILED/GUIDE_ERROR result for too-small regions or implement small-window algorithms safely. Verify minimum-1/minimum and valid narrowframe/region behavior with synthetic local images under sanitizers. No crash/memory-overflow test was executed in this audit.

Additional small-image star-search boundary to assess in the same algorithm-minimum repair or a separate scoped record: indigo_find_stars_precise_threshold2594-2599 uses clip_edge=height/4 below threshold. At height1–3 this is0, and local-max checks at2804ff read off-1/off-width/diagonals for border candidates. Width is not independently guaranteed to have an interior. Caller RAW shape validation permits positive dimensions, so formal shape validity alone does not establish a safe neighbor stencil. Distinct from prior Bayer012 and PSF007 records.

## MATH-02 — Three-point polar alignment divides by a zero intermediate for solvable geometry

Category functional; severity medium; confidence high; owner coordinator; open, nonblocking follow-up.

`indigo_libs/indigo_align.c:702-710` solves the two equations using v=(d3*k1-d2*k3)/(k4*k1-k2*k3) then u=(d2-v*k2)/k1. When the first two hour angles are symmetric around the meridian (e.g -15°, +15°, followed by +45°), k1=cos(a2)-cos(a1)=0 although the determinant can be nonzero and the system remains solvable. The final division yields nonfinite u while returning true; near-symmetry is also poorly conditioned. This is a legitimate three-distinct-point polar alignment geometry. The actual platesolver reference3 path685 ignores the boolean and694-698 publishes the resulting state OK, passing error values into target coordinate generation692-693. Solve both outputs directly using the determinant (or choose a stable pivot), reject genuinely singular/nonfinite results, and require the caller to honor failure before publishing a target/OK. Add numerical symmetric-meridian and truly degenerate geometry cases. No mount move or solver was run.

## MATH-03 — Platesolver reports an exposure started after its acceptance wait expires

Category functional; severity medium; confidence high; owner coordinator; open, nonblocking follow-up.

`indigo_libs/indigo_platesolver.c:275-329` waits up to60s for Imager capture BUSY/ALERT or1s for Guider process BUSY/ALERT. After either loop it rejects only ALERT; a missing/ignored request whose state remains IDLE/OK falls through to `Capture started` and true. `start_process:477-515` therefore leaves a solve/polar operation waiting for an image even though no fresh capture was accepted. Require observed request acceptance/completion, and return failure/ALERT on bounded acceptance timeout, preserving abort behavior. Characterize absent BUSY, immediate ALERT, accepted capture and fast completed capture without stale state confusion. No long wait or fake protocol reproduction was performed.

## MATH-04 — DONUTS maps an exact half-frame correlation displacement to zero

Category functional; severity medium; confidence high in source arithmetic, caller-range policy should be independently reviewed before canonical registration.

`indigo_libs/indigo_raw_utils.c:382-407` finds the circular correlation maximum. The normal max<n/2 branch returns max+subpixel and max>n/2 returns max-n+subpixel, but exactly max==n/2 returns only subpixel, zero for a symmetric maximum. Either sign of n/2 is conventionally ambiguous at that boundary, but zero is not the half-frame displacement. It also selects neighbors n-1 and1 for max==n/2 instead of the peak's adjacent bins. `indigo_calculate_drift:2516-2523` sends the result directly to guiding; it does not reject the half-window ambiguity. Choose/document a half-window convention or reject that ambiguous measurement; do not silently call it zero drift. Verify circular shifts around n/2 and wrap boundaries with numerically generated correlation arrays. No device or unsafe memory test was executed.

## Known records / non-findings

- GP solve allocation uninitialized-output issue remains existing FND-20261023065605-60d4-repository-audit-010; prediction allocation fallbacks and inference cache failure handling inspected without claiming fix.
- Root's weighted multistar zero-SNR API question remains unconfirmed reachability, not a new registered defect here. Valid centroid producers generally yield positive SNR.
- Root's imager/platesolver preview RAW bounds evidence extends existing022; no duplicate.
- Raw PSF loops retain existing007 (last_star inclusive and failed star measurements), Bahtinov008, star exclusion009, maskedcontrast032 and Bayer012 remain known.
- Multi-point mount alignment placeholders were independently traced to exposed Mount Simulator RW four-item alignment mode and GOTO/park/home/inverse usage. Core reviewer owns its canonical finding; no duplicate registration in this wave.

## Coverage

Complementary manual source spans: raw_utils290-422 FFT/correlation/padding;1093-1480 SNR/background/DONUTS projection;2584-3003 robust-grid star search, candidate ownership and neighbor bounds;3062-3300 PSF grid/range and binarization; GP339-446 FFT/spectrum allocation/dimensions;677-729 stage gate/hyperparameters;890-995 projected prediction/data variance;1047-1208 regularization caller bounds, stage spectrum updates and model origin;1209-1720 prediction windows, spectral estimates, gating/convergence, control blending/dithering; align535-809 spherical/hour-angle conversions, distances/refraction and polar error/search; platesolver184-331 mount/capture acceptance deadlines;434-762 async image solve/abort generation, centering/polar stages;999-1203 property/change public interface;1204-1402 detach and snoop/BLOB async handoff. Root covers remaining functions/algorithms as stated in assignment. Per-file hashes and scope in audit-math-closure-coverage.json. No mandatory gap in this explicitly assigned closure surface; not a proof of every numerical input/concurrency interleaving or hardware accuracy.
