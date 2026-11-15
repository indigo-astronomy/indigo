# Task brief

- Task ID: 20261114080917-4a7c-scheduler-reset-delete-all
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-14

## Problem and desired outcome
Owner request 2026-11-14, after a comparison of how other schedulers treat finished work: "delete all and reset all in task management property", with a confirmation hint before deleting, later "reset all should have a hint too". Owner decisions: DELETE_ALL keeps the tasks named as hooks; DELETE_ALL is accepted only while the scheduler is stopped; both items carry `warn_on_set`.

## Users and use cases
Observers who start a new observing list, or rerun all finished tasks.

## Scope and non-goals
In scope: agent, names header, PROPERTIES.md, SCHEDULER.md, user guide, integration tests, sandbox verbs, MIGRATION_STATUS count, recorded run. Non-goals: progress counted per frame; clearing the history.

## Constraints and references
Competitor comparison in this conversation (Ekos, SGP, N.I.N.A. Target Scheduler, Voyager RoboTarget, ACP): finished work is kept until deleted by hand; most offer a bulk reset; start/end actions live apart from targets.

## Initial questions
Answered by the owner (above). RESET_ALL with a running task: coordinator default, the running task is left alone (owner did not object).
