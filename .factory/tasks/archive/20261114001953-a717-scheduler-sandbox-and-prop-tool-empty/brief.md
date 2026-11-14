# Task brief

- Task ID: 20261114001953-a717-scheduler-sandbox-and-prop-tool-empty
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-14

## Problem and desired outcome
Owner request 2026-11-14: "yes please make such tool and fix proptool to set emty values", after the observation that the scheduler is extremely hard to test by hand. Deliver a hand-testing tool that runs the Scheduler Agent in a private server and moves the site to choose the time of day, and let `indigo_prop_tool` send empty text values.

## Users and use cases
Developers and observers trying scheduler behaviour (hooks, nights, urgent tasks) without waiting for the real sky; anyone clearing a text item from the command line.

## Scope and non-goals
In scope: `tools/scheduler_sandbox.py`, `tools/scheduler_sandbox_night.txt`, `indigo_tools/indigo_prop_tool.c`, parser test and its harness, `indigo_test/Makefile` (opt-in target), Xcode project, `INDIGO_SCHEDULER_GUIDE.md`, `PROPERTY_MANIPULATION.md`. Non-goals: changes to the Scheduler Agent; the indigocat equinox question and unrelated tool-safety failures (findings 001 and 002).

## Constraints and references
indigo_test/integration/run_scheduler_simulators.py (JSON client pattern); indigo_agent_scheduler_night.c (night computation the sandbox must agree with).

## Initial questions
None.
