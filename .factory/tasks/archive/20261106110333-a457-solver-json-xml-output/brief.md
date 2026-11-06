# Task brief

- Task ID: 20261106110333-a457-solver-json-xml-output
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-06

## Problem and desired outcome
The `indigo_solver` command line plate solver (`indigo_tools/indigo_solver.c`) prints its solution only as human-readable
text. The owner asked for JSON and XML output of the solution, so scripts and other programs can read it.

## Users and use cases
Scripts and applications that run `indigo_solver` on an image file and need the solved centre, angle, scale, field and
parity, or the reason the image was not solved.

## Scope and non-goals
In scope: an output format switch, JSON and XML documents for solved and not-solved results, help text, tests.
Non-goals: changes to the solver algorithm, to the Platesolver Agent, or to the text output.

## Constraints and references
Root AGENTS.md formatting and C conventions; INDIGO JSON/XML escape helpers (`indigo_json.h`, `indigo_xml.h`);
existing suite `indigo_test/integration/test_indigo_solver_tool.c`.

## Initial questions
None blocking. The switch name, field names and the not-solved document are routine choices, recorded in specification.md.
