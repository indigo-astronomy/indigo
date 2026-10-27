# Specification

Status: accepted

## Problem and outcome
The published commit `a1e7f53d08bcec9b25309f1a2944d39451572253` lacks every Factory artefact. This task adds the
record after the fact without touching the published change. The specification therefore has two parts: what the
reviewed commit delivered (the behaviour this record documents and verifies) and what this task delivers (the
record itself).

## In scope
Behaviour of the Solver Agent (`indigo_drivers/agent_solver`) at `a1e7f53d0`, version 3.0.0.7, relative to its
parent `32d7396d7257e33a3afc3520990151022c6fafcc` (version 3.0.0.5):

1. Star detection of mono frames (RAW MONO8 and MONO16) runs on a filtered copy: the pixel noise sigma is estimated
   from the median absolute difference of horizontally adjacent pixels and is at least the quantisation step of the
   data; pixels more than 10 sigma above the lowest of their eight neighbours whose horizontal or vertical
   neighbours carry less than 10 % of that excess (hot pixels, columns, rows) are replaced by the neighbour median,
   in up to four passes; a separable gaussian blur of sigma 1 px is applied. The frame passed to the solver is left
   unchanged; colour frames are detected as before.
2. Every input is loaded through `indigo_solver_load_image()` as a RAW MONO16 image: RAW MONO16 copied, MONO8 spread
   over 16 bits, RGB24/RGB48 averaged to mono; FITS primary images with BITPIX 8, 16, 32, -32, -64, BZERO/BSCALE and
   NAXIS 2 or 3 (planes averaged), physical values within 0 to 65535 kept, any other range mapped onto it; JPEG
   decoded with libjpeg and its components averaged; anything else through libraw. The downsampling hint applies to
   every input because every input is MONO16 after loading.
3. Unreadable input fails the solve with the reason in the agent message: "Truncated RAW image", "Truncated FITS
   header", "Truncated FITS image", "Unsupported FITS image", "Broken JPEG image", "Unsupported JPEG image",
   "Unsupported camera raw image", "Unsupported image format".
4. Documentation: README describes the index parameter as the quad scale (0.1 n° to 0.8 n° quads for an index of
   scale n°), the input formats and the recorded run; REFACTOR.md records the sweep analysis, the filter, the
   format conversion and the defects found by the format tests; MIGRATION_STATUS.md and TEST_SUMMARY.md carry the
   141 / 0 count; `DRIVER_VERSION` is 0x03000007.
5. Tests: unit `image_stars_noisy_frame`; agent `solve_fits_image`, `solve_float_fits_image`,
   `solve_mono8_raw_image`, `solve_rgb48_raw_image`, `solve_jpeg_image` and the rejection cases asserting
   "Unsupported image format" and "Truncated FITS image"; simulator `rosetta fits`, `rosetta jpeg` and every
   fifteenth guider sweep setup captured as FITS.

This task delivers: brief, specification, plan, test plan, handoff, verification, quality review and security
review for that change; process evidence; the owner's audit deferral entry in `.factory/maintenance.json`; the
archived task directory and its completion record; one commit on `task/20261027195534-4608-solver-retro-evidence`.

## Out of scope
Any modification of the reviewed source, tests or documentation; rewriting or amending `a1e7f53d0`; a new recorded
driver test run; integration into `master`; publication; the 3.0.0.8 changes of the later task.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The task record exists under `.factory/tasks/archive/20261027195534-4608-solver-retro-evidence/` with every standard report populated and passing verdicts | `./.factory/scripts/archive-task --dry-run`, then the move; committed-range process check |
| AC-002 | The record states that the reviewed commit is `a1e7f53d0`, already published on `master`, that history was not rewritten and that the reviews are after the fact by one agent | inspection of handoff.md, verification.md, quality-review.md, security.md |
| AC-003 | The checks that can be re-run on the current tree were re-run and their results recorded: the Solver Agent driver build, `test_agent_solver_core`, `test_agent_solver`, and the opt-in `test_agent_solver_simulator` when the Tycho-2 indexes are installed | verification.md executed-checks table |
| AC-004 | The behaviour of items 1 to 5 above is confirmed from the committed diff and the re-run tests | verification.md acceptance table, quality-review.md |
| AC-005 | Process evidence is recorded; the committed-range process check passes, or its remaining limitation is documented in handoff.md | `python3 .factory/scripts/process_check.py --base <master> --head HEAD` |
| AC-006 | The owner's deferral of the due full repository audit for this task is recorded in `.factory/maintenance.json` only after the owner answered in chat | maintenance.json entry with the owner's answer |
| AC-007 | No file outside `.factory/` changes in this task; no push and no integration into `master` | `git diff --stat master...HEAD`; `git status` |

## Failure behavior and edge cases
If a re-run check fails on the current tree (3.0.0.8), the failure is recorded as a finding against the current
source, not hidden; the record of `a1e7f53d0` still states the commit's own recorded run. If the owner declines the
deferral, the task stays live and blocked until the audit is done or another decision is made. If version 1 process
evidence cannot pass, version 2 content identities cover the task's own evidence-only change and handoff.md explains
why the published commit cannot be bound.

## Permissions, data, and security requirements
No publication, no integration, no README change, no source change. The security review covers the reviewed commit's
handling of client-supplied image data (RAW, FITS, JPEG, camera raw) after the fact.

## Compatibility and nonfunctional requirements
Not applicable to the record itself. The reviewed commit adds libjpeg and libraw use to the solver core, both already
linked by the INDIGO library; Linux and Windows builds of the commit were not run (the agent is not built on Windows,
MIGRATION_STATUS.md).

## Assumptions and open questions
- Low-risk assumption: the README `## Testing` record of `a1e7f53d0` (141/141 at 19:48) is the recorded run of
  that commit; it is reproduced from `git show a1e7f53d0:indigo_drivers/agent_solver/README.md`.
- Owner question: audit deferral (AC-006).

## Owner decisions and requirement changes
- 2026-10-27, Peter Polakovic, in conversation: the Factory rules must always be followed; a retroactive evidence
  task is the agreed repair for `a1e7f53d0`. Affects every criterion.
