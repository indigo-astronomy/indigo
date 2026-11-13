# Task brief

- Task ID: 20261113001457-32cc-scheduler-shutdown-command-leak
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
Owner request 2026-11-13: "fix 004" — FND-20261112184305-e0a6-scheduler-agent-core-004, requests queued when the Scheduler Agent shuts down are leaked.

## Users and use cases
Servers that load and unload the agent repeatedly; sanitizer runs.

## Scope and non-goals
In scope: the agent's request queue at shutdown, a regression test. Non-goals: framework (indigo_libs) changes.

## Constraints and references
Branch `scheduler`; no commits before owner approval.

## Initial questions
None.
