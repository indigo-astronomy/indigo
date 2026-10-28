# Task quality review

Verdict: pass
Reviewer and independence: Claude Fable 5.1 as quality reviewer, same agent as the developer (not independent)
Reviewed revision and working-tree state: uncommitted task working tree on `32d7396d7` (master `42c0960a2` plus the owner's server commit); the merge of origin/master `1e7c23390` (`34f1dea8f`) re-read for the solver test header and array, the status table and the archived notes

## Scope and method
Final diff: the `.driver` change and its regenerated `.c` (version and the two rendering loops only, checked with
`git diff`), the two cases and three helpers added to `test_ccd_simulator.c`, the re-added solver case and header,
MIGRATION_STATUS.md, the README record, TEST_SUMMARY.md and this record (the notes first written into the two
REFACTOR.md files moved into verification.md when master archived those files). Read against the root and test
AGENTS.md rules (tabs, braces, single-line calls, no blank lines in bodies, spaces in initializers, English, version
bump, generated outputs regenerated not hand-edited).

## Quality findings
- The regenerated `.c` carries exactly the `.driver` change; the three mis-indented lines of the old guider loop are
  now indented with their block because they were rewritten, no other reformatting.
- `sim_star_profile()` walks the window twice (centroid, then second moment about it); a one-pass formula would save
  a loop but lose precision on the large coordinates. Nonblocking.
- The imager case hard-codes the region holding the brightest star of the fixed image (found from the data) so that
  blocks summing beyond 65535 are exercised; the constants are documented in the comment. Nonblocking.
- `sim_brightest_peak_below()` scans the whole frame with a 7 x 7 neighbourhood test per candidate; it runs once per
  frame in one case, well under a second. Nonblocking.
- Out of scope, registered as FND-20261027140318-5859-fix-simulator-binning-001 (low, deferred): the hot pixel
  loop of `create_frame()` tests `y > frame_height` where `y >= frame_height` is meant, so a hot pixel whose binned
  row equals the bottom edge of a sub-frame is written one row below the frame; the buffer is the unbinned size, so
  the write lands in its slack except for columns beyond 1439 of a full-width row. Pre-existing, unrelated to binning,
  found by source audit.
- The solver's thin margin on the binned Rosetta field, seen while rejecting the summed variant, is registered as
  FND-20261027140318-5859-fix-simulator-binning-002 (low, deferred) for the solver rather than tuned here.
No scope drift: the solver, the bin handler and the frame geometry are untouched.

## Fix reinspection
The two regression cases pin both rendering changes with values a reader can check against the fixed image and the
Gaussian profile; the solver case is the end to end acceptance. Retest evidence in verification.md.

## Quality verdict
Pass; limitations are the same-agent review and the unrecorded full solver suite (only its Rosetta set was re-run).
