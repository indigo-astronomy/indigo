# Task brief

- Task ID: 20261114005850-ff89-scheduler-sandbox-discovery
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-14

## Problem and desired outcome
Owner report 2026-11-14: "i typed python3 tools/scheduler_sandbox.py but i can not see the server with control panel". The sandbox server ran on a random port with Bonjour disabled, so clients did not list it, and only a localhost URL was printed. Announce it and print a URL usable from another computer.

## Users and use cases
Users of tools/scheduler_sandbox.py watching the sandbox in a client or a browser.

## Scope and non-goals
In scope: `tools/scheduler_sandbox.py`, its section in `indigo_docs/INDIGO_SCHEDULER_GUIDE.md`. Non-goals: server changes.

## Constraints and references
indigo_server `-b name` / `-b-`.

## Initial questions
None.
