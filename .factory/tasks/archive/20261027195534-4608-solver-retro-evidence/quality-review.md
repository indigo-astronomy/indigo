# Task quality review

Verdict: pass
Reviewer and independence: Claude Fable 5.1 in the quality-reviewer role, after the fact; the commit was written in
another session by the same model under the owner, and this reviewer also writes every other report of this task,
so this is a separate review pass, not an independent reviewer
Reviewed revision and working-tree state: the published commit `a1e7f53d08bcec9b25309f1a2944d39451572253` against its
parent `32d7396d7257e33a3afc3520990151022c6fafcc`, read with `git show`; the task working tree holds only this
record; the current source (3.0.0.8, `master` `05f14156a`) was consulted where noted

## Scope and method
The full diff of the ten files of `a1e7f53d0` (830 insertions, 46 deletions): `indigo_drivers/agent_solver/
indigo_solver.c`, `indigo_solver.h`, `indigo_agent_solver.c`, `README.md`, `REFACTOR.md`, `MIGRATION_STATUS.md`,
`TEST_SUMMARY.md`, `indigo_test/unit/test_agent_solver_core.c`, `indigo_test/integration/test_agent_solver.c`,
`indigo_test/integration/test_agent_solver_simulator.c`, read against the specification of this task and the
surrounding code (`indigo_safe_malloc` zeroing in `indigo_bus.h`, the JPEG error handling pattern of
`indigo_agent_astrometry.c` and `indigo_ccd_driver.c`, `indigo_find_stars_precise_threshold`). Checked for:
duplication, dead code, weak tests, misleading documentation, scope drift, and the formatting rules of root
`AGENTS.md` (braces, single-line calls, no blank lines in bodies, tabs, `{ 0 }` initialisers). Not covered: the
Linux and Windows builds, the behaviour on a real libraw file (no fixture), performance of the filter on large frames.

## Quality findings
None blocking; no FND record created. Observations without a finding:
- `jpeg_image()` assigns `image` and `row` after `setjmp()` and frees them in the `longjmp()` branch without
  `volatile`. Registers are restored to their `setjmp()` values, so on an error raised during scanline decoding the
  pointers may read as NULL and the buffers leak; nothing is freed twice or used after free. The same pattern exists
  in `indigo_agent_astrometry.c` and `indigo_ccd_driver.c`, the input is a client upload that already failed, and the
  leak is bounded by one frame. Recorded as an observation for a later cleanup of all three sites, not as a defect
  of this commit.
- `fits_values()` computes the data length as `(long)(pixels * planes * bytes)`; with 65535 x 65535 x 16 planes x 8
  bytes the product exceeds a 32-bit `long`, so the truncation check would be unreliable where `long` is 32 bits.
  The Solver Agent is not built on Windows (MIGRATION_STATUS.md) and `long` is 64 bits on the supported Linux and
  macOS targets; see security.md for the assessment.
- `image_noise_sigma()` relies on `indigo_safe_malloc()` zeroing the 65536-bin histogram; confirmed in
  `indigo_bus.h`. The function reads horizontally adjacent pairs only, which is what REFACTOR.md describes.
- Duplication: the RGB-to-mono averaging appears three times (RAW RGB24, RGB48, libraw 8 and 16 bit); each
  variant differs in the input width and scaling, and a shared helper would save a few lines at the price of a
  generic accessor. Left as is.
- Tests: `image_stars_noisy_frame` asserts detection within 1.5 px, that no defect is detected and that the input
  frame is byte-identical afterwards, which covers the three claims of the filter. The format cases render the same
  synthetic field through every reader and assert the solved centre, and the two rejection cases check the exact
  messages. The simulator cases exercise the FITS and JPEG uploads of the Imager Agent. The libraw path has no
  fixture; REFACTOR.md records that.
- Documentation: README limitation and index wording match the code (quad scale, formats, 3.0.0.7 record);
  REFACTOR.md says "Version 3.0.0.6" for the filter and "Version 3.0.0.7" for the formats while the commit carries
  a single bump from 0x03000005 to 0x03000007; that is the chronology of the work inside one commit, not a mismatch
  with the shipped version.
- Scope: the README index wording change is unrelated to the two features but is small, documented in the commit
  message and corrects a misleading statement. No other drift.
- Formatting: braces on every body, single-line calls, tabs, `{ 0 }` initialiser in `dslr_image()`; `git diff
  --check 32d7396d7 a1e7f53d0` reports nothing.

## Fix reinspection
No fixes were made by this task; the reviewed change is published. The two defects the commit's own format tests
found (zero noise estimate on 8 bit data, sharp stars taken for spikes) are described in REFACTOR.md and their
corrections (quantisation step floor, 10 % neighbour rule) are in the reviewed code; the re-run tests in
verification.md cover them.

## Quality verdict
Pass for the diff of `a1e7f53d0`, after the fact. Limitations: same-agent review of an already published commit;
no Linux/Windows inspection; the libraw path judged by reading only. Next assignment: see handoff.md.
