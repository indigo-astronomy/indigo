# Task brief

- Task ID: 20261113090002-9520-scheduler-related-agent-plan
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-13

## Problem and desired outcome
The Scheduler Agent uses fixed agent names ("Scripting Agent", "Mount Agent", "Guider Agent"), so it works only on the same server as those agents. Owner request 2026-11-13: "please add this to the plan we need only scripting agent to be selected. and we will meed aux when we add safety".

## Users and use cases
Observers whose Scripting Agent runs on another INDIGO server than the Scheduler Agent.

## Scope and non-goals
In scope: the implementation plan and architecture text in `indigo_docs/SCHEDULER.md`, and the current same-server requirement in `indigo_docs/INDIGO_SCHEDULER_GUIDE.md`. Non-goals: code changes.

## Constraints and references
Related-agent pattern: `FILTER_RELATED_AGENT_LIST` of the filter agents; AUX device lists `FILTER_AUX_<n>_LIST` in `indigo_names.h`.

## Initial questions
None.
