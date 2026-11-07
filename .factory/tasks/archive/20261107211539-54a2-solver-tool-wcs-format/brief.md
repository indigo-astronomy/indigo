# Task brief

- Task ID: 20261107211539-54a2-solver-tool-wcs-format
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-07

## Problem and desired outcome
The owner reported: "indigo_solver shows the solution when wcs is selected. wcs output is inconsistent with json and xml". The `-w, --wcs <file>` option of `indigo_tools/indigo_solver.c` (task 20261107114222-213d-solver-tool-wcs-output, commit 93807aff3) wrote a side file while stdout still carried the text result. The owner approved the proposal "add -f wcs": the WCS header becomes a fourth output format on stdout, like json and xml, and `-w` is removed.

## Users and use cases
Command line and script users saving a `.wcs` file of a solved image: `indigo_solver -f wcs image.fits > image.wcs`.

## Scope and non-goals
In scope: `-f wcs`, removal of `-w` and exit code 3, help text, `indigo_test/integration/test_indigo_solver_tool.c`. Not in scope: the header content (unchanged), the Platesolver Agent, Ain Platesolver.

## Constraints and references
Portable C (Linux, macOS, Windows). The header content stays the convention of Ain Platesolver `wcs_file_header()`. No README change.

## Initial questions
None; the behaviour was proposed to and approved by the owner in the conversation (2026-11-07).
