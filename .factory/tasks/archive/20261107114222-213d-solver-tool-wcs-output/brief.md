# Task brief

- Task ID: 20261107114222-213d-solver-tool-wcs-output
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-07

## Problem and desired outcome
The owner asked: "add wcs output to indigo solver". The `indigo_solver` command line tool (`indigo_tools/indigo_solver.c`) prints its solution as text, JSON or XML, but cannot write a WCS file that other software (viewers, stackers, Astrometry.net tools) can apply to the image. The desired outcome is a WCS file of the solution, the same as the "Save WCS" button of Ain Platesolver (indigo_imager commit 5e2cae0, `ain_platesolver_src/wcsfile.cpp`) writes.

## Users and use cases
Command line and script users who solve an image file and want to attach the solution to it, as with the `.wcs` files of Astrometry.net `solve-field` and ASTAP.

## Scope and non-goals
In scope: a `-w, --wcs <file>` option of `indigo_solver`, its help text and exit code, and tests in `indigo_test/integration/test_indigo_solver_tool.c`. Not in scope: writing WCS into the image file itself, SIP distortion terms, the Platesolver Agent, the Solve page, Ain Platesolver.

## Constraints and references
Same card layout and CD-matrix convention as `wcs_file_header()` of Ain Platesolver, so both tools write the same file for the same solution. Portable C (Linux, macOS, Windows). No README change without approval.

## Initial questions
Option name and file naming: an explicit file argument was chosen (scriptable, like `--format`); no default file name next to the image. Recorded as a low-risk assumption in specification.md.
